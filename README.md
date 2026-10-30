# Xadrez 3D com Stockfish

Uma única aplicação C++17/OpenGL: tabuleiro em um plano 3D, peças construídas
com primitivas e partidas contra Stockfish 17.1. Você joga com as brancas.
A aplicação abre a janela gráfica por padrão; a antiga interface de terminal
foi substituída. A integração UCI foi preservada.
Uma única aplicação C++17/OpenGL: tabuleiro em um plano 3D, peças construídas
com primitivas e partidas contra Stockfish 17.1. Você joga com as brancas.
A aplicação abre a janela gráfica por padrão; a antiga interface de terminal
foi substituída. A integração UCI foi preservada.

## Compilar e jogar

Requisitos comuns: Python 3.9+, compiladores C/C++17 e um driver OpenGL 3.3.
GLFW e o carregador GLAD estão incluídos como fontes em `vendor/glfw` e são
compilados localmente para cada sistema. Não é preciso instalar GLFW/GLEW,
baixar dependências gráficas ou manter pastas externas ao projeto.

### Windows x64

Instale MinGW/GCC e use:

```powershell
python tools/build.py --run
# Se o compilador estiver em outra pasta:
python tools/build.py --run --compiler C:/MinGW/bin/g++.exe
```

O script procura o compilador no PATH e, no Windows, também em
`C:/MinGW/bin/g++.exe`. Esse é apenas um caminho alternativo para o compilador,
não uma dependência do OpenGL. O executável fica em `build/windows/bin/xadrez.exe`.
O script procura o compilador no PATH e, no Windows, também em
`C:/MinGW/bin/g++.exe`. Esse é apenas um caminho alternativo para o compilador,
não uma dependência do OpenGL. O executável fica em `build/windows/bin/xadrez.exe`.

### Linux

Além de Python e GCC/Clang, instale os headers do sistema de janelas X11.
Além de Python e GCC/Clang, instale os headers do sistema de janelas X11.
No Debian/Ubuntu:

```sh
sudo apt install build-essential python3 xorg-dev
python3 tools/build.py --run
```

O build Python usa X11 (também funciona em sessões Wayland com XWayland).
O build Python usa X11 (também funciona em sessões Wayland com XWayland).
O CMake permite habilitar Wayland nativo com `-DGLFW_BUILD_WAYLAND=ON`;
nesse caso são necessários também `libwayland-dev`, `libxkbcommon-dev`,
nesse caso são necessários também `libwayland-dev`, `libxkbcommon-dev`,
`wayland-scanner` e `pkg-config`.

### macOS Intel e Apple Silicon

Instale as ferramentas de desenvolvimento da Apple (`xcode-select --install`)
e Python 3.9+. GLFW usa os frameworks Cocoa, IOKit e QuartzCore do SDK:

```sh
python3 tools/build.py --run --compiler clang++ --cc clang
```

A janela usa contexto core 3.3 e forward compatibility no macOS.
Não há dependência de DLLs Windows ou de bibliotecas gráficas Homebrew.
Não há dependência de DLLs Windows ou de bibliotecas gráficas Homebrew.

### CMake e Code::Blocks

O mesmo alvo `xadrez` pode ser compilado com CMake 3.16+:

```sh
cmake -S . -B build/cmake
cmake --build build/cmake --config Release
```

No Windows com MinGW, adicione seu `bin` ao PATH e configure com
`-G "MinGW Makefiles"`. MSVC também é suportado pelo CMake.
`-G "MinGW Makefiles"`. MSVC também é suportado pelo CMake.
No Code::Blocks, abra somente `XadrezStockfish.cbp` e selecione o alvo da
plataforma (`Windows`, `Linux` ou `macOS`). O projeto delega a compilação ao
mesmo `tools/build.py`; configure `CXX`/`CC` no ambiente para escolher outro
compilador. Não há um segundo projeto gráfico.
compilador. Não há um segundo projeto gráfico.

## Controles

| Ação | Controle |
| Ação | Controle |
| --- | --- |
| Selecionar peça / jogar | Clique esquerdo na peça branca, depois no destino verde |
| Cancelar seleção / promoção | Botão direito (ou clique fora do tabuleiro para seleção) |
| Girar / inclinar câmera | Arraste com o botão do meio (scroll pressionado) |
| Deslocar câmera | Setas; Shift + cima/baixo para altura |
| Limites da câmera | X/Z: -24 a 24; Y: 2,5 a 26 |
| Aproximar / afastar | Roda do mouse |
| Restaurar câmera | Home |
| Restaurar câmera | Home |
| Nova partida | N |
| Aumentar / reduzir dificuldade | + / - (ou teclado numérico) |
| Aumentar / reduzir tempo | Page Up / Page Down (100 ms por ajuste) |
| Escolher promoção | Clique em um dos quatro quadros: rainha, torre, bispo, cavalo |
| Sair / cancelar escolha de promoção | Esc |

A seleção tem contorno dourado e realce sob o cursor. Capturas legais exibem
um anel vermelho na base da peça adversária (incluindo en passant). O destino
do roque tem uma borda tracejada verde. As quatro opções de promoção giram
na mesma direção e velocidade e recebem destaque ao passar o mouse.

O título da janela informa o turno, pensamento do motor, resultado, dificuldade
e tempo. Durante a busca, a câmera e a renderização continuam funcionando;
comandos que alteram a partida aguardam a resposta da engine.
Roque, en passant, capturas, promoções e término de partida são validados
pela biblioteca de xadrez e refletidos na cena a partir da posição FEN.
Roque, en passant, capturas, promoções e término de partida são validados
pela biblioteca de xadrez e refletidos na cena a partir da posição FEN.

