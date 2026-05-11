#ifndef BALL_H
#define BALL_H

#include <QGraphicsEllipseItem>
#include <QRectF>
#include <QPixmap>

class Ball : public QGraphicsEllipseItem
{
public:
    Ball(qreal x, qreal y, qreal radius, const QColor &color);

    void setVelocity(qreal vx, qreal vy);
    void applyImpulse(qreal impulseY);
    void setBounds(const QRectF &rect);

    void setSkin(const QPixmap &pixmap);
    int computeAttack() const;
    void takeDamage(int damage);
    int hp() const { return m_hp; }
    qreal speed() const;
    QPointF vel() const { return QPointF(m_vx, m_vy); }
    void setDead() { m_dead = true; }
    bool isDead() const { return m_dead; }

protected:
    void advance(int phase) override;

private:
    qreal m_vx = 0;
    qreal m_vy = 0;
    qreal m_radius;
    qreal m_gravity = 0.4;       // 降低重力，让弹跳更持久
    qreal m_bounce = 0.85;       // 提高边界反弹系数
    QRectF m_bounds;

    int m_hp = 1000;
    QPixmap m_skin;
    bool m_hasSkin = false;
    bool m_dead = false;
};

#endif // BALL_H