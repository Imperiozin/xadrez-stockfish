# Cenários

O jogo carrega `sunset.cfg` por padrão. Cada arquivo reúne a pasta da skybox,
a iluminação, a névoa e a projeção das sombras. Os shaders recebem esses valores
em tempo de execução.

Edite o arquivo de origem e rode `python tools/build.py --run` para copiar os
assets e iniciar o jogo. Para experimentar sem recompilar, edite a cópia em
`build/windows/bin/assets/scenarios/sunset.cfg` e reabra o executável; o próximo
build sobrescreve essa cópia.

Para criar outro cenário, duplique `sunset.cfg`, ajuste `skybox` e a iluminação.
Mantenha as seis faces PNG na pasta indicada por `skybox` com os nomes existentes.
Depois de copiar os assets pelo build, selecione o cenário ao iniciar:

```powershell
.\build\windows\bin\xadrez.exe --scenario assets/scenarios/outro.cfg
```

Os caminhos são relativos à pasta do executável; caminhos absolutos também são
aceitos. Cada linha contém um nome e seus valores, separados por espaços.
Comentários começam com `#`.

- `lightPosition`: posição XYZ da fonte, apontada para a origem da cena.
- `lightColor`: cor RGB, normalmente entre 0 e 1.
- `lightIntensity`: multiplicador da luz direta, independente da distância.
- `ambientColor` e `ambientStrength`: cor e força da luz nas áreas de sombra.
- `diffuseStrength`, `specularStrength`, `woodSpecularStrength`, `shininess`:
  resposta difusa e dos brilhos.
- `fogColor` e `fogDensity`: cor e densidade da névoa; densidade 0 a desativa.
- `shadowFov`: abertura da câmera da luz em graus.
- `shadowAutoFit`: 1 ajusta automaticamente a abertura e os planos de corte
  quando a luz muda de posição; 0 usa `shadowFov`, `shadowNear` e `shadowFar`.
- `shadowRadius`: raio do volume ao redor da origem que precisa receber e
  projetar sombras. O padrão 20 inclui a mesa inteira e seus pés. Aumente para
  objetos fora desse volume; a luz deve estar além desse raio.
- `shadowNear` e `shadowFar`: limites de profundidade, em unidades da cena.
  No modo manual, ajuste-os para incluir toda a mesa e os objetos.
- `shadowBias` e `shadowMinBias`: compensação de profundidade para evitar
  marcas de autossombreamento; valores excessivos afastam as sombras dos objetos.
- `shadowNormalOffset`: deslocamento da consulta à sombra ao longo da normal,
  em unidades do mundo. Suprime marcas nas superfícies curvas; valores altos
  podem afastar as sombras de contato. O padrão é 0.012.