As configurações iniciais são dificuldade 0 e 300 ms por lance:
As configurações iniciais são dificuldade 0 e 300 ms por lance:

```sh
./build/linux/bin/xadrez --skill 10 --time 500
```

De 0 a 20, a dificuldade usa o Skill Level nativo do Stockfish. De -1 a -20,
o motor fica no Skill Level 0 e o jogo tem 5% de chance por nível negativo de
o motor fica no Skill Level 0 e o jogo tem 5% de chance por nível negativo de
substituir sua escolha por outra jogada legal sorteada (50% no -10 e 100% no
-20, quando existem alternativas). O MultiPV consulta três candidatos e o
sorteio exclui essas opções e a resposta do motor, preservando pelo menos uma
alternativa quando há poucos lances legais. Pode ocasionalmente escolher uma
boa jogada; os níveis negativos não correspondem a uma escala Elo.
Use `--skill -20` para iniciar no modo mais fácil.
-20, quando existem alternativas). O MultiPV consulta três candidatos e o
sorteio exclui essas opções e a resposta do motor, preservando pelo menos uma
alternativa quando há poucos lances legais. Pode ocasionalmente escolher uma
boa jogada; os níveis negativos não correspondem a uma escala Elo.
Use `--skill -20` para iniciar no modo mais fácil.

Os pacotes oficiais do Stockfish para Windows x64, Linux x64, macOS x64 e
macOS ARM64 já estão em `vendor/stockfish`, com fontes e licenças. Para outra
arquitetura, forneça um motor compatível: `python3 tools/build.py --engine CAMINHO`.
O executável encontra o motor ao lado dele em `engines`, independentemente
da pasta de trabalho. Para distribuir, mantenha essa pasta junto do executável.
macOS ARM64 já estão em `vendor/stockfish`, com fontes e licenças. Para outra
arquitetura, forneça um motor compatível: `python3 tools/build.py --engine CAMINHO`.
O executável encontra o motor ao lado dele em `engines`, independentemente
da pasta de trabalho. Para distribuir, mantenha essa pasta junto do executável.

## Organização orientada a objetos

A estrutura segue as responsabilidades do projeto de referência `GLFW_Cubo_OO`:
A estrutura segue as responsabilidades do projeto de referência `GLFW_Cubo_OO`:

| Classe / pasta | Responsabilidade |
| --- | --- |
| `Application` | Janela GLFW, eventos, câmera, seleção e execução assíncrona da engine |
| `Object` | Posição, rotação, escala e transformação herdada do objeto pai |
| `Application` | Janela GLFW, eventos, câmera, seleção e execução assíncrona da engine |
| `Object` | Posição, rotação, escala e transformação herdada do objeto pai |
| `Mesh` | Geometria e recursos de GPU (VAO/VBO) |
| `Shader` | Compilação GLSL, iluminação e matrizes |
| `Shader` | Compilação GLSL, iluminação e matrizes |
| `Primitives` | Malhas compartilhadas de cubo, cilindro, cone e esfera |
| `Board` | Casas, seleção, destinos legais e objetos das peças |
| `Piece` | Base comum, material, cor e criação das peças |
| `Pawn`, `Rook`, `Knight`, `Bishop`, `Queen`, `King` | Composição e desenho próprios de cada tipo de peça |
| `Jogo` / `Stockfish` | Regras e comunicação UCI, independentes da renderização |
| `Board` | Casas, seleção, destinos legais e objetos das peças |
| `Piece` | Base comum, material, cor e criação das peças |
| `Pawn`, `Rook`, `Knight`, `Bishop`, `Queen`, `King` | Composição e desenho próprios de cada tipo de peça |
| `Jogo` / `Stockfish` | Regras e comunicação UCI, independentes da renderização |

Headers ficam em `include/`; renderização em `src/graphics/`; cada tipo de
peça tem seu header e implementação em `src/pieces/`. Para ajustar uma peça,
altere as posições e escalas das primitivas no seu método `draw`.
Cada peça do tabuleiro é um objeto, enquanto as malhas são compartilhadas.
`Math.h` contém apenas os vetores e matrizes usados pela cena.

O worker consulta `Jogo` enquanto a interface desenha uma cópia da posição.
Não há leituras simultâneas do estado do jogo durante a busca. A resposta
atualiza a cena; em caso de erro, a posição original é restaurada.

## Licenças
## Licenças

Código próprio: [MIT](LICENSE). As dependências preservam seus avisos:
Código próprio: [MIT](LICENSE). As dependências preservam seus avisos:
`vendor/glfw/LICENSE.md`, headers GLAD em `vendor/glfw/deps/glad`,
`vendor/chess-library/LICENSE`, `vendor/ghc-filesystem/LICENSE` e GPLv3 do
Stockfish (fontes e `Copying.txt` em cada pacote). A demonstração gráfica
Stockfish (fontes e `Copying.txt` em cada pacote). A demonstração gráfica
anterior foi arquivada localmente em `.cache/legacy-opengl`, fora do build.

## Sombras

A cena utiliza shadow mapping em duas passagens, seguindo a referência do
[LearnOpenGL](https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping).
O mapa de profundidade de 2048 × 2048 é atualizado a cada quadro e inclui
peças, tabuleiro e chão. A luz em `Application::render` usa uma projeção
perspectiva alinhada à iluminação pontual existente. O shader aplica PCF 3 × 3
para suavizar as bordas e bias dependente da inclinação para reduzir artefatos.
A sombra afeta as componentes difusa e especular, preservando a luz ambiente.
As prévias de promoção são iluminadas separadamente, sem consultar esse mapa.
