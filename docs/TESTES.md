# Relatório histórico de validação

Este relatório descreve a versão validada antes das alterações de tiro contínuo, metas progressivas, miniboss com 150 de vida, reforços durante chefes, novas variantes de inimigos e redução de spawn na fase 5. A suíte ainda contém expectativas anteriores a essas mudanças e não foi reexecutada ao organizar o repositório. Os resultados abaixo não são uma validação da versão atual.

Validação realizada em 28/09/2026, Linux/WSL2, GCC 13.3, FreeGLUT 3.4 e Mesa 25.2.8/llvmpipe. O contexto gráfico dos testes reportou **`1.0 Mesa 25.2.8-0ubuntu0.24.04.2`** com `MESA_GL_VERSION_OVERRIDE=1.0` e renderização por software em Xvfb. Não se trata apenas de uma compilação: houve leitura de pixels e partidas completas com eventos X11 enviados à aplicação.

Relatório atualizado para a campanha solicitada de 10 fases, miniboss na 5, boss final na 10 e fundo sem quadriculado. A meta `nParaGanhar` agora vale para cada fase comum; a vitória encerra a campanha após o boss final.

## Resultado

| Verificação | Resultado |
|---|---|
| Lógica, configuração, geometria, fases e chefes (`make test`) | **72 testes passaram, 0 falharam** |
| Renderização real e comparação de pixels (`make test-graphics`) | **20 testes passaram, 0 falharam** |
| Aplicação completa, teclado/mouse e arquivos (`make test-ui`) | **73 verificações passaram** |
| API OpenGL | **24 funções distintas**, todas no conjunto `GL_VERSION_1_0` do registro oficial Khronos |
| AddressSanitizer + LeakSanitizer + UndefinedBehaviorSanitizer | Mesmos 72 testes passaram, sem erros reportados |
| Compilação com `-Wall -Wextra -Wpedantic -Werror` | Aprovada na cópia extraída do ZIP |
| `make clean`, `make all`, `make test`, `make clean`, `make all` | Aprovados na cópia extraída do ZIP |
| Pacote de entrega | Makefile, XML e fontes presentes; sem executáveis, objetos ou bibliotecas compiladas |

As 165 verificações funcionais correspondem a 72 + 20 + 73; a execução instrumentada não foi contada novamente. Há múltiplas asserções e cenários em vários testes.

## Correspondência com o PDF

