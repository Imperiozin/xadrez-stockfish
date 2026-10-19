# Validação da entrega

## Atualização: dificuldade e tempo de pensamento (18/10/2026)

Validação executada no Windows x64 com Python 3.14 e MinGW/GCC 15.2:

- `python tools/build.py --test`: compilação C++17 com avisos habilitados e
  autoteste aprovado com o Stockfish 17.1 real.
- `Stockfish` e `Jogo`: alteração e consulta de dificuldade e tempo, aceitação
  dos limites e rejeição de valores inválidos sem perder a configuração.
- Log UCI: mudança de Skill Level seguida de `isready` e uso do tempo atualizado
  em `go movetime` na próxima busca; entradas inválidas não enviam comandos.
- Preservação do FEN ao configurar, manutenção do histórico entre turnos e
  preservação das configurações ao reiniciar a partida.
- Terminal: comandos `config`, `dificuldade` e `tempo`, tratamento de argumentos
  ausentes, excedentes ou malformados e continuidade após erros de configuração.
- Execução a partir de outra pasta, resposta legal ao lance humano e reinício.
- CMake com MSVC 19.44: compilação Release e CTest aprovados (1/1). O MSVC
  emite avisos de conversão e condições constantes na biblioteca de terceiros
  `vendor/chess-library/chess.hpp`.
- Motores incluídos: hashes SHA-256, licenças e fontes conferidos nas quatro
  plataformas; cada binário tem menos de 100 MiB.

Os relatos abaixo correspondem à entrega anterior e registram o ambiente e os
limites de validação daquela versão.

Ambiente de execução disponível: Linux x64, GCC 13.3, Python 3.12.

## Verificações aprovadas

- Build direto usando `tools/build.py --test`, com `-Wall -Wextra -Wpedantic`.
- Configuração/build com CMake e execução do teste por CTest: 1/1 aprovado.
- Comunicação real com o Stockfish 17.1 incluído: handshake UCI, configurações,
  busca de resposta, vários turnos, reinício e encerramento do processo.
- Entradas inválidas (`e2e5`, `e7e5` na vez das brancas, casas inexistentes,
  entrada vazia e tentativa de incluir um comando UCI na entrada) retornam
  `nullptr`, preservando o FEN.
- A busca somente é solicitada para lances válidos, confirmado pelo log UCI.
- Uma resposta real foi conferida contra as jogadas legais da posição, e o FEN
  final foi comparado ao estado esperado após os dois lances.
- Execução a partir de outra pasta de trabalho: o motor continuou sendo localizado
  em relação ao executável do jogo.
- Build e self-test com chamadas HTTP bloqueadas, em uma nova pasta de saída com
  espaços no nome: os motores incluídos foram usados sem download.
- Casos de contrato exercitados com um processo UCI controlado: mate do humano
  sem resposta do motor, mate pelo motor, resposta ilegal do motor e encerramento
  inesperado do processo. Nas falhas, o estado local ficou intacto.
- XML do projeto Code::Blocks validado, incluindo os três targets.
- Os quatro binários tiveram seus hashes SHA-256 e formatos ELF/PE/Mach-O
  verificados; licenças e fontes fornecidos pelos pacotes oficiais estão incluídos.

## Limites desta validação

O IDE Code::Blocks não foi aberto neste ambiente. Os executáveis de Windows e
macOS estão incluídos, mas a compilação/execução do programa nessas plataformas
não foi testada aqui. O build e a partida foram testados em Linux x64.

O requisito offline pressupõe que compilador e Python já estejam instalados.
Os motores incluídos não cobrem todas as arquiteturas e distribuições; veja a
tabela no README. Não foi incluído OpenGL nesta demonstração.

Para repetir o teste principal: `python3 tools/build.py --test` ou, no Windows,
`compilar.bat --test`.

## Revisão de compatibilidade com MinGW/GCC 8.1

O log enviado mostrou falhas dentro de `bits/fs_path.h` da distribuição MinGW
8.1.0. Foi incluída a biblioteca MIT `ghc::filesystem` v1.5.14 para substituir
automaticamente esse componente em GCC anterior a 9. Compiladores recentes
continuam usando a biblioteca padrão. O aviso de redefinição de `NOMINMAX`
também foi corrigido com guardas de pré-processador.

Após essa mudança, a compilação e o self-test com Stockfish real passaram no
Linux tanto no caminho padrão quanto forçando o caminho de compatibilidade com
`-DXADREZ_USE_BUNDLED_FILESYSTEM`. A execução pelo Code::Blocks/MinGW 8.1 no
Windows continua dependendo de verificação na máquina do usuário.
