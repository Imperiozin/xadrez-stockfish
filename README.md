# Xadrez 3D com Stockfish

Uma ?nica aplica??o C++17/OpenGL: tabuleiro em um plano 3D, pe?as constru?das
com primitivas e partidas contra Stockfish 17.1. Voc? joga com as brancas.
A aplica??o abre a janela gr?fica por padr?o; a antiga interface de terminal
foi substitu?da. A integra??o UCI foi preservada.

## Compilar e jogar

Requisitos comuns: Python 3.9+, compiladores C/C++17 e um driver OpenGL 3.3.
GLFW e o carregador GLAD est?o inclu?dos como fontes em `vendor/glfw` e s?o
compilados localmente para cada sistema. N?o ? preciso instalar GLFW/GLEW,
baixar depend?ncias gr?ficas ou manter pastas externas ao projeto.

### Windows x64

Instale MinGW/GCC e use:

```powershell
python tools/build.py --run
# Se o compilador estiver em outra pasta:
python tools/build.py --run --compiler C:/MinGW/bin/g++.exe
```

O script procura o compilador no PATH e, no Windows, tamb?m em
`C:/MinGW/bin/g++.exe`. Esse ? apenas um caminho alternativo para o compilador,
n?o uma depend?ncia do OpenGL. O execut?vel fica em `build/windows/bin/xadrez.exe`.

### Linux

Al?m de Python e GCC/Clang, instale os headers do sistema de janelas X11.
No Debian/Ubuntu:

```sh
sudo apt install build-essential python3 xorg-dev
python3 tools/build.py --run
```

O build Python usa X11 (tamb?m funciona em sess?es Wayland com XWayland).
O CMake permite habilitar Wayland nativo com `-DGLFW_BUILD_WAYLAND=ON`;
nesse caso s?o necess?rios tamb?m `libwayland-dev`, `libxkbcommon-dev`,
`wayland-scanner` e `pkg-config`.

### macOS Intel e Apple Silicon

Instale as ferramentas de desenvolvimento da Apple (`xcode-select --install`)
e Python 3.9+. GLFW usa os frameworks Cocoa, IOKit e QuartzCore do SDK:

```sh
python3 tools/build.py --run --compiler clang++ --cc clang
```

A janela usa contexto core 3.3 e forward compatibility no macOS.
N?o h? depend?ncia de DLLs Windows ou de bibliotecas gr?ficas Homebrew.

### CMake e Code::Blocks

O mesmo alvo `xadrez` pode ser compilado com CMake 3.16+:

```sh
cmake -S . -B build/cmake
cmake --build build/cmake --config Release
```

No Windows com MinGW, adicione seu `bin` ao PATH e configure com
`-G "MinGW Makefiles"`. MSVC tamb?m ? suportado pelo CMake.
No Code::Blocks, abra somente `XadrezStockfish.cbp` e selecione o alvo da
plataforma (`Windows`, `Linux` ou `macOS`). O projeto delega a compila??o ao
mesmo `tools/build.py`; configure `CXX`/`CC` no ambiente para escolher outro
compilador. N?o h? um segundo projeto gr?fico.

## Controles

| A??o | Controle |
| --- | --- |
| Selecionar pe?a / jogar | Clique esquerdo na pe?a branca, depois no destino verde |
| Cancelar sele??o | Clique fora do tabuleiro |
| Girar / inclinar c?mera | Arraste com o bot?o direito |
| Aproximar / afastar | Roda do mouse |
| Restaurar c?mera | Home |
| Nova partida | N |
| Aumentar / reduzir dificuldade | Seta para cima / para baixo (-20 a 20) |
| Aumentar / reduzir tempo | Page Up / Page Down (100 ms por ajuste) |
| Escolher promo??o | Q: rainha, R: torre, B: bispo, C: cavalo |
| Sair / cancelar escolha de promo??o | Esc |