| Requisito | Validação executada |
|---|---|
| C++ com OpenGL e GLUT | Compilação e execução do binário `trabalhocg`; contexto FreeGLUT real |
| Somente OpenGL 1.0 | Chamadas comparadas ao `gl.xml` oficial e teste gráfico com versão reportada 1.0 |
| XML junto ao executável | Aplicação iniciada de `/tmp`; carregou o XML da pasta do binário |
| Largura e altura configuráveis | Parser 640 × 720; desenho em 600 × 800 e 200 × 500; resize real 420 × 600 |
| Arena preenche toda a janela | Viewport de origem (0,0), tamanho da janela e pixels nos cantos |
| Raio de cada personagem | Leitura independente, escala do pivô/cano, cores e centros verificados nos pixels |
| Pernas e arma proporcionais | Dobragem de raio dobra deslocamentos; rotação do corpo preserva partes unidas |
| Cabeça verde/roxa, pernas pretas, arma vermelha | Amostragem de cores da imagem OpenGL |
| Barril retangular com largura/altura do XML | Pontos dentro e fora do retângulo, dimensões do modelo e imagem |
| Resistência visível no barril | Impactos 3 → 2 → 1 → remoção; imagem do número muda |
| Jogador no centro da base | Coordenadas iniciais e orientação verificadas |
| A/D giram, W/S andam na direção do corpo | Posições e ângulos esperados; repetido com teclas reais da janela |
| Duas teclas descrevem movimento circular | Trajetória comparada com solução analítica; W+A real na aplicação |
| Animação das pernas ao andar | Fase de passada ao avançar/recuar e diferença dos pixels das pernas |
| Rotação parada não exige animação | Giro sem translação mantém pernas em repouso |
| Cabeça não sai das laterais/base | Movimento mantido contra cada borda; raio incluído no limite |
| Não ultrapassar centro vertical | Cabeça inteira limitada à metade inferior; W real até o limite |
| Colisão cabeça × barril causa derrota | Tangência, interseção, canto externo e passagem rápida; partida X11 termina por contato |
| Arma e pernas não são a cabeça | Barril colocado na ponta da arma não causa derrota por si só |
| Um tiro por clique esquerdo | Down repetido e Up não criam novos tiros; segurar mouse real confirma ausência de rajada |
| Tiro segue trajetória independente | Jogador anda, gira e altera mira após disparar; posição/velocidade do tiro conservadas |
| Arma gira com movimento horizontal do mouse | Mouse esquerdo aumenta ângulo, direito diminui; eventos reais da janela |
| Limite angular da Figura 3 | Saturação em −30° e +30°, abertura total de 60°; imagens distintas nos extremos |
| Cada impacto retira uma vida | Um decremento por projétil, remoção do projétil e atualização visual |
| Destruição soma um barril | Tiros simultâneos não duplicam pontuação; barril e inimigo removidos juntos |
| Placar superior direito | Imagem muda após destruição; texto e barra de progresso desenhados no topo direito |
| Meta de N barris | Lógica com N=2 conclui uma fase; campanha real com N=1 avança até o boss final |
| Mensagens de vitória/derrota | Imagens próprias para vitória, tiro e contato; estados terminais bloqueiam controles do jogo |
| X aleatório e início no topo | 10.000 sorteios, amplitude horizontal, limites do retângulo/cabeça e altura inicial |
| Probabilidade de 50% de inimigo | Sorteio Bernoulli(0,5); frequência observada dentro de 47%–53% em 10.000 amostras determinísticas |
| Barris rolam para baixo em linha reta | X constante, deslocamento em Y por tempo e fase visual do rolamento |
| Inimigo acompanha barril | Posição da cabeça inimiga coincide com a posição do barril a cada passo |
| Pernas inimigas sempre animadas | Fase e pixels mudam inclusive com deslocamento dos barris desligado |
| Inimigo aponta para jogador quando possível | Direção calculada do pivô é colinear com alvo dentro do cone; limites testados para ambos os lados |
| Frequência/velocidade de tiro configuráveis | 2,5 tiros/s produzem 5 tiros em 2 s; módulo da velocidade e origem no cano; frequência zero |
| Tiro inimigo causa derrota | Teste estático, teste de alta velocidade e partida real encerrada por disparo |
| Inimigo morre com barril | Após destruir, não cria novos tiros; tiros anteriores continuam independentes |
| Equivalência entre máquinas | Estados iguais a 20, 30, 60, 144 e 240 FPS; chamadas irregulares equivalentes a uma chamada longa |
| Double buffer | Consulta `GL_DOUBLEBUFFER` verdadeira no contexto e uso de `glutSwapBuffers` |
| Estado das teclas e KeyUp | Teclas simultâneas, opostas, maiúsculas, soltura e saída do cursor da janela |
| Atalhos para apresentação | F1/H, P, R, F2–F5, indicadores visuais e Esc testados pela interface; pausa também congela transições e avisos de chefes |
| Modularização | Configuração, geometria, modelo, controle, desenho e inicialização em módulos separados |
| Entrega com Makefile e nome exigido | ZIP extraído compilou `trabalhocg`; clean e recompilação aprovados |

## Casos adicionais

- XML vazio, malformado, raiz incorreta, múltiplas raízes, elemento ausente/duplicado, arquivo inexistente, `...`, `NaN`, infinito, unidades misturadas ao número, valores negativos, resistência fracionária, meta zero, raio incompatível e barril maior que a arena são rejeitados.
- Colisão contínua de projétil a 100.000 unidades/s e barril a 10.000 unidades/s; o objeto não atravessa o alvo entre dois passos.
- Dois barris no mesmo trajeto são resolvidos pela distância de impacto, independentemente da ordem do vetor.
- Tangência real no canto é detectada; um ponto no quadrado da caixa expandida, mas fora do círculo do canto, não causa falso positivo.
- Tiro próprio não mata jogador; tiro inimigo não danifica barril; barril que escapa não pontua.
- Pausa e ajuda suspendem o relógio; retornar não acumula o tempo pausado. A pausa anterior continua válida depois de fechar a ajuda.
- Estado reiniciado limpa placar, entidades, teclas, mira, ajuda e opções de demonstração. Semente fixa reproduz a sequência inicial.
- Simulação de 120 segundos confirma remoção de projéteis/barris que saem e valores finitos.
- Cem renderizações consecutivas mantêm a pilha de matrizes no nível inicial.

