#include "ball.h"
#include <QGraphicsScene>
#include <QtMath>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

Ball::Ball(qreal x, qreal y, qreal radius, const QColor &color)
    : QGraphicsEllipseItem(-radius, -radius, radius * 2, radius * 2),
    m_radius(radius), m_color(color)
{
    setPos(x, y);
    setPen(Qt::NoPen);
    // 不再使用 setBrush，由 paint() 负责绘制
}

void Ball::setVelocity(qreal vx, qreal vy)
{
    m_vx = vx;
    m_vy = vy;
}

void Ball::applyImpulse(qreal impulseY)
{
    m_vy = -impulseY;
}

void Ball::addImpulse(qreal vx, qreal vy)
{
    m_vx += vx;
    m_vy += vy;
}

void Ball::setBounds(const QRectF &rect)
{
    m_bounds = rect;
}

void Ball::setSkin(const QPixmap &pixmap)
{
    if (pixmap.isNull()) return;
    m_skin = pixmap.scaled(static_cast<int>(m_radius*2), static_cast<int>(m_radius*2),
                           Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    m_hasSkin = true;
    update();
}

int Ball::computeAttack() const
{
    const qreal MAX_SPEED = 15.0;
    qreal s = qMin(speed(), MAX_SPEED);
    int attack = 10 + static_cast<int>((s / MAX_SPEED) * 90);
    return qBound(10, attack, 100);
}

qreal Ball::speed() const
{
    return qSqrt(m_vx * m_vx + m_vy * m_vy);
}

void Ball::takeDamage(int damage)
{
    m_hp -= damage;
    if (m_hp <= 0) {
        m_dead = true;
    }
}

void Ball::heal(int amount)
{
    m_hp = qMin(m_hp + amount, 1000);
}

void Ball::applySpeedBuff(float multiplier, int durationMs)
{
    m_speedMultiplier = multiplier;
    m_speedBuffTimer = durationMs;
}

void Ball::advance(int phase)
{
    if (!phase) return;

    QRectF bound = m_bounds.isValid() ? m_bounds :
                       QRectF(0, 0, scene()->width(), scene()->height() * 0.9);

    // 无重力，直接使用当前速度（可享受buff加速）
    qreal vx = m_vx * m_speedMultiplier;
    qreal vy = m_vy * m_speedMultiplier;
    qreal newX = x() + vx;
    qreal newY = y() + vy;

    // 底部碰撞
    if (newY + m_radius > bound.bottom()) {
        newY = bound.bottom() - m_radius;
        m_vy = -m_vy * m_bounce;
        if (qAbs(m_vy) < 0.1) m_vy = 0;
    }
    // 顶部碰撞
    if (newY - m_radius < bound.top()) {
        newY = bound.top() + m_radius;
        m_vy = -m_vy * m_bounce;
    }
    // 左右碰撞
    if (newX - m_radius < bound.left()) {
        newX = bound.left() + m_radius;
        m_vx = -m_vx * m_bounce;
    }
    if (newX + m_radius > bound.right()) {
        newX = bound.right() - m_radius;
        m_vx = -m_vx * m_bounce;
    }

    setPos(newX, newY);

    // Buff 时间衰减（每帧约 16ms）
    if (m_speedBuffTimer > 0) {
        m_speedBuffTimer -= 16;
        if (m_speedBuffTimer <= 0) {
            m_speedMultiplier = 1.0f;
            m_speedBuffTimer = 0;
        }
    }
}

void Ball::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHint(QPainter::Antialiasing);
    QRectF rect(-m_radius, -m_radius, m_radius*2, m_radius*2);

    if (m_hasSkin) {
        // 使用圆形裁剪路径确保皮肤不会出现四分
        QPainterPath path;
        path.addEllipse(rect);
        painter->setClipPath(path);
        painter->drawPixmap(rect.toRect(), m_skin);
        painter->setClipping(false);
    } else {
        painter->setBrush(m_color);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(rect);
    }
}