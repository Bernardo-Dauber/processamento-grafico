#include "Player.hpp"

#include "Level.hpp"
#include "SpriteRenderer.hpp"
#include "Texture2D.hpp"

void Player::spawn(const glm::vec2 &pos)
{
    position = pos;
    velocity = glm::vec2(0.0f);
    onGround = false;
    facingRight = true;
    mAnim = kIdle;
    mAnimTimer = 0.0f;
}

void Player::update(float dt, const Level &level, const PlayerInput &input)
{
    // ---- Movimento horizontal (arcade: velocidade instantânea) ----
    float move = (input.right ? 1.0f : 0.0f) - (input.left ? 1.0f : 0.0f);
    velocity.x = move * level.moveSpeed;
    if (move > 0.0f)
        facingRight = true;
    else if (move < 0.0f)
        facingRight = false;

    // ---- Pulo ----
    // Só pula com a "borda de subida" da tecla E estando no chão.
    if (input.jumpPressed && onGround)
        velocity.y = -level.jumpVelocity;
    // Pulo variável: soltar a tecla cedo corta o impulso (o pulo "parece Mario")
    if (!input.jumpHeld && velocity.y < -level.jumpVelocity * 0.35f)
        velocity.y = -level.jumpVelocity * 0.35f;

    // ---- Gravidade ----
    velocity.y += level.gravity * dt;

    // ---- Colisão por eixos separados ----
    // Posição ANTES de qualquer movimento deste frame: é o que permite às
    // funções de resolução saber de "onde o jogador veio" (veio de cima?
    // veio de baixo?) e resolver a penetração pelo lado certo.
    const float prevBottom = hitbox().bottom();
    const float prevTop = hitbox().y;

    // 1) Move e resolve no eixo X (paredes)
    position.x += velocity.x * dt;
    resolveCollisionsX(level);

    // 2) Move e resolve no eixo Y (chão e teto)
    position.y += velocity.y * dt;
    onGround = false;
    resolveCollisionsY(level, prevBottom, prevTop);

    updateAnimation(dt, move != 0.0f);
}

void Player::resolveCollisionsX(const Level &level)
{
    AABB hb = hitbox();
    for (const Platform &p : level.platforms) {
        AABB pb = aabbFrom(p.position, p.size);
        if (!hb.overlaps(pb))
            continue;
        // Empurra a hitbox de volta para a borda da plataforma
        if (velocity.x > 0.0f) // indo para a direita: encosta pela esquerda da plataforma
            position.x = pb.x - (hb.x - position.x) - hb.w;
        else if (velocity.x < 0.0f) // indo para a esquerda
            position.x = pb.right() - (hb.x - position.x);
        velocity.x = 0.0f;
        hb = hitbox();
    }
}

void Player::resolveCollisionsY(const Level &level, float prevBottom, float prevTop)
{
    const float kTolerance = 1.0f;

    // Invariante do jogo: como o Y é resolvido todo frame, o jogador nunca
    // começa um frame embutido em plataforma — assim o passe de X só vê
    // sobreposição quando o PRÓPRIO movimento horizontal a criou (parede).
    AABB hb = hitbox();
    for (const Platform &p : level.platforms) {
        AABB pb = aabbFrom(p.position, p.size);
        if (!hb.overlaps(pb))
            continue;

        if (velocity.y > 0.0f) {
            // Caindo: só aterrissa se veio de CIMA da plataforma
            // (evita "teletransportar" para o topo ao roçar a lateral)
            if (prevBottom <= pb.y + kTolerance) {
                position.y = pb.y - (hb.y - position.y) - hb.h;
                velocity.y = 0.0f;
                onGround = true;
                hb = hitbox();
            }
        } else if (velocity.y < 0.0f) {
            // Subindo: só bate a cabeça se veio de BAIXO da plataforma
            if (prevTop >= pb.bottom() - kTolerance) {
                position.y = pb.bottom() - (hb.y - position.y);
                velocity.y = 0.0f;
                hb = hitbox();
            }
        }
    }
}

void Player::updateAnimation(float dt, bool moving)
{
    PlayerAnim newState;
    if (!onGround)
        newState = PlayerAnim::Jump;
    else if (moving)
        newState = PlayerAnim::Walk;
    else
        newState = PlayerAnim::Idle;

    // Trocar de estado reinicia o timer (cada animação começa do 1º frame)
    const Animation *target = nullptr;
    switch (newState) {
    case PlayerAnim::Idle: target = &kIdle; break;
    case PlayerAnim::Walk: target = &kWalk; break;
    case PlayerAnim::Jump: target = &kJump; break;
    }
    if (mAnim.row != target->row || mAnim.frameCount != target->frameCount) {
        mAnimTimer = 0.0f;
    }
    mAnim = *target;
    mAnimTimer += dt;
}

void Player::draw(SpriteRenderer &renderer, const Texture2D &texture, float camX,
                  const glm::vec4 &tint) const
{
    // Frame atual: coluna = quantos ciclos de frameTime já passaram
    int col = static_cast<int>(mAnimTimer / mAnim.frameTime) % mAnim.frameCount;

    glm::vec2 texSize(static_cast<float>(texture.width()), static_cast<float>(texture.height()));
    glm::vec2 uvOff = uvOffsetFor(size, texSize, col, mAnim.row);
    glm::vec2 uvScale = uvScaleFor(size, texSize);

    // Espelhar o sprite horizontalmente quando olha para a esquerda
    glm::vec2 drawnSize = facingRight ? size : glm::vec2(-size.x, size.y);

    // A câmera entra como deslocamento em X (o mundo desliza para a esquerda)
    renderer.draw(texture, spriteModel(glm::vec2(position.x - camX, position.y), drawnSize),
                  uvOff, uvScale, tint);
}
