# Cenários e iluminação

O jogo escolhe o cenário pela dificuldade e troca o ambiente ao selecionar outra
opção no bloco sobre a mesa. A troca preserva a partida e o andamento da animação
do pet; apenas a animação do ambiente recomeça.
O nível padrão continua sendo Intermediário (`--skill 0`).

| Arquivo | Skybox | Dificuldade | Pet | Iluminação |
| --- | --- | --- | --- | --- |
| `1.cfg` | `assets/skybox/1` — arco-íris | Iniciante, −20 | Abelha | Dia claro, luz quente, ambiente azulado. |
| `2.cfg` | `assets/skybox/2` — jardim zen | Fácil, −10 | Panda | Luz difusa neutra com leve tom verde. |
| `3.cfg` | `assets/skybox/3` — pôr do sol | Intermediário, 0 | Raposa | Luz baixa e alaranjada. |
| `4.cfg` | `assets/skybox/4` — planeta com dois sóis | Difícil, 10 | Vaca | Sol normal e sol vermelho menor, móveis, com eclipses. |
| `5.cfg` | `assets/skybox/5` — bosque assombrado | Impossível, 20 | Gato | Pouca luz azulada, névoa e relâmpagos. |

Níveis intermediários da linha de comando usam a opção mais próxima; os pontos
médios (−15, −5, 5, 15) pertencem à opção superior. As faces mantêm os nomes
`right.png`, `left.png`, `top.png`, `bottom.png`, `front.png` e `back.png`,
necessários ao carregamento OpenGL. A numeração identifica a pasta.

## Configuração global

`assets/scenarios/global.cfg` é carregado automaticamente **antes** do cenário.
Ele centraliza materiais e ajustes de sombras iguais nos cinco ambientes;
esses parâmetros não são repetidos nos arquivos numerados. Um cenário
personalizado também herda o global e pode sobrescrever seus valores.

| Parâmetro global | Função |
| --- | --- |
| `diffuseStrength` | Resposta difusa dos materiais. |
| `specularStrength`, `woodSpecularStrength`, `shininess` | Força e concentração dos brilhos, incluindo a madeira. |
| `shadowAutoFit` | `1` ajusta automaticamente a projeção de cada luz; `0` usa os planos manuais. |
| `shadowRadius` | Raio do volume de sombras ao redor do tabuleiro; padrão 20. |
| `shadowFov`, `shadowNear`, `shadowFar` | Abertura em graus e planos de corte do modo manual. |
| `shadowBias`, `shadowMinBias`, `shadowNormalOffset` | Compensações contra autossombreamento; valores altos podem afastar sombras de contato. |

## Configuração de cada ambiente

| Parâmetro | Função |
| --- | --- |
| `skybox` | Pasta com as seis faces PNG. |
| `pet` | Identificador de um dos 24 modelos Kenney em `include/PetCatalog.h`. Cenários 1–5: `Bee`, `Panda`, `Fox`, `Cow`, `Cat`. Padrão: `Fox`. |
| `animation` | `static`, `binary` ou `storm`. |
| `lightPosition` | Posição XYZ da luz principal, voltada para o centro da cena. |
| `lightColor`, `lightIntensity` | Cor RGB e intensidade direta, independente da distância. |
| `ambientColor`, `ambientStrength` | Cor e intensidade nas áreas de sombra. |
| `skyBrightness` | Multiplicador do brilho da skybox. |
| `fogColor`, `fogDensity` | Cor e densidade da névoa; densidade zero a desativa. |

Os pets usam os modelos FBX e animações do pacote Cube Pets, por
[Kenney](https://kenney.nl), sob licença CC0. Para usar outro animal, altere `pet`
para um dos identificadores: `Beaver`, `Bee`, `Bunny`, `Cat`, `Caterpillar`,
`Chick`, `Cow`, `Crab`, `Deer`, `Dog`, `Elephant`, `Fish`, `Fox`, `Giraffe`,
`Hog`, `Koala`, `Lion`, `Monkey`, `Panda`, `Parrot`, `Penguin`, `Pig`, `Polar`, `Tiger`.

### Dois sóis — `4.cfg`

O sol normal e o vermelho são discos desenhados sobre a skybox e fontes de luz
independentes, cada um com seu mapa de sombras. A imagem do deserto permanece
como fundo. O movimento dos discos acompanha a direção da iluminação e das sombras.

- `secondaryLightColor`, `secondaryLightIntensity`: cor e intensidade do sol menor.
- `lightPosition`: direção inicial do sol principal; a órbita calcula as duas
  posições ao longo do tempo.
- `sunRadius`, `secondarySunRadius`: raios aparentes em radianos. O vermelho deve
  ser menor que o principal.
- `orbitPeriod`: duração, em segundos, de uma volta dos sóis no céu.
- `eclipsePeriod`: duração do ciclo relativo; há dois alinhamentos por ciclo.

Os sóis permanecem acima do horizonte e alternam qual fica à frente. O vermelho
oculta parte do maior ao passar à frente, e pode ser totalmente encoberto ao
passar atrás. A luz varia pela área de sobreposição dos discos; o ambiente e a
skybox escurecem junto. Com os períodos padrão, os primeiros eclipses ocorrem
aproximadamente aos 25 e 60 segundos.

### Tempestade — `5.cfg`

- `lightningInterval`: intervalo central entre eventos, em segundos.
- `lightningJitter`: variação dos instantes de início. Com intervalo 12 e variação
  4, os eventos ficam separados por 8 a 16 segundos.
- `lightningStrength`: intensidade da luz azulada dos relâmpagos.

Cada evento sorteia direção, altura e formato ramificado do raio. O clarão tem
três pulsos rápidos com espaçamento e força variáveis; ilumina peças, tabuleiro,
mesa, céu e névoa. Durante o clarão, a luz do relâmpago projeta sombras conforme
a direção sorteada, além das sombras da luz noturna. O mapa de sombras do raio
só é atualizado enquanto sua luz está ativa. Uma nova execução sorteia outra sequência,
sem depender da taxa de quadros.

## Editar e executar

Edite os arquivos de origem e execute `python tools/build.py --run` para copiar
os recursos e abrir o jogo. Também é possível editar as cópias em
`build/windows/bin/assets/scenarios/` e reabrir o executável, sem recompilar;
o próximo build sobrescreve essas cópias.

```powershell
.\build\windows\bin\xadrez.exe --skill 10
.\build\windows\bin\xadrez.exe --scenario assets/scenarios/5.cfg
```

`--scenario` escolhe apenas o ambiente inicial, sem mudar a força do motor.
Ao selecionar outra dificuldade, o cenário correspondente volta a ser aplicado.
Os caminhos são relativos ao executável; caminhos absolutos também são aceitos.
Cada linha contém uma chave e valores separados por espaços; comentários começam
com `#`.
