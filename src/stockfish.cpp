#include "stockfish.hpp"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
extern char **environ;
#endif

namespace {
using Clock = std::chrono::steady_clock;

void validarDificuldade(int nivel) {
    if (nivel < 0 || nivel > 20) {
        throw std::invalid_argument("A dificuldade deve estar entre 0 e 20.");
    }
}

void validarTempoPensamento(int tempoMs) {
    if (tempoMs < 1 || tempoMs > 60000) {
        throw std::invalid_argument("O tempo de pensamento deve estar entre 1 e 60000 ms.");
    }
}

class Process {
  public:
    explicit Process(const arquivos::path &executable) {
#ifdef _WIN32
        SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
        HANDLE childInput = nullptr, childOutput = nullptr;
        if (!CreatePipe(&childInput, &input_, &security, 0)) {
            fail("CreatePipe stdin");
        }
        if (!CreatePipe(&output_, &childOutput, &security, 0)) {
            CloseHandle(childInput);
            CloseHandle(input_);
            input_ = nullptr;
            fail("CreatePipe stdout");
        }
        if (!SetHandleInformation(input_, HANDLE_FLAG_INHERIT, 0) ||
            !SetHandleInformation(output_, HANDLE_FLAG_INHERIT, 0)) {
            CloseHandle(childInput);
            CloseHandle(childOutput);
            CloseHandle(input_);
            CloseHandle(output_);
            input_ = output_ = nullptr;
            fail("SetHandleInformation");
        }
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdInput = childInput;
        startup.hStdOutput = childOutput;
        startup.hStdError = childOutput;
        PROCESS_INFORMATION info{};
        std::wstring command = L"\"" + executable.wstring() + L"\"";
        const BOOL ok = CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
                                       CREATE_NO_WINDOW, nullptr, executable.parent_path().c_str(),
                                       &startup, &info);
        const DWORD code = GetLastError();
        CloseHandle(childInput);
        CloseHandle(childOutput);
        if (!ok) {
            CloseHandle(input_);
            CloseHandle(output_);
            input_ = output_ = nullptr;
            throw std::runtime_error("CreateProcessW falhou, código " + std::to_string(code));
        }
        process_ = info.hProcess;
        CloseHandle(info.hThread);
#else
        // Evita que uma falha do motor encerre o jogo ao escrever no pipe.
        std::signal(SIGPIPE, SIG_IGN);
        int in[2], out[2];
        if (pipe(in) != 0) {
            fail("pipe stdin");
        }
        if (pipe(out) != 0) {
            close(in[0]);
            close(in[1]);
            fail("pipe stdout");
        }
        for (int fd : {in[0], in[1], out[0], out[1]}) {
            fcntl(fd, F_SETFD, FD_CLOEXEC);
        }
        posix_spawn_file_actions_t actions;
        posix_spawn_file_actions_init(&actions);
        posix_spawn_file_actions_adddup2(&actions, in[0], STDIN_FILENO);
        posix_spawn_file_actions_adddup2(&actions, out[1], STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&actions, out[1], STDERR_FILENO);
        for (int fd : {in[0], in[1], out[0], out[1]}) {
            posix_spawn_file_actions_addclose(&actions, fd);
        }
        std::string path = executable.string();
        char *argv[] = {path.data(), nullptr};
        const int result = posix_spawn(&pid_, path.c_str(), &actions, nullptr, argv, environ);
        posix_spawn_file_actions_destroy(&actions);
        close(in[0]);
        close(out[1]);
        if (result != 0) {
            close(in[1]);
            close(out[0]);
            pid_ = -1;
            throw std::runtime_error("Não foi possível iniciar Stockfish: " +
                                     std::string(std::strerror(result)));
        }
        input_ = in[1];
        output_ = out[0];
#endif
        try {
            reader_ = std::thread([this] {
                readOutput();
            });
        } catch (...) {
            shutdown();
            throw;
        }
    }

    ~Process() {
        shutdown();
    }

    Process(const Process &) = delete;
    Process &operator=(const Process &) = delete;

