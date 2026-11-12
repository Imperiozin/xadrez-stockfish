# Xadrez 3D com Stockfish

Jogo de xadrez em **C++17 e OpenGL 3.3**, com peças importadas com Assimp, câmera livre e partidas contra o **Stockfish 17.1**. O jogador controla as peças brancas; o motor joga com as pretas por meio do protocolo UCI.

A cena reúne tabuleiro, mesa, céu panorâmico, iluminação, sombras e névoa. A interface usa Dear ImGui para iniciar partidas, escolher a dificuldade e acompanhar o estado do jogo.

## Recursos

- Seleção pelo mouse, indicação de destinos legais e destaque de capturas e roques.
- Movimentos animados para os dois lados, com salto dos cavalos e movimento simultâneo do rei e da torre no roque.
- Regras validadas pela `chess-library`, incluindo roque, en passant e promoção para rainha, torre, bispo ou cavalo.
- Detecção de xeque-mate e empates por afogamento, material insuficiente, tripla repetição e regra dos 50 lances.
- Cinco opções de dificuldade na interface e níveis de −20 a 20 pela linha de comando.
- Busca do Stockfish em segundo plano, mantendo a câmera e a renderização ativas.
- Cenários configuráveis em arquivos de texto, sem recompilar o código C++.

## Requisitos

- Python **3.9 ou superior**.
- Compiladores C e C++ com suporte a **C++17**.
- Ambiente gráfico e driver com suporte a **OpenGL 3.3 Core**.
- Internet na primeira compilação para baixar as fontes do Assimp 5.4.3. O pacote fica em `.cache/` e pode ser reutilizado sem internet.
- CMake **3.22 ou superior** é opcional, necessário apenas se escolher a alternativa CMake abaixo.

GLFW, GLAD, Dear ImGui, `chess-library` e `ghc-filesystem` estão incluídos em `vendor/`. Os pacotes do Stockfish para Windows x64, Linux x64, macOS Intel e macOS Apple Silicon também estão incluídos, com fontes e licenças. Com esses arquivos presentes, a preparação do motor dispensa download.

Execute os comandos abaixo na raiz do projeto.

## Compilar e executar

### Windows x64

Instale Python e MinGW/GCC, disponibilizando `g++` e `gcc` no `PATH`:

```powershell
python tools/build.py --run
```

Para informar os compiladores explicitamente:

```powershell
python tools/build.py --run --compiler C:/MinGW/bin/g++.exe --cc C:/MinGW/bin/gcc.exe
```

O script também procura `g++` em `C:/MinGW/bin/g++.exe` quando não encontra o compilador solicitado. A saída padrão é `build/windows/bin/xadrez.exe`.

### Linux x64

No Debian ou Ubuntu, instale as ferramentas de compilação e os cabeçalhos do X11:

```sh
sudo apt install build-essential python3 xorg-dev
python3 tools/build.py --run
```

A saída padrão é `build/linux/bin/xadrez`. O script usa X11; em sessões Wayland, precisa de XWayland.

### macOS Intel e Apple Silicon

Instale Python e as ferramentas de desenvolvimento da Apple:

```sh
xcode-select --install
python3 tools/build.py --run --compiler clang++ --cc clang
```

A saída padrão é `build/darwin/bin/xadrez`. A compilação usa os frameworks Cocoa, IOKit e QuartzCore do SDK da Apple.

### Opções do script

| Opção | Função |
| --- | --- |
| `--run` | Abre o jogo após compilar. |
| `--compiler CAMINHO` | Escolhe o compilador C++; também aceita a variável `CXX`. |
| `--cc CAMINHO` | Escolhe o compilador C usado pelo GLFW; também aceita a variável `CC`. |
| `--output PASTA` | Define a pasta de saída do executável e dos recursos. |
| `--engine CAMINHO` | Copia e verifica um executável local do Stockfish. |

