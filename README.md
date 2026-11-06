# Ataque de Barris — Computação Gráfica 2D

Jogo em C++17, OpenGL **1.0** e FreeGLUT, implementado a partir do PDF desta pasta e ampliado com uma campanha de **10 fases**, miniboss na fase 5 e boss final na fase 10. A arena ocupa a janela inteira e tem fundo liso. O jogador destrói barris e chefes, evitando contato e tiros inimigos.

**Navegação:** [Execução](#compilar-e-executar) · [Controles](#controles) · [Configuração](#configuração) · [Campanha](#campanha-e-chefes) · [Testes](#testes) · [Organização](#organização)

## Visão geral

| Área | Implementação |
| --- | --- |
| Linguagem e janela | C++17, OpenGL 1.0 e FreeGLUT |
| Interação | Movimento pelo teclado, mira pelo mouse e disparos |
| Simulação | Barris, projéteis, colisões e estados de jogo |
| Progressão | Campanha com dez fases e dois chefes |
| Configuração | XML lido com TinyXML2 incluído no repositório |

O [enunciado da disciplina](26-09-24%20CG%202020-2%20-%20T2D.pdf) registra os requisitos e as regras de avaliação. A documentação de apoio e as fontes dos testes permanecem disponíveis em [docs/](docs) e [tests/](tests).

## Compilar e executar

Na raiz, o script `make.sh` executa o Makefile e detecta o ambiente temporário desta sessão quando necessário:

```sh
./make.sh                     # Compilar
./make.sh run                 # Compilar e abrir o jogo
./make.sh clean               # Limpar arquivos gerados
```

O script também funciona quando chamado de outra pasta e aceita os demais alvos/opções do Make.

Em Linux com compilador C++17, Make, OpenGL e FreeGLUT disponíveis:

```sh
make all
./trabalhocg
# Ou compile e execute em um comando:
make run
```

Em Ubuntu/Debian, as dependências de desenvolvimento são `g++`, `make` e `freeglut3-dev`:

```sh
sudo apt install g++ make freeglut3-dev
```

O TinyXML2 10.0.0 está incluído em código-fonte, com licença, em `third_party/tinyxml2`. Não é necessário instalar um parser XML. O jogo não usa imagens externas, fontes externas, shaders, texturas, VBOs, GLU nem funções OpenGL posteriores a 1.0. Até as letras são desenhadas com quadriláteros em modo imediato.

```sh
make clean                    # Remove executável, objetos e resultados de testes
make all                      # Recompila antes das verificações abaixo
./trabalhocg --verificar-config
./trabalhocg --ajuda
```

O alvo de compilação validado foi Linux/WSL2. É necessário um servidor gráfico, como WSLg ou X11, para abrir a janela. Não foi validada uma compilação nativa para Windows/macOS.

Na preparação inicial dos testes, as ferramentas foram extraídas **temporariamente em `/tmp/cg-toolchain`**. Se as dependências ainda não estiverem instaladas, `make.sh` reconhece esse ambiente automaticamente enquanto os arquivos existirem. Para carregá-lo manualmente:

```sh
. /tmp/cg-env.sh
make all
./trabalhocg
```

## Controles

| Entrada | Ação |
|---|---|
| W / S | Andar para frente / para trás na direção do corpo |
| A / D | Girar no sentido anti-horário / horário |
| W ou S + A ou D | Percorrer um arco de círculo |
| Mouse para esquerda / direita | Girar somente a arma; limite de −30° a +30° relativo ao corpo |
| Botão esquerdo | Um tiro imediato por clique; segurar mantém disparos contínuos e soltar interrompe |
| P | Pausar / continuar |
| R | Reiniciar, restaurando também todas as opções de demonstração |
| F1 ou H | Abrir / fechar ajuda; a simulação para enquanto a ajuda está aberta |
| F2 | Ligar / desligar deslocamento dos barris |
| F3 | Ligar / desligar novos disparos dos inimigos |
| F4 | Ligar / desligar nascimento de novos barris |
| F5 | Mostrar / ocultar contornos de colisão e direção do cano |
| Esc | Sair |

Os estados dos atalhos de demonstração aparecem na tela. F3 não apaga tiros já disparados. As pernas inimigas continuam animadas mesmo com F2 desligado.

## Configuração

O arquivo padrão é **`configuracoes.xml` ao lado do executável**, independentemente da pasta de onde o programa for chamado. O XML fornecido inicialmente continha `...`; ele foi preservado em `docs/configuracoes-original.xml`. O arquivo ativo contém valores jogáveis.

| Elemento / atributo | Unidade / significado | Padrão entregue |
|---|---|---|
| `arena.largura`, `arena.altura` | Pixels da janela e dimensões do mundo | 600 × 800 |
| `jogador.raioCabeca` | Raio; escala proporcionalmente pernas e arma | 18 |
| `jogador.velocidade` | Unidades por segundo | 180 |
| `jogador.velocidadeGiro` | Graus por segundo, opcional | 150 |
| `jogador.velocidadeTiro` | Unidades por segundo, opcional | 480 |
| `jogador.tirosPorSegundo` | Cadência ao segurar o mouse, opcional | 6 |
| `inimigo.raioCabeca` | Raio e escala do corpo inimigo | 15 |
| `inimigo.tirosPorSegundo` | Frequência por inimigo; aceita frações e zero | 0,7 |
| `inimigo.velocidadeTiro` | Unidades por segundo | 190 |
| `barril.altura`, `barril.largura` | Dimensões do retângulo | 64 × 76 |
| `barril.numeroTiros` | Resistência base dos barris e escala de vida dos chefes | 3 |
| `barril.nParaGanhar` | Barris necessários em cada fase comum | 10 |
| `barril.velocidade` | Unidades por segundo, para baixo | 65 |
| `outros.intervaloBarris` | Segundos entre nascimentos, opcional | 2,1 |
| `outros.sensibilidadeMouse` | Graus por pixel horizontal, opcional | 0,25 |
| `outros.semente` | Inteiro opcional para repetir o sorteio | Ausente: semente aleatória |

No XML, números decimais usam **ponto**. Sem `jogador.velocidadeTiro`, a velocidade do tiro inimigo é usada também para o jogador, mantendo compatibilidade com os atributos do modelo original. Valores inválidos, números ausentes e dimensões incompatíveis produzem mensagem de erro antes de abrir a janela. Velocidade zero de jogador/barril e frequência zero do inimigo são permitidas para demonstração.

É possível carregar outro arquivo explicitamente:

```sh
./trabalhocg --config caminho/alternativo.xml
```

## Regras e decisões de implementação

- A Figura 3 representa uma abertura **total de 60°**: a arma tem limite de 30° para cada lado do corpo. O inimigo comum usa o mesmo limite e mira a partir do pivô da própria arma. Os chefes têm canhões e padrões próprios, incluindo ataques radiais.
- Toda a cabeça do jogador permanece na metade inferior, inclusive ao encostar nas laterais e na linha central. Arma e pernas não participam das colisões.
- Barris aparecem inteiros no topo, em X aleatório. Cada nascimento tem sorteio independente com probabilidade de 50% de conter um inimigo.
- O barril perde uma vida por tiro do jogador. Com resistência zero, ele e o inimigo são removidos e o placar aumenta uma vez. Barris que escapam não pontuam.
- Barris sem inimigo são removidos depois de sair pela base; os que carregam inimigo são removidos quando a cabeça dele alcança a saída, preservando as cabeças dos personagens dentro da arena.
- Tiros já disparados conservam origem, velocidade e direção, mesmo se o atirador mover, girar ou morrer. Um tiro só pode atingir o primeiro barril válido do seu trajeto. Tiros inimigos só atingem o jogador.
- A simulação usa passos fixos de 1/240 s e o tempo real decorrido entre chamadas `idle`. A renderização é solicitada a aproximadamente 60 quadros/s. Uma chamada demorada processa todos os passos pendentes, sem descartar tempo.
- Colisões usam trajetórias contínuas relativas, incluindo círculos nos cantos do retângulo, evitando atravessamentos de tiros rápidos e falsos contatos nas quinas. Eventos são resolvidos por ordem de impacto. Em empate exato, dano de tiro em barril precede contato corporal.
- O programa usa double buffer, estado persistente das teclas e `glutKeyboardUpFunc`. Repetição automática de teclado é desabilitada. Sair com o cursor da janela limpa teclas presas e reinicia a referência do mouse.

## Campanha e chefes

| Fase | Objetivo |
|---|---|
| 1–4 | Destruir a meta de barris da fase; a dificuldade aumenta a cada fase |
| 5 | Derrotar o **Barão do Tambor**, miniboss com canhões e armadura dourada |
| 6–9 | Ondas de barris mais rápidas, resistentes e frequentes |
| 10 | Derrotar o **Rei da Pólvora**, boss final de armadura vermelha, caveira e dois canhões |

O HUD separa fase atual, meta da fase e total de barris destruídos. Ao completar uma fase, a arena é limpa e há uma pausa automática de dois segundos. O jogador começa a próxima fase na base, com orientação e mira restauradas. A vitória só aparece após derrotar o chefe da fase 10. R reinicia toda a campanha; P e a ajuda também suspendem a contagem entre fases.

Nas fases comuns, a meta é `nParaGanhar + 2 × (fase − 1)`: com o XML padrão, são 10, 12, 14 e 16 barris nas fases 1–4 e 20, 22, 24 e 26 nas fases 6–9. A velocidade dos barris recebe +8% da base por fase, o intervalo de nascimento é dividido por `1 + 0,10 × (fase − 1)` e a resistência aumenta em 1 a cada três fases. A frequência de tiro dos inimigos comuns recebe +6% da base por fase. As opções de demonstração F2–F5 continuam disponíveis durante a campanha.

Os chefes têm visuais próprios de munição e armadura inspirados em **Enter the Gungeon**, desenhados com primitivas OpenGL 1.0. Eles oscilam lateralmente na metade superior e não escapam pela base. Barris comuns continuam surgindo durante os confrontos; na fase 5, o intervalo entre nascimentos é multiplicado por cinco (7,5 segundos no XML padrão). Destruir reforços soma ao placar, mas a fase só termina ao derrotar o chefe.

- **Barão do Tambor:** 150 pontos de vida; alterna leque de três tiros direcionado ao jogador e anel radial com abertura para fuga. A cadência foi dobrada e os projéteis ficaram mais rápidos em relação à versão inicial.
- **Rei da Pólvora:** vida igual a 10 vezes `numeroTiros` (30 no padrão); alterna leque de cinco tiros, anel radial e rajada dos dois canhões. Ao atingir metade da vida, entra em **fúria**, aumenta o ritmo e a velocidade dos ataques e passa a disparar mais projéteis.
- Antes de cada ataque, os canhões brilham, um contorno pulsa e o HUD mostra o padrão que será disparado. Os tiros dos chefes têm contorno escuro e núcleo claro para facilitar a leitura.
- Os anéis deixam uma abertura de fuga; o intervalo mínimo entre ataques é de 0,6 s para o miniboss e 1,2 s para o boss final. A velocidade e o ritmo usam os parâmetros do inimigo como base. Frequência zero ou F3 desligado suspendem os ataques dos chefes.

A partir da fase 6, os barris com inimigo podem carregar o atirador comum (50%), um atirador com leque de três tiros (25%) ou um canhoneiro com oito tiros em salva circular (25%). Os dois novos tipos têm cores e armas próprias, aviso antes dos ataques e barris com, respectivamente, um e três pontos extras de resistência. A inspiração em **Seraph's Last Stand** se limita aos padrões de ataque; os personagens continuam vindo em barris.

## Testes

Os testes e o relatório registram a versão anterior ao tiro contínuo e aos últimos ajustes de dificuldade. As expectativas de campanha, chefes e mouse ainda precisam ser atualizadas; os resultados históricos não validam o estado atual. Nenhuma nova execução foi feita ao organizar este repositório.

```sh
make test                     # Lógica, XML, geometria e controle
make test-api                 # Auditoria das chamadas OpenGL
make test-graphics            # Janela OpenGL real, pixels e screenshots
make test-ui                  # Aplicação completa com teclado/mouse X11
```

Os dois primeiros testes não precisam de tela. Os gráficos precisam de uma tela X11; o teste de interface também requer Python 3 e `xdotool`. Essas ferramentas são necessárias **somente para os testes**, não para compilar ou jogar.

Para executá-los sem usar a tela física, com Xvfb instalado:

```sh
LIBGL_ALWAYS_SOFTWARE=1 MESA_GL_VERSION_OVERRIDE=1.0 xvfb-run -a make test-graphics
LIBGL_ALWAYS_SOFTWARE=1 MESA_GL_VERSION_OVERRIDE=1.0 xvfb-run -a make test-ui
```

As capturas PPM e estados JSON ficam em `build/evidencias/`. O relatório de cobertura está em [docs/TESTES.md](docs/TESTES.md) e o roteiro de apresentação em [docs/APRESENTACAO.md](docs/APRESENTACAO.md).

Para registrar uma demonstração própria, execute `./trabalhocg --diagnostico pasta` e pressione F12. Cada captura grava o framebuffer e o estado da mesma cena; o modo é opcional e não altera as regras.

## Entrega

```sh
make dist AUTOR=SeuNomeCompleto
```

Gera `entrega/SeuNomeCompleto.zip` contendo Makefile, XML, fontes, parser e licença, documentação e fontes dos testes. O pacote exclui executáveis, objetos, bibliotecas compiladas, capturas de teste e ferramentas locais. O nome do autor deve ser preenchido antes de gerar a entrega.

## Organização

| Arquivo | Responsabilidade |
|---|---|
| `src/config.*` | XML, validação e localização do executável |
| `src/math.*` | Vetores e interseções geométricas |
| `src/game.*` | Entidades, passos de simulação, combate e estados |
| `src/controller.*` | Teclado e mouse |
| `src/renderer.*` | Desenho exclusivamente em OpenGL 1.0 |
| `src/main.cpp` | Janela, callbacks FreeGLUT, relógio e inicialização |
| `tests/` | Testes automatizados, auditoria e geração do ZIP |

Referências técnicas usadas: [registro oficial OpenGL da Khronos](https://registry.khronos.org/OpenGL/index_gl.php), [registro XML da API](https://github.com/KhronosGroup/OpenGL-Registry/blob/main/xml/gl.xml) e [TinyXML2 10.0.0](https://github.com/leethomason/tinyxml2/tree/10.0.0).
