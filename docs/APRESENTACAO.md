# Roteiro de demonstração — até 10 minutos

Antes de apresentar, compile com `make all`, confira `./trabalhocg --verificar-config` e abra o jogo. Use os mesmos fontes e XML entregues. F1 mostra todos os controles. R sempre restaura uma partida normal e todos os atalhos de demonstração.

| Tempo | Demonstrar | Procedimento e resultado a observar |
|---|---|---|
| 0:00–1:00 | Desenho e XML | Mostrar XML; identificar arena, cabeças verde/roxa, pernas pretas, arma vermelha, barril laranja, resistência e placar superior direito. A janela tem 600 × 800 pixels. |
| 1:00–2:30 | Movimento | Desligar F2/F3/F4. W/S movem na direção do corpo; A/D giram. Combinar W+A e W+D para mostrar trajetórias circulares. Andando, as pernas alternam; girando parado, não. |
| 2:30–3:30 | Limites | W até o centro, S até a base, girar e andar até cada lateral. A cabeça para inteira na borda. F5 revela a cabeça usada na colisão. |
| 3:30–4:30 | Mira e tiros | Mover mouse para os dois lados, mostrando os limites de −30°/+30°. Clicar para um tiro imediato; segurar para disparos contínuos e soltar para interromper. Girar após atirar: o tiro antigo mantém trajetória. |
| 4:30–6:00 | Resistência e placar | R; desligar F2/F3/F4 para isolar o primeiro barril. Alinhar corpo/mira; clicar três vezes, aguardando os impactos. A resistência passa por 3, 2, 1; o barril desaparece e o placar soma 1. |
| 6:00–7:00 | Oponente | R e aguardar barril com inimigo (sorteio de 50%). F2 para observar a animação contínua das pernas. A arma acompanha o jogador dentro dos limites. F3 alterna novos disparos. Destruir o barril remove o inimigo. |
| 7:00–8:00 | Derrotas e reinício | R e alinhar jogador a um barril que está descendo; deixar tocar a cabeça. Mostrar mensagem de derrota e R. Em outra partida, deixar um tiro inimigo atingir o jogador. |
| 8:00–9:00 | Campanha e chefes | Mostrar a meta crescente e a transição automática. A fase 5 tem miniboss com 150 de vida e reforços menos frequentes; após ela, aparecem atiradores de leque e canhoneiros em barris. A fase 10 tem boss final com fúria. Usar uma partida já avançada para a demonstração dos chefes. |
| 9:00–10:00 | Estrutura e testes | Explicar `Game`, `Renderer`, `Controller`, XML, relógio, double buffer e KeyUp. Distinguir os resultados históricos em `TESTES.md` das alterações recentes ainda não validadas. |

Abertura da arma: os 60° da Figura 3 são o arco total, não 60° para cada lado. Inimigos comuns usam o mesmo intervalo. Todos os comprimentos do corpo, pivô da arma e cano são multiplicados pelo raio da cabeça. Os chefes da campanha usam canhões próprios, ataques em leque, anéis com abertura e rajadas, com aviso antes de disparar.

Para reproduzir uma sequência de barris durante o ensaio, adicionar `semente="42"` em `<outros>`. Manter o mesmo XML para entrega e apresentação. F2/F3/F4/F5 sempre mostram na tela quais recursos estão ligados; P e a ajuda interrompem o relógio da simulação.
