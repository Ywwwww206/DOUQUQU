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

    // 新增
    void setSkin(const QPixmap &pixmap);   // 设置图片皮肤
    int computeAttack() const;             // 根据速度计算攻击力
    void takeDamage(int damage);           // 受到伤害
    int hp() const { return m_hp; }
    qreal speed() const;                   // 当前速率
    QPointF vel() const { return QPointF(m_vx, m_vy); }
    void setDead() { m_dead = true; }
    bool isDead() const { return m_dead; }

protected:
    void advance(int phase) override;

private:
    qreal m_vx = 0;
    qreal m_vy = 0;
    qreal m_radius;
    qreal m_gravity = 0.6;
    qreal m_bounce = 0.82;
    QRectF m_bounds;

    int m_hp = 1000;                       // 血量
    QPixmap m_skin;                        // 皮肤图片
    bool m_hasSkin = false;
    bool m_dead = false;   // 标记是否待删除
};

#endif // BALL_H