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

## Luzes locais: lâmpadas, lanternas e velas

Além das duas luzes do ambiente, a cena aceita até **16 luzes locais ativas**.
Elas iluminam automaticamente mesa, peças, tabuleiro, bloco e modelos que usam
`scene.frag`, com a mesma resposta difusa e especular dos materiais existentes.
Nenhuma luz local é adicionada por padrão.

Adicione uma ou mais linhas ao cenário desejado, ou a `global.cfg` para aplicar
a todos os cenários. As luzes do cenário são somadas às globais; `clearLocalLights`
remove as anteriores e permite substituir a lista herdada. A troca de cenário
recarrega essa lista.

```text
# Lâmpada: posição XYZ, cor RGB, intensidade, alcance.
pointLight 5 3 2  1 0.85 0.65  12 14

# Lanterna: posição XYZ, direção XYZ, cor RGB, intensidade, alcance,
# semiângulo interno e externo em graus.
spotLight -5 4 3  0.6 -1 -0.4  1 0.95 0.85  25 16  15 25

# Vela: posição XYZ, cor RGB, intensidade, alcance,
# amplitude da oscilação (0 a 1) e velocidade (> 0).
candleLight 5 0.8 -4  1 0.55 0.18  3 6  0.25 8
```

Posição e alcance usam as unidades do mundo. A intensidade é um multiplicador
artístico, não uma medida em watts. A contribuição cai conforme
`(1 - (distância / alcance)²)² / (1 + distância²)` dentro do alcance e chega a
zero suavemente no limite. A direção da lanterna aponta **da fonte para o alvo**;
não precisa estar normalizada. Os semiângulos devem satisfazer
`0 <= interno < externo < 90`; entre eles, a borda do cone é suavizada.

Na vela, a intensidade varia suavemente entre `intensidade × (1 - amplitude)` e
`intensidade × (1 + amplitude)`, com alvos pseudoaleatórios interpolados ao longo
do tempo. A intensidade configurada é o valor base; os picos podem ultrapassá-la.
Amplitude zero desativa a oscilação. Posição e fase diferenciam cada fonte,
independentemente da taxa de quadros.

No candelabro, cada pavio usa amplitude `0.30` e velocidade entre `2` e `3.5`.
Uma variação compartilhada de amplitude `0.15` e velocidade `0.6` simula correntes
de ar suaves. A sequência é sorteada a cada execução, e o brilho visível do fogo
acompanha a intensidade total das luzes.
Valores inválidos e excesso de luzes geram erros com indicação do arquivo e linha.

### Vincular uma luz a um objeto em C++

`include/Light.h` oferece as fábricas `Light::point`, `Light::spot` e
`Light::candle`. Todo `Object` possui um vetor público `lights`; adicione a fonte
ao objeto antes de entregá-lo à mesa com `Table::addProp`:

```cpp
// lamp é um objeto da sua cena; a posição abaixo é local ao modelo.
lamp->lights.push_back(Light::point({0, 1.5f, 0}, {1, .85f, .65f}, 12, 14));

flashlight->lights.push_back(Light::spot({0, 0, -.3f}, {0, 0, -1}));
candle->lights.push_back(Light::candle({0, .8f, 0}));
```

As luzes acompanham a posição, rotação e transformação dos pais do objeto. O
alcance permanece em unidades do mundo, mesmo ao escalar o modelo. Configure
`enabled = false` para desligar uma fonte. O limite considera juntas as luzes
ativas do cenário e dos objetos, e um excesso gera erro em vez de descartar luzes.
Objetos compostos novos devem sobrescrever `collectLights`, chamar a implementação
de `Object` e coletar os filhos usando `model(parent)`, como faz `Table`.

Para o bulbo ou a chama parecer aceso, após `shader.object(...)` e antes de
desenhar sua malha, chame `shader.emission({1, .55f, .18f})`. Essa emissão se soma
à iluminação, funciona mesmo no escuro e é zerada no próximo `shader.object(...)`.
Ela representa o brilho da superfície; a fonte `Light` ilumina o entorno.
Não há efeito de halo ou bloom.

As sombras dos sóis e relâmpagos continuam funcionando. As luzes locais desta
implementação **não projetam sombras próprias** e sua contribuição atravessa
obstáculos; sombras locais exigem mapas adicionais (cubemaps para luzes pontuais).
Elas também não alteram a skybox nem produzem feixes volumétricos na névoa.
