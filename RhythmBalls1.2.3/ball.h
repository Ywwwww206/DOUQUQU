#ifndef BALL_H
#define BALL_H

#include <QGraphicsEllipseItem>
#include <QRectF>
#include <QPixmap>
#include <QColor>

class Ball : public QGraphicsEllipseItem
{
public:
    Ball(qreal x, qreal y, qreal radius, const QColor &color);

    void setVelocity(qreal vx, qreal vy);
    void applyImpulse(qreal impulseY);          // 保留，但不再使用
    void addImpulse(qreal vx, qreal vy);        // 向速度叠加冲量
    void setBounds(const QRectF &rect);

    void setSkin(const QPixmap &pixmap);
    int computeAttack() const;
    void takeDamage(int damage);
    int hp() const { return m_hp; }
    qreal speed() const;
    QPointF vel() const { return QPointF(m_vx, m_vy); }
    void setDead() { m_dead = true; }
    bool isDead() const { return m_dead; }

    void heal(int amount);
    void applySpeedBuff(float multiplier, int durationMs);

protected:
    void advance(int phase) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    qreal m_vx = 0;
    qreal m_vy = 0;
    qreal m_radius;
    qreal m_gravity = 0.0;      // 移除重力
    qreal m_bounce = 0.85;
    QRectF m_bounds;
    QColor m_color;

    int m_hp = 1000;
    QPixmap m_skin;
    bool m_hasSkin = false;
    bool m_dead = false;

    float m_speedMultiplier = 1.0f;
    int m_speedBuffTimer = 0;
};

#endif // BALL_H