#ifndef BALL_H
#define BALL_H

#include <QGraphicsEllipseItem>
#include <QRectF>
#include <QPixmap>
#include <QColor>
#include <QPainterPath>

class Ball : public QGraphicsEllipseItem
{
public:
    Ball(qreal x, qreal y, qreal radius, const QColor &color);

    // 运动
    void setVelocity(qreal vx, qreal vy);
    void addImpulse(qreal vx, qreal vy);
    void setBounds(const QRectF &rect);

    // 外观
    void setSkin(const QPixmap &pixmap);

    // 战斗
    int computeAttack() const;
    void takeDamage(int damage);
    void heal(int amount);          // 参数忽略，直接回满
    void applySpeedBuff(float multiplier, int durationMs);

    // 状态
    int hp() const { return m_hp; }
    qreal speed() const;
    QPointF vel() const { return QPointF(m_vx, m_vy); }
    void setDead() { m_dead = true; }
    bool isDead() const { return m_dead; }

    // 覆盖绘制相关函数
    QRectF boundingRect() const override;
    QPainterPath shape() const override;

protected:
    void advance(int phase) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    qreal m_vx = 0;
    qreal m_vy = 0;
    qreal m_radius;
    qreal m_gravity = 0.0;
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