## Campanha e chefes inspirados em Enter the Gungeon

| Funcionalidade | Evidência |
|---|---|
| Fundo liso | Pixels antes ocupados por linhas verticais/horizontais são iguais ao fundo; linha central permanece |
| Dez fases em ordem | Lógica e aplicação real percorrem 1–10 sem atalhos; não há fase 11 |
| Miniboss apenas na 5 e boss final apenas na 10 | Tipo da entidade verificado em cada uma das dez fases |
| Meta por fase e placar acumulado | Placar local reinicia, total permanece; progresso de todas as fases verificado na interface |
| Intervalo automático | Arena limpa, tiros removidos, controles bloqueados e jogador reposicionado após dois segundos |
| Dificuldade progressiva | Velocidade e frequência crescem, resistência não diminui nas fases comuns |
| Chefes ampliados | Corpo, pontos de vida e colisão nas extremidades conferidos independentemente do tamanho do barril comum |
| Movimento lateral | Chefe permanece inteiro na metade superior, não escapa e não gera barris extras durante 60 s |
| Aviso antes dos ataques | Ausência de tiros inicial, brilho/contorno e aviso visíveis antes da rajada; F3 interrompe ataques |
| Leque direcionado | Direção do projétil central aponta ao jogador; projéteis laterais deixam espaçamento |
| Anel com abertura | Tiros em várias direções e abertura angular de pelo menos 60° para fuga |
| Dois canhões | Rajada nasce de duas posições distintas do boss final |
| Fúria a meia vida | Mais projéteis, velocidade e cadência maiores; olhos e HUD mudam; ativação confirmada na partida real |
| Vitória no final | Chefe da fase 10 precisa perder toda a vida; fases anteriores nunca geram vitória |
| Reinício e pausa | R volta à fase 1 e remove fúria/ataques; P e ajuda congelam intervalos e avisos |
| Independência de FPS | Movimento lateral e ataques de chefe idênticos a 20 e 144 FPS |

A campanha automatizada usa resistência base 1, meta 1, barris imóveis e frequência inimiga zero para isolar a sequência de fases com cliques reais. Os padrões ofensivos, dano, avisos, fúria e movimento são cobertos separadamente pelos testes de lógica e pelos testes gráficos com disparos habilitados. Isso não equivale a uma avaliação humana do equilíbrio da dificuldade.

## Evidências e reprodução

Os códigos executáveis dos testes estão em `tests/game_tests.cpp`, `tests/graphics_tests.cpp` e `tests/ui_tests.py`. Os comandos completos estão no README.

`build/evidencias/01-arena.ppm` a `19-anel.ppm` registram cenas, fases, miniboss, boss final, fúria e aviso de ataque. As pastas `build/evidencias/ui/controles`, `campanha`, `transicao`, `derrota-barril` e `derrota-tiro` contêm imagens PPM, estados JSON, XML do cenário e saída do programa. Esses artefatos são gerados novamente pelos testes e removidos por `make clean`; não entram na entrega.

O auditor aceita opcionalmente o registro Khronos para uma conferência independente da lista local:

```sh
python3 tests/check_gl_api.py caminho/gl.xml
```

Fonte conferida: [registro oficial `gl.xml`, seção `GL_VERSION_1_0`](https://github.com/KhronosGroup/OpenGL-Registry/blob/main/xml/gl.xml). Foram auditadas as chamadas dos fontes da aplicação e dos testes gráficos; o FreeGLUT é a biblioteca de janela/input exigida no enunciado.

## Limites da evidência

Execução gráfica confirmada em X11 virtual com Mesa por software; não foi feita uma apresentação humana, nem validação em GPU física ou no computador do laboratório. O sanitizer foi executado fora do sandbox, pois o LeakSanitizer não funciona sob o rastreamento de processos do sandbox. Nenhuma falha de memória foi reportada nessa execução.

O ZIP foi gerado com um nome temporário apenas para validar extração/compilação e removido depois. Para a entrega final, usar `make dist AUTOR=SeuNomeCompleto`. O nome completo do autor e a apresentação presencial não foram inferidos.
