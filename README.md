# Ataque de Barris — Computação Gráfica 2D

Jogo desenvolvido em **C++17, OpenGL e FreeGLUT** na disciplina de Computação Gráfica da UFES. O jogador controla um personagem com canhão, desvia de barris e enfrenta inimigos em uma arena 2D. A campanha amplia as regras do trabalho com dez fases, um miniboss e um chefe final.

**[Jogar a campanha no navegador](https://gabrieldealbuquerque.github.io/jogos/ataque-de-barris/)** · [Compilar](#compilar-e-executar) · [Controles](#controles) · [Versão web](#versão-web)

## O que o projeto explora

| Área | Implementação |
| --- | --- |
| Geometria | Translação, rotação e composição de transformações do corpo, pernas e arma |
| Animação | Movimento e passada dos personagens, rolagem dos barris e efeitos de combate |
| Colisões | Interseções contínuas entre trajetórias, círculos e retângulos |
| Simulação | Passo fixo de 1/240 s, estados de jogo e progressão entre fases |
| Configuração | Arena, velocidades, resistência e frequência de disparos em XML |
| Web | Mesmo núcleo C++ compilado para WebAssembly com Emscripten |

## Compilar e executar

Em Linux, instale um compilador C++17, Make e FreeGLUT. Em Ubuntu/Debian:

```sh
sudo apt install g++ make freeglut3-dev
make
./trabalhocg
```

O TinyXML2 já está incluído em `third_party/tinyxml2`, com sua licença. Para abrir a janela, é necessário um ambiente gráfico, como X11 ou WSLg.

```sh
make run                         # Compilar e abrir o jogo
./trabalhocg --verificar-config   # Validar o XML sem abrir a janela
./trabalhocg --ajuda              # Ver os controles e opções
make clean                       # Remover os arquivos gerados
```

O script `./make.sh` também aceita os alvos do Makefile e funciona quando chamado de outra pasta. O executável é gerado localmente; não faz parte do repositório.

## Controles

| Entrada | Ação |
| --- | --- |
| W / S | Avançar / recuar na direção do corpo |
| A / D | Girar o personagem |
| Mouse na horizontal | Girar a arma, até 30° para cada lado |
| Botão esquerdo | Atirar; segurar mantém os disparos |
| P | Pausar / continuar |
| R | Reiniciar |
| H / F1 | Ajuda na versão desktop |
| M | Voltar à seleção de modos no desktop |
| Esc | Sair da versão desktop |

A versão desktop oferece o **modo trabalho**, com as regras da disciplina, e a **campanha**, com vida, dano e progressão. A ajuda reúne os atalhos de demonstração disponíveis no desktop.

No navegador, a campanha começa diretamente. Também é possível atirar com Espaço ou usar os controles de toque. O jogo pausa ao perder o foco.

## Campanha

São dez fases, com dificuldade crescente e padrões de ataque distintos. A quinta fase apresenta um miniboss; a décima encerra a campanha com o chefe final. O jogador começa com 100 pontos de vida e recebe um breve período de proteção após sofrer dano. Barris destruídos contam para a meta da fase; barris que saem da arena não pontuam.

## Configuração

`configuracoes.xml` define as dimensões da arena, o tamanho e a velocidade dos personagens, a resistência dos barris e os intervalos de disparo e nascimento. Os valores usam ponto como separador decimal. O arquivo fornecido configura uma arena de **600 × 800**.

Por padrão, o programa procura o XML ao lado do executável. Para usar uma configuração alternativa:

```sh
./trabalhocg --config caminho/alternativo.xml
```

Na versão web, o XML do repositório é incorporado ao WebAssembly durante a compilação. Alterações nele exigem gerar novamente os arquivos web.

## Versão web

Ative o [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) e execute:

```sh
source /caminho/emsdk/emsdk_env.sh
make web
# Ou escolha a pasta de saída:
./web/build.sh /caminho/saida
```

O build gera `game.js` e `game.wasm` em `web/dist/` por padrão. A versão utilizada foi **Emscripten 7.0.0**. `web/main.cpp` adapta o loop e as entradas para o navegador; a simulação e a renderização são compartilhadas com a versão desktop. O OpenGL de modo imediato usa a camada de compatibilidade WebGL do Emscripten.

Os fontes e o script de compilação ficam neste repositório. A interface do jogador e os arquivos compilados são publicados em [`jogos/ataque-de-barris/` no portfólio](https://github.com/gabrieldealbuquerque/gabrieldealbuquerque.github.io/tree/main/jogos/ataque-de-barris), servidos pelo GitHub Pages. Para atualizar o jogo publicado, substitua os dois arquivos gerados nessa pasta.

## Organização

| Caminho | Responsabilidade |
| --- | --- |
| `src/configuracao.*` | Leitura e validação do XML |
| `src/geometria.*` | Vetores e interseções |
| `src/jogo.*` | Entidades, simulação, combate e progressão |
| `src/controlador.*` | Entradas da versão desktop |
| `src/renderizador.*` | Desenho da arena, personagens e interface |
| `src/principal.cpp` | Janela e callbacks FreeGLUT |
| `web/main.cpp` | Inicialização e integração com o navegador |
| `web/build.sh` | Compilação WebAssembly |
| `third_party/tinyxml2/` | Parser XML e licença de terceiros |
