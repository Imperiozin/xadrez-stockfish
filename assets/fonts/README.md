# Fonte do bloco de notas

`Schoolbell-Regular.ttf` é a fonte Schoolbell usada na folha do bloco. Os avisos na tela mantêm a fonte padrão do Dear ImGui.

- Família: [Schoolbell, no Google Fonts](https://fonts.google.com/specimen/Schoolbell).
- Origem: [repositório oficial Google Fonts](https://github.com/google/fonts/tree/main/apache/schoolbell), arquivo `Schoolbell-Regular.ttf`, sem alterações.
- Licença: Apache License 2.0, incluída em `LICENSE-Schoolbell.txt`.

A fonte anterior, Playwrite Brasil, permanece em `PlaywriteBR.ttf`, com a licença SIL Open Font License 1.1 em `OFL-PlaywriteBR.txt`.

Para usar outra fonte, adicione o arquivo `.ttf` ou `.otf` nesta pasta e altere o caminho passado para `AddFontFromFileTTF` em `src/graphics/Notepad.cpp`. A fonte deve conter os acentos do português. A compilação copia esta pasta junto dos demais recursos.