    void send(const std::string &command) {
        const std::string data = command + "\n";
        std::size_t offset = 0;
        while (offset < data.size()) {
#ifdef _WIN32
            DWORD written = 0;
            if (!WriteFile(input_, data.data() + offset, static_cast<DWORD>(data.size() - offset),
                           &written, nullptr) ||
                !written) {
                fail("WriteFile Stockfish");
            }
#else
            const auto written = write(input_, data.data() + offset, data.size() - offset);
            if (written < 0 && errno == EINTR) {
                continue;
            }
            if (written <= 0) {
                fail("write Stockfish");
            }
#endif
            offset += static_cast<std::size_t>(written);
        }
    }

    std::string readLine(Clock::time_point deadline) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!condition_.wait_until(lock, deadline, [this] {
                return ended_ || !lines_.empty();
            })) {
            throw std::runtime_error("Tempo limite aguardando resposta do Stockfish.");
        }
        if (lines_.empty()) {
            throw std::runtime_error(
                "Stockfish encerrou inesperadamente. Confira binário e redes NNUE.");
        }
        std::string line = std::move(lines_.front());
        lines_.pop_front();
        return line;
    }

  private:
    [[noreturn]] static void fail(const std::string &what) {
#ifdef _WIN32
        throw std::runtime_error(what + ": erro " + std::to_string(GetLastError()));
#else
        throw std::runtime_error(what + ": " + std::string(std::strerror(errno)));
#endif
    }

    void readOutput() {
        char buffer[4096];
        std::string pending;
        for (;;) {
#ifdef _WIN32
            DWORD count = 0;
            if (!ReadFile(output_, buffer, sizeof(buffer), &count, nullptr) || count == 0) {
                break;
            }
#else
            const auto count = read(output_, buffer, sizeof(buffer));
            if (count < 0 && errno == EINTR) {
                continue;
            }
            if (count <= 0) {
                break;
            }
#endif
            pending.append(buffer, static_cast<std::size_t>(count));
            std::size_t newline;
            while ((newline = pending.find('\n')) != std::string::npos) {
                std::string line = pending.substr(0, newline);
                pending.erase(0, newline + 1);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    lines_.push_back(std::move(line));
                }
                condition_.notify_one();
            }
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ended_ = true;
        }
        condition_.notify_all();
    }

    void shutdown() noexcept {
        try {
            send("quit");
        } catch (...) {
        }
#ifdef _WIN32
        if (input_) {
            CloseHandle(input_);
            input_ = nullptr;
        }
        if (process_) {
            if (WaitForSingleObject(process_, 1500) != WAIT_OBJECT_0) {
                TerminateProcess(process_, 1);
                WaitForSingleObject(process_, INFINITE);
            }
        }
#else
        if (input_ >= 0) {
            close(input_);
            input_ = -1;
        }
        if (pid_ > 0) {
            bool exited = false;
            const auto deadline = Clock::now() + std::chrono::milliseconds(1500);
            do {
                const auto result = waitpid(pid_, nullptr, WNOHANG);
                if (result == pid_ || (result < 0 && errno == ECHILD)) {
                    exited = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            } while (Clock::now() < deadline);
            if (!exited) {
                kill(pid_, SIGKILL);
                while (waitpid(pid_, nullptr, 0) < 0 && errno == EINTR) {
                }
            }
            pid_ = -1;
        }
#endif
        if (reader_.joinable()) {
            reader_.join();
        }
#ifdef _WIN32
        if (output_) {
            CloseHandle(output_);
            output_ = nullptr;
        }
        if (process_) {
            CloseHandle(process_);
            process_ = nullptr;
        }
#else
        if (output_ >= 0) {
            close(output_);
            output_ = -1;
        }
#endif
    }

#ifdef _WIN32
    HANDLE input_ = nullptr, output_ = nullptr, process_ = nullptr;
#else
    int input_ = -1, output_ = -1;
    pid_t pid_ = -1;
#endif
    std::thread reader_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<std::string> lines_;
    bool ended_ = false;
};
} // namespace

