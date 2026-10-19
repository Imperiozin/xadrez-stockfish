#pragma once

// GCC/MinGW 8 tem uma implementacao incompleta de std::filesystem no Windows.
// Usa uma implementacao equivalente, ja incluida, sem alterar os headers do GCC.
// A macro permite exercitar a compatibilidade tambem com compiladores recentes.
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#if defined(XADREZ_USE_BUNDLED_FILESYSTEM) || \
    (defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 9)
#include "../vendor/ghc-filesystem/filesystem.hpp"
namespace arquivos = ghc::filesystem;
#else
#include <filesystem>
namespace arquivos = std::filesystem;
#endif