O script chama os compiladores diretamente, sem CMake. Ele baixa e verifica o Assimp 5.4.3, compila o importador FBX e a zlib em uma biblioteca estática e a vincula ao jogo. A primeira compilação demora mais; os objetos do Assimp e do GLFW ficam em cache para as próximas compilações. Não é necessário instalar Assimp nem copiar uma DLL.

Os scripts `compilar.bat` e `compilar.sh` encaminham os argumentos para `tools/build.py`. Por exemplo, `./compilar.bat --run` no PowerShell ou `sh compilar.sh --run` no Linux/macOS.

### CMake

Alternativa opcional ao script Python. Requer CMake 3.22 ou superior e aceita um pacote Assimp 5 instalado (`assimp_DIR`/`CMAKE_PREFIX_PATH`); caso não o encontre, baixa as fontes automaticamente.

```sh
cmake -S . -B build/cmake
cmake --build build/cmake --config Release
```

O CMake também compila a verificação matemática do bloco de notas. Execute-a com `ctest --test-dir build/cmake -C Release --output-on-failure`.

O executável e seus recursos ficam em `build/cmake/bin/`, inclusive nos geradores com múltiplas configurações. No Windows, execute `./build/cmake/bin/xadrez.exe`; no Linux/macOS, `./build/cmake/bin/xadrez`.

Para MinGW no Windows, use `-G "MinGW Makefiles"` na configuração e mantenha suas ferramentas no `PATH`. Para MSVC, use um ambiente de desenvolvimento do Visual Studio.

Um motor alternativo pode ser definido na configuração com `-DSTOCKFISH_LOCAL="CAMINHO_DO_STOCKFISH"`. No Linux, o backend Wayland pode ser habilitado com `-DGLFW_BUILD_WAYLAND=ON`, desde que suas dependências de desenvolvimento estejam instaladas.

### Code::Blocks

Abra [XadrezStockfish.cbp](XadrezStockfish.cbp) e escolha o alvo `Windows`, `Linux` ou `macOS`. O projeto chama o mesmo script Python e grava os arquivos em `bin/Windows`, `bin/Linux` ou `bin/macOS`. Use `CXX` e `CC` no ambiente para selecionar os compiladores.

## Como jogar

1. Clique em uma peça branca para selecioná-la.
2. Clique em um destino verde para realizar o lance.
3. Aguarde a resposta do Stockfish e faça sua próxima jogada.

A peça selecionada se eleva suavemente e recebe um contorno dourado. Ao cancelar ou trocar a seleção, ela desce até o tabuleiro. Capturas legais são marcadas com um anel vermelho, inclusive no en passant; o destino do roque tem uma borda verde tracejada. Na promoção, clique em uma das quatro prévias de peças.

Os lances do jogador e do Stockfish são animados em sequência. Aguarde a peça chegar ao destino antes de selecionar a próxima; a câmera continua disponível durante o movimento. Capturas e promoções são concluídas ao final da animação.

| Ação | Controle |
| --- | --- |
| Selecionar peça ou destino | Botão esquerdo do mouse. |
| Cancelar seleção ou promoção | Botão direito do mouse. |
| Cancelar seleção | Clique fora do tabuleiro. |
| Girar e inclinar a câmera | Arraste com o botão do meio pressionado. |
| Deslocar a câmera no plano horizontal | Setas do teclado. |
| Subir ou descer a câmera | `Shift` + seta para cima ou para baixo. |
| Aproximar ou afastar | Roda do mouse. |
| Restaurar a câmera inicial | `Home`. |
| Iniciar outra partida | Linha **Nova partida** no bloco sobre a mesa ou botão no resultado final. |
| Alterar a dificuldade | Clique diretamente na linha desejada na folha do bloco. |
| Cancelar promoção; fora dela, sair | `Esc`. |

Durante a busca do motor, os controles de nova partida e dificuldade ficam desabilitados. A câmera continua disponível. Reiniciar a partida preserva a dificuldade e o tempo de pensamento escolhidos.