O t?tulo da janela informa o turno, pensamento do motor, resultado, dificuldade
e tempo. Durante a busca, a c?mera e a renderiza??o continuam funcionando;
comandos que alteram a partida aguardam a resposta da engine.
Roque, en passant, capturas, promo??es e t?rmino de partida s?o validados
pela biblioteca de xadrez e refletidos na cena a partir da posi??o FEN.

As configura??es iniciais s?o dificuldade 0 e 300 ms por lance:

```sh
./build/linux/bin/xadrez --skill 10 --time 500
```

De 0 a 20, a dificuldade usa o Skill Level nativo do Stockfish. De -1 a -20,
o motor fica no Skill Level 0 e o jogo tem 5% de chance por nivel negativo de
substituir sua escolha por outra jogada legal sorteada (50% no -10 e 100% no
-20, quando existem alternativas). O MultiPV consulta tres candidatos e o
sorteio exclui essas opcoes e a resposta do motor, preservando pelo menos uma
alternativa quando ha poucos lances legais. Pode ocasionalmente escolher uma
boa jogada; os niveis negativos nao correspondem a uma escala Elo.
Use `--skill -20` para iniciar no modo mais facil.

Os pacotes oficiais do Stockfish para Windows x64, Linux x64, macOS x64 e
macOS ARM64 j? est?o em `vendor/stockfish`, com fontes e licen?as. Para outra
arquitetura, forne?a um motor compat?vel: `python3 tools/build.py --engine CAMINHO`.
O execut?vel encontra o motor ao lado dele em `engines`, independentemente
da pasta de trabalho. Para distribuir, mantenha essa pasta junto do execut?vel.

## Organiza??o orientada a objetos

A estrutura segue as responsabilidades do projeto de refer?ncia `GLFW_Cubo_OO`:

| Classe / pasta | Responsabilidade |
| --- | --- |
| `Application` | Janela GLFW, eventos, c?mera, sele??o e execu??o ass?ncrona da engine |
| `Object` | Posi??o, rota??o, escala e transforma??o herdada do objeto pai |
| `Mesh` | Geometria e recursos de GPU (VAO/VBO) |
| `Shader` | Compila??o GLSL, ilumina??o e matrizes |
| `Primitives` | Malhas compartilhadas de cubo, cilindro, cone e esfera |
| `Board` | Casas, sele??o, destinos legais e objetos das pe?as |
| `Piece` | Base comum, material, cor e cria??o das pe?as |
| `Pawn`, `Rook`, `Knight`, `Bishop`, `Queen`, `King` | Composi??o e desenho pr?prios de cada tipo de pe?a |
| `Jogo` / `Stockfish` | Regras e comunica??o UCI, independentes da renderiza??o |

Headers ficam em `include/`; renderiza??o em `src/graphics/`; cada tipo de
pe?a tem seu header e implementa??o em `src/pieces/`. Para ajustar uma pe?a,
altere as posi??es e escalas das primitivas no seu m?todo `draw`.
Cada pe?a do tabuleiro ? um objeto, enquanto as malhas s?o compartilhadas.
`Math.h` cont?m apenas os vetores e matrizes usados pela cena.

O worker consulta `Jogo` enquanto a interface desenha uma c?pia da posi??o.
N?o h? leituras simult?neas do estado do jogo durante a busca. A resposta
atualiza a cena; em caso de erro, a posi??o original ? restaurada.

## Licen?as

C?digo pr?prio: [MIT](LICENSE). As depend?ncias preservam seus avisos:
`vendor/glfw/LICENSE.md`, headers GLAD em `vendor/glfw/deps/glad`,
`vendor/chess-library/LICENSE`, `vendor/ghc-filesystem/LICENSE` e GPLv3 do
Stockfish (fontes e `Copying.txt` em cada pacote). A demonstra??o gr?fica
anterior foi arquivada localmente em `.cache/legacy-opengl`, fora do build.
