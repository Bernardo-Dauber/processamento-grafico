# CONTEXT.md — Glossário do Projeto (Grau A — Processamento Gráfico)

Glossário de domínio. Sem detalhes de implementação — decisões técnicas ficam em `docs/adr/`.

## Termos

### Mini-Mario
O jogo escolhido: platformer lateral simplificado inspirado em Super Mario Bros., com escopo deliberadamente reduzido a 1 fase.

### Fase (Level)
O único nível do jogo. Tamanho fixo (aprox. 2–3 larguras de tela). Contém plataformas, moedas, inimigos e a Bandeira.

### Plataforma
Retângulo sólido do cenário sobre o qual o Jogador pode ficar de pé. Bloqueia movimento por cima, pelos lados e por baixo. Posição definida no arquivo de configuração.

### Jogador (Player)
O personagem controlado pelo usuário via teclado. Estados de animação: parado, andando, pulando.

### Inimigo (Goomba)
Entidade que anda de um lado para o outro em uma Plataforma, sem IA. Encostar nele pelo lado mata o Jogador; cair em cima o derrota.

### Moeda (Coin)
Item colecionável animado (spritesheet). Coletada quando a hitbox do Jogador a toca.

### Bandeira (Flag)
Marcador de fim de Fase. Tocá-la encerra a fase com vitória.

### Hitbox
Retângulo (AABB) associado a cada entidade, usado para testes de colisão — tanto entre sprites (Jogador × Moeda/Inimigo) quanto contra o cenário (Jogador × Plataforma).

### Camada (Layer)
Imagem de fundo que rola em velocidade proporcional à câmera, criando o efeito de Parallax.

### Parallax
Movimento das Camadas de fundo em velocidades diferentes (mais distantes = mais lentas).