O bloco usa o modelo `assets/objects/NotepadwithRingbindings.fbx`. As pautas e a margem cobrem a folha de cima a baixo, acompanhando sua curvatura. Os textos e as marcações são desenhados com Dear ImGui em uma textura aplicada diretamente à malha da folha, com a mesma iluminação e sem aproximar a câmera. A fonte Schoolbell está incluída em `assets/fonts/`, com sua licença, e pode ser trocada no carregamento de `Notepad.cpp`. As texturas externas citadas pelo FBX não são necessárias. A rotação com o botão do meio e o zoom continuam disponíveis com o mouse sobre o bloco.

Para posicionar o conjunto, altere `pad->position`, `pad->rotation` e `pad->scale` na inicialização de `Application.cpp`. As rotações usam radianos. O modelo, a interface e as áreas clicáveis compartilham essa transformação e também herdam a transformação da mesa. Escalas não uniformes são aceitas; uma escala nula desabilita a interação. A parte curva da folha e a espiral podem ocultar os controles e bloquear cliques.

## Dificuldade e opções de execução

Por padrão, o jogo inicia no nível **0**, com **300 ms de pensamento por lance**.

| Opção na interface | Nível |
| --- | ---: |
| Iniciante | −20 |
| Fácil | −10 |
| Intermediário | 0 |
| Difícil | 10 |
| Impossível | 20 |

De 0 a 20, o valor corresponde ao `Skill Level` do Stockfish. Nos níveis negativos, o motor permanece no nível 0 e cada ponto negativo acrescenta 5% de chance de substituir sua resposta por outra jogada legal: 50% no −10 e 100% no −20, quando há alternativas. O sorteio procura excluir a resposta do motor e os candidatos obtidos por MultiPV, preservando uma alternativa quando há poucos lances. Esses níveis não representam uma classificação Elo nem garantem uma jogada ruim.

O tempo de pensamento é configurado ao iniciar o executável:

```powershell
# Windows: nível mais fácil, com 500 ms por lance.
./build/windows/bin/xadrez.exe --skill -20 --time 500
```

```sh
# Linux: nível 10, com 1 segundo por lance.
./build/linux/bin/xadrez --skill 10 --time 1000
```

| Argumento do jogo | Valores e comportamento |
| --- | --- |
| `--skill NIVEL` | Inteiro de −20 a 20. Padrão: `0`. |
| `--time MS` | Inteiro de 1 a 60000, em milissegundos por lance. Padrão: `300`. |
| `--engine CAMINHO` | Executável do Stockfish. Por padrão, usa `engines/stockfish` ou `engines/stockfish.exe` ao lado do jogo. |
| `--scenario CAMINHO` | Arquivo de cenário. Padrão: `assets/scenarios/sunset.cfg`. |
| `--trace` | Exibe a comunicação UCI no terminal para diagnóstico. |
| `--help` | Mostra os argumentos disponíveis e encerra. |

Passe esses argumentos diretamente ao executável. A opção `--run` do script de compilação abre o jogo com os valores padrão.

## Cenários e renderização

O cenário padrão está em [assets/scenarios/sunset.cfg](assets/scenarios/sunset.cfg). Ele define a skybox, a posição e a cor da luz, os materiais, a névoa e os parâmetros das sombras.

Para criar um cenário, copie esse arquivo, ajuste os valores e compile novamente para copiar os recursos. Depois, selecione a configuração ao executar:

```powershell
./build/windows/bin/xadrez.exe --scenario assets/scenarios/outro.cfg
```

Os caminhos de cenário são relativos à pasta do executável; caminhos absolutos também são aceitos. Para experimentar sem recompilar, edite a cópia na pasta de saída e reabra o jogo. A próxima compilação sobrescreve essa cópia.