struct Stockfish::Impl {
    Process process;
    int milliseconds;
    int skill;
    bool trace;

    Impl(const arquivos::path &path, int time, int skill, bool log)
        : process(path), milliseconds(time), skill(skill), trace(log) {
        send("uci");
        waitFor("uciok", 20000);
        send("setoption name Threads value 1");
        send("setoption name Hash value 16");
        send("setoption name Skill Level value " + std::to_string(skill));
        ready();
    }

    void send(const std::string &line) {
        if (trace) {
            std::cerr << ">> " << line << '\n';
        }
        process.send(line);
    }

    std::string waitFor(const std::string &prefix, int timeout,
                        std::vector<std::string> *melhores = nullptr) {
        const auto deadline = Clock::now() + std::chrono::milliseconds(timeout);
        for (;;) {
            auto line = process.readLine(deadline);
            if (trace) {
                std::cerr << "<< " << line << '\n';
            }
            if (melhores && line.rfind("info ", 0) == 0) {
                const auto multi = line.find(" multipv ");
                const auto pv = line.find(" pv ");
                if (multi != std::string::npos && pv != std::string::npos) {
                    int index = 0;
                    std::istringstream rank(line.substr(multi + 9));
                    std::istringstream variation(line.substr(pv + 4));
                    std::string move;
                    if ((rank >> index) && (variation >> move) && index >= 1 && index <= 3) {
                        melhores->resize(3);
                        (*melhores)[index - 1] = move;
                    }
                }
            }
            if (line == prefix || line.rfind(prefix + " ", 0) == 0) {
                return line;
            }
        }
    }

    void ready() {
        send("isready");
        waitFor("readyok", 20000);
    }
};

Stockfish::Stockfish(const arquivos::path &executable, int milliseconds, int skill, bool trace) {
    validarTempoPensamento(milliseconds);
    validarDificuldade(skill);
    impl_ = std::make_unique<Impl>(arquivos::absolute(executable), milliseconds, skill, trace);
}

Stockfish::~Stockfish() = default;

void Stockfish::definirDificuldade(int nivel) {
    validarDificuldade(nivel);
    impl_->send("setoption name Skill Level value " + std::to_string(nivel));
    impl_->ready();
    impl_->skill = nivel;
}

int Stockfish::dificuldade() const {
    return impl_->skill;
}

void Stockfish::definirTempoPensamento(int tempoMs) {
    validarTempoPensamento(tempoMs);
    impl_->milliseconds = tempoMs;
}

int Stockfish::tempoPensamento() const {
    return impl_->milliseconds;
}

void Stockfish::novaPartida() {
    impl_->send("ucinewgame");
    impl_->ready();
}

std::string Stockfish::responder(const std::vector<std::string> &history,
                               std::vector<std::string> *melhores) {
    if (melhores) {
        melhores->clear();
    }
    impl_->send(std::string("setoption name MultiPV value ") + (melhores ? "3" : "1"));
    impl_->ready();
    std::string position = "position startpos";
    if (!history.empty()) {
        position += " moves";
    }
    for (const auto &move : history) {
        position += " " + move;
    }
    impl_->send(position);
    impl_->send("go movetime " + std::to_string(impl_->milliseconds));
    const auto line = impl_->waitFor("bestmove", impl_->milliseconds + 20000, melhores);
    const auto end = line.find(' ', 9);
    return line.substr(9, end == std::string::npos ? end : end - 9);
}

arquivos::path pastaDoExecutavel() {
#ifdef _WIN32
    std::vector<wchar_t> buffer(32768);
    const auto size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!size || size == buffer.size()) {
        throw std::runtime_error("Falha ao localizar executável do jogo.");
    }
    return arquivos::path(std::wstring(buffer.data(), size)).parent_path();
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        throw std::runtime_error("Falha ao localizar executável do jogo.");
    }
    return arquivos::weakly_canonical(buffer.data()).parent_path();
#else
    return arquivos::canonical("/proc/self/exe").parent_path();
#endif
}
