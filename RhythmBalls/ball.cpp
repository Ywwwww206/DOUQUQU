#include "ball.h"
#include <QGraphicsScene>
#include <QtMath>
#include <QPainter>

Ball::Ball(qreal x, qreal y, qreal radius, const QColor &color)
    : QGraphicsEllipseItem(-radius, -radius, radius * 2, radius * 2),
    m_radius(radius)
{
    setPos(x, y);
    setBrush(color);
    setPen(Qt::NoPen);
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

void Ball::setBounds(const QRectF &rect)
{
    m_bounds = rect;
}

void Ball::setSkin(const QPixmap &pixmap)
{
    if (pixmap.isNull()) return;
    m_skin = pixmap.scaled(static_cast<int>(m_radius*2), static_cast<int>(m_radius*2),
                           Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_hasSkin = true;
    setBrush(QBrush(m_skin));
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

void Ball::advance(int phase)
{
    if (!phase) return;

    QRectF bound = m_bounds.isValid() ? m_bounds :
                       QRectF(0, 0, scene()->width(), scene()->height() * 0.9);

    m_vy += m_gravity;
    qreal newX = x() + m_vx;
    qreal newY = y() + m_vy;

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
}