A descrição de cada parâmetro está em [assets/scenarios/README.md](assets/scenarios/README.md). Os shaders ficam em `shaders/`: a cena usa iluminação Blinn–Phong, madeira procedural e sombras em duas passagens, com mapa de profundidade de 4096 × 4096 e suavização PCF 3 × 3.

## Organização do código

| Local | Responsabilidade |
| --- | --- |
| `src/main.cpp` | Argumentos de execução e inicialização do jogo. |
| `src/jogo.cpp` e `src/jogo.hpp` | Regras, lances legais, histórico e dificuldade. |
| `src/stockfish.cpp` e `src/stockfish.hpp` | Processo do Stockfish e comunicação UCI. |
| `src/graphics/Application.cpp` | Janela, eventos, câmera, seleção, renderização e busca assíncrona. |
| `src/graphics/ApplicationUI.cpp` | Estado da partida e resultado final em Dear ImGui. |
| `src/graphics/Notepad.cpp` | Bloco 3D, textura da interface e seleção das linhas na folha. |
| `src/graphics/` | Tabuleiro, mesa, skybox, cenários, malhas, objetos e shaders. |
| `src/pieces/` | Associação de cada tipo de peça ao modelo e desenho. |
| `include/` | Cabeçalhos da camada gráfica e operações matemáticas em `Math3D.h`. |
| `tests/notepad_math.cpp` | Verificação de cliques com rotação, escala e transformações herdadas. |
| `shaders/` | Programas GLSL da cena, das sombras e da skybox. |
| `assets/` | Modelos das peças, configurações de cenário e imagens da skybox. |
| `tools/` | Compilação e preparação dos pacotes do motor. |
| `vendor/` | Dependências e pacotes do Stockfish. |

As peças usam os seis modelos FBX de `assets/objects/pieces/`, importados pelo Assimp com triangulação, normais e transformações dos nós aplicadas. Cada tipo compartilha uma malha na GPU entre os dois lados e as prévias de promoção. Os modelos são centralizados, apoiados no tabuleiro e ajustados proporcionalmente para caber nas casas; as cores continuam definidas pelo jogo. Falhas de importação exibem o caminho do modelo e o diagnóstico do Assimp.

A camada gráfica sincroniza o tabuleiro pela posição FEN. Enquanto o Stockfish calcula, a interface desenha uma cópia da posição e a tarefa em segundo plano acessa o jogo; se o motor falhar, o lance não é confirmado e a cena recupera a posição anterior.

## Arquivos para execução

Mantenha o executável junto das pastas `engines/`, `shaders/` e `assets/` geradas pela compilação. Os recursos padrão são localizados a partir da pasta do executável, permitindo abrir o jogo a partir de outro diretório.

Para usar um Stockfish próprio, informe `--engine` ao script de compilação ou ao jogo. A preparação verifica a resposta UCI do motor. Se o pacote correspondente à plataforma não estiver em `vendor/stockfish/`, o preparador pode baixar a versão 17.1; para plataformas sem pacote automático, forneça um executável compatível.

## Licenças

O código próprio está sob a [licença MIT](LICENSE). As dependências mantêm suas licenças e avisos nos respectivos diretórios:

- [Assimp](vendor/Assimp-LICENSE.txt) (BSD de 3 cláusulas).
- [GLFW](vendor/glfw/LICENSE.md).
- GLAD: avisos nos arquivos em `vendor/glfw/deps/glad/`.
- [Dear ImGui](vendor/imgui/LICENSE.txt).
- Fonte Schoolbell, usada no bloco de notas: [Apache License 2.0](assets/fonts/LICENSE-Schoolbell.txt).
- Fonte Playwrite Brasil, incluída como recurso: [SIL Open Font License 1.1](assets/fonts/OFL-PlaywriteBR.txt).
- [chess-library](vendor/chess-library/LICENSE).
- [ghc-filesystem](vendor/ghc-filesystem/LICENSE).
- Stockfish: GPLv3, com fontes e `Copying.txt` em cada pacote de `vendor/stockfish/`.
