#include "ball.h"
#include <QGraphicsScene>
#include <QtMath>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QFontMetrics>

Ball::Ball(qreal x, qreal y, qreal radius, const QColor &color)
    : QGraphicsEllipseItem(),
    m_radius(radius), m_color(color)
{
    setPos(x, y);
    setPen(Qt::NoPen);
    // 注意：不再在构造函数中调用 setRect，因为我们用自定义 boundingRect
}

// ---------- 自定义裁剪与形状 ----------
QRectF Ball::boundingRect() const
{
    // 基础球体矩形
    QRectF base(-m_radius, -m_radius, m_radius*2, m_radius*2);
    // 文字区域向上扩展（字体14号加粗，高度约18像素，描边偏移）
    int textExtra = 22;
    return base.adjusted(-2, -textExtra, 2, 2);  // 左右也稍扩大，避免描边被切
}

QPainterPath Ball::shape() const
{
    QPainterPath path;
    path.addEllipse(-m_radius, -m_radius, m_radius*2, m_radius*2);
    return path;
}

// ---------- 运动 ----------
void Ball::setVelocity(qreal vx, qreal vy)
{
    m_vx = vx;
    m_vy = vy;
}

void Ball::addImpulse(qreal vx, qreal vy)
{
    m_vx += vx;
    m_vy += vy;
    update();        // 确保运动后的血量显示（虽然移动本身会触发重绘）
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

// ---------- 战斗 ----------
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
        m_hp = 0;
        m_dead = true;
    }
    update();        // 血量变化时强制重绘
}

void Ball::heal(int)
{
    m_hp = 1000;
    update();
}

void Ball::applySpeedBuff(float multiplier, int durationMs)
{
    m_speedMultiplier = multiplier;
    m_speedBuffTimer = durationMs;
}

// ---------- 物理更新 ----------
void Ball::advance(int phase)
{
    if (!phase) return;

    QRectF bound = m_bounds.isValid() ? m_bounds :
                       QRectF(0, 0, scene()->width(), scene()->height() * 0.9);

    qreal vx = m_vx * m_speedMultiplier;
    qreal vy = m_vy * m_speedMultiplier;
    qreal newX = x() + vx;
    qreal newY = y() + vy;

    if (newY + m_radius > bound.bottom()) {
        newY = bound.bottom() - m_radius;
        m_vy = -m_vy * m_bounce;
    } else if (newY - m_radius < bound.top()) {
        newY = bound.top() + m_radius;
        m_vy = -m_vy * m_bounce;
    }
    if (newX - m_radius < bound.left()) {
        newX = bound.left() + m_radius;
        m_vx = -m_vx * m_bounce;
    } else if (newX + m_radius > bound.right()) {
        newX = bound.right() - m_radius;
        m_vx = -m_vx * m_bounce;
    }

    setPos(newX, newY);

    if (m_speedBuffTimer > 0) {
        m_speedBuffTimer -= 16;
        if (m_speedBuffTimer <= 0) {
            m_speedMultiplier = 1.0f;
            m_speedBuffTimer = 0;
        }
    }
}

// ---------- 绘制 ----------
void Ball::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHint(QPainter::Antialiasing);

    // 1. 绘制球体
    QRectF ballRect(-m_radius, -m_radius, m_radius*2, m_radius*2);
    if (m_hasSkin) {
        QPainterPath clip;
        clip.addEllipse(ballRect);
        painter->setClipPath(clip);
        painter->drawPixmap(ballRect.toRect(), m_skin);
        painter->setClipping(false);
    } else {
        painter->setBrush(m_color);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(ballRect);
    }

    // 2. 绘制血量（带描边）
    QFont font("Arial", 14, QFont::Bold);
    painter->setFont(font);
    QString hpText = QString::number(m_hp);
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(hpText);
    int textHeight = fm.height();
    QRectF textRect(-textWidth/2, -m_radius - textHeight - 4, textWidth, textHeight);

    // 黑色描边
    painter->setPen(Qt::black);
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if (dx != 0 || dy != 0)
                painter->drawText(textRect.adjusted(dx, dy, 0, 0), Qt::AlignCenter, hpText);

    // 白色文字
    painter->setPen(Qt::white);
    painter->drawText(textRect, Qt::AlignCenter, hpText);
}