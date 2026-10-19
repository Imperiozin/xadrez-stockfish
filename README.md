# Xadrez com Stockfish

Jogo de xadrez em C++17 para terminal, com validação local de lances e comunicação
UCI com o Stockfish 17.1. O humano joga de brancas e o motor responde de pretas.

## Compilação e execução

Requisitos: Python 3.9+ e um compilador C++17 com suporte a threads.
No Windows, use MinGW/GCC; a compilação com MSVC está disponível pelo CMake.

```powershell
# Windows: compila, prepara o motor e executa os testes.
python tools/build.py --test
python tools/build.py --run
```

```sh
# Linux e macOS.
python3 tools/build.py --test
python3 tools/build.py --run
```

O repositório inclui os motores oficiais para Windows x64, Linux x64, macOS x64
e macOS ARM64, com suas licenças e fontes. O preparo copia e verifica o motor
da plataforma atual, permitindo compilar e jogar sem download, desde que Python
e o compilador já estejam instalados. Se os pacotes incluídos estiverem ausentes,
o preparo baixa o motor oficial na primeira execução e o reutiliza nas seguintes.
Para fornecer outro executável, use `python tools/build.py --engine CAMINHO`.
O compilador pode ser informado com `--compiler CAMINHO` ou pela variável `CXX`.

Os motores ficam em `vendor/stockfish/`; arquivos gerados pela compilação ficam
fora do Git. Para reconstruir os pacotes locais, execute
`python tools/bundle_engines.py` com acesso à internet. Distribuições desses
motores devem preservar também as licenças e os fontes fornecidos.

Com CMake 3.16+:

```sh
cmake -S . -B build/cmake
cmake --build build/cmake --config Release
ctest --test-dir build/cmake -C Release --output-on-failure
```

Também é possível abrir `XadrezStockfish.cbp` no Code::Blocks.

## Configuração durante a partida

Digite lances em UCI, como `e2e4` ou `e7e8q` para promoção.

| Comando | Efeito |
| --- | --- |
| `dificuldade 10` | Define o Skill Level entre 0 e 20; 20 é a força máxima. |
| `tempo 500` | Define o tempo de pensamento por lance entre 1 e 60000 ms. |
| `config` | Mostra a dificuldade e o tempo atuais. |
| `fen` | Mostra a posição em FEN. |
| `lances` | Lista os lances legais do humano. |
| `novo` | Reinicia a partida, preservando a dificuldade e o tempo. |
| `ajuda` | Mostra os comandos disponíveis. |
| `sair` | Encerra o jogo. |

Os valores iniciais são dificuldade 5 e tempo de 300 ms. Também podem ser
definidos na inicialização:

```powershell
.\build\windows\bin\xadrez.exe --skill 10 --time 500
```

Use `--trace` para acompanhar a comunicação UCI ou `--engine CAMINHO` para
escolher um motor já instalado. O tempo configura a busca do motor por lance;
a comunicação e a atualização do tabuleiro acrescentam algum tempo à resposta.

## API C++

```cpp
#include "jogo.hpp"

Jogo jogo("caminho/para/stockfish", 300, 5);
jogo.definirDificuldade(10);
jogo.definirTempoPensamento(500);

int nivel = jogo.dificuldade();
int tempoMs = jogo.tempoPensamento();
auto resposta = jogo.fazerMovimento("e2e4");

jogo.reiniciar();  // Mantém dificuldade 10 e tempo de 500 ms.
```

As mesmas quatro funções estão disponíveis diretamente em `Stockfish`.
`definirDificuldade` envia `setoption name Skill Level value N` e aguarda
`readyok`. `definirTempoPensamento` define o `go movetime N` da próxima busca.
As mudanças não alteram o tabuleiro nem o histórico.

Valores fora dos limites lançam `std::invalid_argument` e preservam a
configuração anterior. O construtor usa as mesmas validações. Skill Level
expressa dificuldade, sem correspondência fixa com uma classificação Elo.

Acesse cada instância por uma única thread de cada vez. Na interface gráfica,
aguarde o término de `fazerMovimento` antes de consultar ou alterar o jogo.
Lances inválidos retornam `nullptr`; falhas de comunicação lançam exceções.

## Validação

`python tools/build.py --test` compila com avisos habilitados e executa
`xadrez --self-test` com o motor real. O teste cobre limites das configurações,
comandos UCI, preservação do estado, entradas inválidas, respostas do motor e
reinício. Veja também [docs/VALIDACAO.md](docs/VALIDACAO.md).

## Licenças

Os fontes próprios usam a [licença MIT](LICENSE). As bibliotecas incluídas
mantêm suas licenças em `vendor/chess-library/LICENSE` e
`vendor/ghc-filesystem/LICENSE`. O Stockfish usa GPLv3; os pacotes incluídos em
`vendor/stockfish/` preservam a licença e os fontes oficiais. Consulte o arquivo
`Copying.txt` de cada plataforma.
