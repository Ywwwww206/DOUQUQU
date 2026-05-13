#include "bosspage.h"
#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <cmath>
#include <QRandomGenerator>
#include <QFontMetrics>
#include <QPainterPath>

// 辅助宏：键盘状态检测（因Qt没有直接获取全局按键状态的简单方法，使用成员变量记录）
#define KEY_PRESSED(key) (m_keysPressed.contains(key))

BossPage::BossPage(QWidget *parent) : QWidget(parent),
    m_gameTimer(new QTimer(this)),
    m_spawnTimer(new QTimer(this)),
    m_regenTimer(new QTimer(this)),
    m_forceRegenCounter(0),
    m_keyA(false), m_keyS(false), m_wallDirLeft(false), m_wallDirRight(false),
    m_wallAngle(0), m_dragging(false), m_healthUsed(false), m_healthRegenTimer(0),
    m_attackBuff(false), m_speedBuff(false), m_breakBuff(false),
    m_attackBuffTimer(0), m_speedBuffTimer(0), m_breakBuffTimer(0),
    m_wallCount(10), m_forceCount(10), m_healthCount(10)
{
    setFixedSize(VIEW_WIDTH, VIEW_HEIGHT);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    // 擂台区域：正方形居中，占一半屏幕高度（约600x600）
    m_arenaRect = QRectF((VIEW_WIDTH - ARENA_SIZE)/2, (VIEW_HEIGHT - ARENA_SIZE)/2 - 50, ARENA_SIZE, ARENA_SIZE);

    // Boss血条标签（顶部居中加粗）
    m_bossHpLabel = new QLabel(this);
    m_bossHpLabel->setAlignment(Qt::AlignCenter);
    m_bossHpLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: red; background-color: rgba(0,0,0,180); padding: 5px;");
    m_bossHpLabel->setGeometry((VIEW_WIDTH - 300)/2, 20, 300, 40);

    // 玩家血条标签（底部居中）
    m_playerHpLabel = new QLabel(this);
    m_playerHpLabel->setAlignment(Qt::AlignCenter);
    m_playerHpLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: red; background-color: rgba(0,0,0,180); padding: 3px;");
    m_playerHpLabel->setGeometry((VIEW_WIDTH - 200)/2, VIEW_HEIGHT - 60, 200, 30);

    // 退出按钮
    m_backBtn = new QPushButton("退出", this);
    m_backBtn->setGeometry(VIEW_WIDTH - 100, 20, 80, 30);
    connect(m_backBtn, &QPushButton::clicked, this, &BossPage::backToMenu);

    // 道具栏区域（左侧）
    m_itemBarRect = QRect(20, VIEW_HEIGHT - 140, 200, 120);

    // 定时器设置
    m_gameTimer->setInterval(16); // ~60 FPS
    connect(m_gameTimer, &QTimer::timeout, this, &BossPage::gameUpdate);

    m_spawnTimer->setInterval(10000);
    connect(m_spawnTimer, &QTimer::timeout, this, &BossPage::spawnRandomItem);

    m_regenTimer->setInterval(1000);
    connect(m_regenTimer, &QTimer::timeout, this, &BossPage::healthRegen);

    // 初始化玩家属性
    m_player.radius = 20;
    m_player.maxHp = 100;
    m_player.hp = 100;
    m_player.attack = 25;
    m_player.speed = 10.0;
    m_player.hasImage = false;
}

void BossPage::setBossId(int id)
{
    m_boss.id = id;
    initBoss();
}

void BossPage::setPlayerImage(const QPixmap &img)
{
    m_player.image = img;
    m_player.hasImage = true;
}

void BossPage::startBattle()
{
    initArena();
    m_gameTimer->start();
    m_spawnTimer->start();
    m_regenTimer->start();
    setFocus();
    update();
}

void BossPage::initArena()
{
    m_player.pos = QPointF(m_arenaRect.left() + 100, m_arenaRect.center().y());
    m_player.vel = QPointF(0, 0);
    m_player.hp = m_player.maxHp;

    m_boss.pos = m_arenaRect.center();
    m_boss.hp = m_boss.maxHp;
    m_boss.faceRed = QRandomGenerator::global()->bounded(4);
    m_boss.skillState = 0;
    m_boss.skillTimer = 0;
    m_boss.invincible = false;
    m_boss.reflectDamage = false;

    m_walls.clear();
    m_items.clear();
    m_wallCount = 10;
    m_forceCount = 10;
    m_healthCount = 10;
    m_attackBuff = m_speedBuff = m_breakBuff = false;
    m_attackBuffTimer = m_speedBuffTimer = m_breakBuffTimer = 0;
    m_healthUsed = false;
    m_healthRegenTimer = 0;
    m_forceRegenCounter = 0;
    m_keysPressed.clear();
}

void BossPage::initBoss()
{
    m_boss.size = 60;
    m_boss.maxHp = 1000;
    m_boss.hp = 1000;
    m_boss.faceRed = 0;
    m_boss.skillState = 0;
    m_boss.skillTimer = 0;
    m_boss.invincible = false;
    m_boss.reflectDamage = false;
}
void BossPage::gameUpdate()
{
    // 胜负判定
    if (m_player.hp <= 0 || m_boss.hp <= 0) {
        m_gameTimer->stop();
        m_spawnTimer->stop();
        m_regenTimer->stop();
        update();
        return;
    }

    // 玩家移动
    updatePlayerMovement();

    // Boss AI
    updateBossAI();

    // 碰撞检测
    checkCollisions();

    // Buff计时
    if (m_attackBuff) {
        if (--m_attackBuffTimer <= 0) m_attackBuff = false;
    }
    if (m_speedBuff) {
        if (--m_speedBuffTimer <= 0) m_speedBuff = false;
    }
    if (m_breakBuff) {
        if (--m_breakBuffTimer <= 0) m_breakBuff = false;
    }

    // 回血包计时
    if (m_healthUsed) {
        if (--m_healthRegenTimer <= 0) {
            m_player.hp = m_player.maxHp;
            m_healthUsed = false;
        }
    }

    // 更新UI文字
    m_bossHpLabel->setText(QString("Boss HP: %1 / %2").arg(m_boss.hp).arg(m_boss.maxHp));
    m_playerHpLabel->setText(QString("HP: %1 / %2").arg(m_player.hp).arg(m_player.maxHp));

    update();
}

void BossPage::updatePlayerMovement()
{
    qreal speed = m_player.speed * (m_speedBuff ? 2.0 : 1.0);
    QPointF move(0, 0);
    if (m_keysPressed.contains(Qt::Key_W) || m_keysPressed.contains(Qt::Key_Up))    move.ry() -= 1;
    if (m_keysPressed.contains(Qt::Key_S) || m_keysPressed.contains(Qt::Key_Down))  move.ry() += 1;
    if (m_keysPressed.contains(Qt::Key_A) || m_keysPressed.contains(Qt::Key_Left))  move.rx() -= 1;
    if (m_keysPressed.contains(Qt::Key_D) || m_keysPressed.contains(Qt::Key_Right)) move.rx() += 1;

    if (!move.isNull()) {
        qreal len = std::sqrt(move.x()*move.x() + move.y()*move.y());
        move /= len;
        m_player.pos += move * speed;
    }

    // 边界限制
    m_player.pos.setX(qBound(m_arenaRect.left() + m_player.radius, m_player.pos.x(), m_arenaRect.right() - m_player.radius));
    m_player.pos.setY(qBound(m_arenaRect.top() + m_player.radius, m_player.pos.y(), m_arenaRect.bottom() - m_player.radius));
}

void BossPage::spawnRandomItem()
{
    int type = QRandomGenerator::global()->bounded(6); // 0:墙,1:力,2:血,3:攻,4:速,5:破
    qreal x = m_arenaRect.left() + QRandomGenerator::global()->bounded(m_arenaRect.width() - 40) + 20;
    qreal y = m_arenaRect.top() + QRandomGenerator::global()->bounded(m_arenaRect.height() - 40) + 20;
    m_items.push_back({static_cast<Item::Type>(type), QPointF(x, y), true});
}

void BossPage::healthRegen()
{
    // 玩家每秒回复1点
    if (!m_healthUsed && m_player.hp < m_player.maxHp) {
        m_player.hp = qMin(m_player.maxHp, m_player.hp + 1);
    }
    // 力道具每5秒回复1个
    if (++m_forceRegenCounter >= 5) {
        m_forceRegenCounter = 0;
        if (m_forceCount < 10) m_forceCount++;
    }
}

void BossPage::useWall()
{
    if (m_wallCount <= 0) return;
    qreal length = m_arenaRect.width() / 5.0;
    QPointF dir;
    if (m_wallAngle == 0) dir = QPointF(length, 0);
    else dir = QPointF(0, length);
    QLineF line(m_wallStartPoint, m_wallStartPoint + dir);
    // 简单碰撞检查：不与现有墙重合或相交（此处略去详细检测，确保基本功能）
    bool valid = true;
    for (const Wall &w : m_walls) {
        QPointF intersectionPoint;
        if (w.line.intersects(line, &intersectionPoint) == QLineF::BoundedIntersection) {
            valid = false;
            break;
        }
    }
    if (valid) {
        m_walls.push_back({line, true});
        m_wallCount--;
    }
}

void BossPage::useForce(const QPointF &from, const QPointF &to)
{
    if (m_forceCount <= 0) return;
    QPointF delta = to - from;
    qreal dist = std::sqrt(delta.x()*delta.x() + delta.y()*delta.y());
    if (dist < 5) return;
    qreal speed = qBound(10.0, dist / 5.0, 20.0);
    QPointF dir = delta / dist;
    m_player.vel = dir * speed;
    m_forceCount--;
}

void BossPage::useHealth()
{
    if (m_healthCount <= 0 || m_healthUsed) return;
    m_healthUsed = true;
    m_healthRegenTimer = 60; // 1秒后回满（60fps）
    m_healthCount--;
}
void BossPage::updateBossAI()
{
    // 技能冷却递减
    if (m_boss.skillTimer > 0) {
        m_boss.skillTimer--;
    }

    // 当前有技能正在执行
    if (m_boss.skillState != 0) {
        switch (m_boss.skillState) {
        case 1: // 冲锋一
            if (m_boss.skillTimer == 0) {
                // 停顿结束，开始冲锋
                m_boss.chargeDir = (m_boss.chargeTarget - m_boss.pos);
                qreal len = std::sqrt(m_boss.chargeDir.x()*m_boss.chargeDir.x() + m_boss.chargeDir.y()*m_boss.chargeDir.y());
                if (len > 0) m_boss.chargeDir /= len;
                m_boss.chargeSpeed = 30.0;
                m_boss.skillState = 10; // 冲锋中
                m_boss.skillTimer = 120; // 最大冲锋时间
            }
            break;
        case 10: // 冲锋一执行中
            m_boss.pos += m_boss.chargeDir * m_boss.chargeSpeed;
            // 边界反弹（暂时忽略障碍物）
            if (m_boss.pos.x() - m_boss.size/2 < m_arenaRect.left() || m_boss.pos.x() + m_boss.size/2 > m_arenaRect.right()) {
                m_boss.chargeDir.setX(-m_boss.chargeDir.x());
            }
            if (m_boss.pos.y() - m_boss.size/2 < m_arenaRect.top() || m_boss.pos.y() + m_boss.size/2 > m_arenaRect.bottom()) {
                m_boss.chargeDir.setY(-m_boss.chargeDir.y());
            }
            if (--m_boss.skillTimer <= 0) {
                m_boss.skillState = 0;
            }
            break;
        case 2: // 旋转
            m_boss.reflectDamage = true;
            m_boss.invincible = true;
            // 旋转动画（改变faceRed）
            m_boss.faceRed = (m_boss.faceRed + 1) % 4;
            if (--m_boss.skillTimer <= 0) {
                m_boss.skillState = 0;
                m_boss.reflectDamage = false;
                m_boss.invincible = false;
            }
            break;
        case 3: // 冲锋二准备
            if (m_boss.skillTimer == 0) {
                // 确定方向：红色面的对面灰色面指向玩家
                int oppositeFace = (m_boss.faceRed + 2) % 4;
                QPointF dir;
                switch (oppositeFace) {
                case 0: dir = QPointF(0, -1); break; // 上
                case 1: dir = QPointF(1, 0); break;  // 右
                case 2: dir = QPointF(0, 1); break;  // 下
                case 3: dir = QPointF(-1, 0); break; // 左
                }
                m_boss.chargeDir = dir;
                m_boss.chargeSpeed = 40.0;
                m_boss.skillState = 30; // 冲锋二执行中
                m_boss.skillTimer = 300; // 最多5秒
            }
            break;
        case 30: // 冲锋二执行中
            m_boss.pos += m_boss.chargeDir * m_boss.chargeSpeed;
            m_boss.chargeSpeed = qMax(0.0, m_boss.chargeSpeed - 4.0/60.0); // 每帧减速4/60
            // 边界反弹
            if (m_boss.pos.x() - m_boss.size/2 < m_arenaRect.left()) {
                m_boss.pos.setX(m_arenaRect.left() + m_boss.size/2);
                m_boss.chargeDir.setX(-m_boss.chargeDir.x());
            } else if (m_boss.pos.x() + m_boss.size/2 > m_arenaRect.right()) {
                m_boss.pos.setX(m_arenaRect.right() - m_boss.size/2);
                m_boss.chargeDir.setX(-m_boss.chargeDir.x());
            }
            if (m_boss.pos.y() - m_boss.size/2 < m_arenaRect.top()) {
                m_boss.pos.setY(m_arenaRect.top() + m_boss.size/2);
                m_boss.chargeDir.setY(-m_boss.chargeDir.y());
            } else if (m_boss.pos.y() + m_boss.size/2 > m_arenaRect.bottom()) {
                m_boss.pos.setY(m_arenaRect.bottom() - m_boss.size/2);
                m_boss.chargeDir.setY(-m_boss.chargeDir.y());
            }
            if (m_boss.chargeSpeed <= 0 || --m_boss.skillTimer <= 0) {
                m_boss.skillState = 0;
            }
            break;
        }
        // 技能执行期间不进行新技能选择
        return;
    }

    // 无技能时：普通行为——红色面转向玩家
    QPointF toPlayer = m_player.pos - m_boss.pos;
    qreal angle = std::atan2(toPlayer.y(), toPlayer.x()) * 180.0 / M_PI;
    int newFace = 0;
    if (angle >= -45 && angle < 45) newFace = 1;      // 右
    else if (angle >= 45 && angle < 135) newFace = 2;  // 下
    else if (angle >= 135 || angle < -135) newFace = 3; // 左
    else newFace = 0;                                  // 上
    m_boss.faceRed = newFace;

    // 随机选择技能（每10秒大约一次机会，通过计时器控制）
    static int skillCooldown = 0;
    if (skillCooldown > 0) {
        skillCooldown--;
    } else {
        int r = QRandomGenerator::global()->bounded(100);
        if (m_boss.hp < m_boss.maxHp/2 && r < 30) {
            // 技能三
            m_boss.skillState = 3;
            m_boss.skillTimer = 120; // 2秒准备（60fps）
            skillCooldown = 600;      // 10秒冷却
        } else if (r < 40) {
            // 技能一
            m_boss.skillState = 1;
            m_boss.skillTimer = 60;  // 1秒停顿
            m_boss.chargeTarget = m_player.pos;
            skillCooldown = 600;
        } else if (r < 70) {
            // 技能二
            m_boss.skillState = 2;
            m_boss.skillTimer = 180; // 3秒旋转
            skillCooldown = 600;
        }
        // 否则不释放技能
    }
}
void BossPage::checkCollisions()
{
    QRectF playerRect(m_player.pos.x() - m_player.radius, m_player.pos.y() - m_player.radius,
                      m_player.radius*2, m_player.radius*2);
    QRectF bossRect(m_boss.pos.x() - m_boss.size/2, m_boss.pos.y() - m_boss.size/2,
                    m_boss.size, m_boss.size);

    // 玩家与Boss碰撞（仅当Boss处于冲锋状态时造成伤害）
    if (playerRect.intersects(bossRect)) {
        if (m_boss.skillState == 10 || m_boss.skillState == 30) {
            // 造成50伤害
            m_player.hp = qMax(0, m_player.hp - 50);
            // 弹开玩家
            QPointF dir = m_player.pos - m_boss.pos;
            qreal len = std::sqrt(dir.x()*dir.x() + dir.y()*dir.y());
            if (len > 0) {
                dir /= len;
                m_player.pos = m_boss.pos + dir * (m_boss.size/2 + m_player.radius + 5);
            }
        }
        // 如果玩家攻击Boss（通过攻击键，这里简化为每次碰撞玩家都攻击）
        // 实际应设计攻击键，此处简化：只要接触且非技能状态，玩家对Boss造成伤害
        if (m_boss.skillState == 0) {
            int damage = m_player.attack * (m_attackBuff ? 2 : 1);
            // 判断攻击到哪个面
            QPointF rel = m_player.pos - m_boss.pos;
            int faceHit = -1;
            // 简单判定：根据相对角度
            qreal angle = std::atan2(rel.y(), rel.x()) * 180.0 / M_PI;
            if (angle >= -45 && angle < 45) faceHit = 1;      // 右
            else if (angle >= 45 && angle < 135) faceHit = 2;  // 下
            else if (angle >= 135 || angle < -135) faceHit = 3; // 左
            else faceHit = 0;                                  // 上

            if (m_boss.reflectDamage) {
                // 反弹伤害给玩家
                m_player.hp = qMax(0, m_player.hp - damage);
            } else if (!m_boss.invincible) {
                if (faceHit == m_boss.faceRed) {
                    damage *= 2; // 红色面双倍
                } else {
                    damage /= 2; // 灰色面减半
                }
                m_boss.hp = qMax(0, m_boss.hp - damage);
                // 如果玩家有“破”buff，则眩晕Boss
                if (m_breakBuff) {
                    m_boss.skillState = 0;
                    m_boss.skillTimer = 600; // 10秒眩晕
                    m_boss.chargeSpeed = 0;
                    m_breakBuff = false; // 一次性效果
                }
            }
        }
    }

    // 玩家与墙体碰撞
    for (const Wall &w : m_walls) {
        if (!w.placed) continue;
        // 线段与圆的碰撞检测
        QLineF line = w.line;
        QPointF closest;
        qreal t;
        qreal dx = line.dx();
        qreal dy = line.dy();
        qreal len2 = dx*dx + dy*dy;
        if (len2 == 0) {
            closest = line.p1();
        } else {
            t = ((m_player.pos.x() - line.x1()) * dx + (m_player.pos.y() - line.y1()) * dy) / len2;
            t = qBound(0.0, t, 1.0);
            closest = line.pointAt(t);
        }
        QPointF diff = m_player.pos - closest;
        qreal dist = std::sqrt(diff.x()*diff.x() + diff.y()*diff.y());
        if (dist < m_player.radius) {
            QPointF normal = diff / dist;
            m_player.pos = closest + normal * m_player.radius;
            // 反弹速度
            qreal dot = QPointF::dotProduct(m_player.vel, normal);
            m_player.vel = m_player.vel - 2 * dot * normal;
        }
    }

    // 拾取道具
    for (Item &item : m_items) {
        if (!item.active) continue;
        QRectF itemRect(item.pos.x() - 15, item.pos.y() - 15, 30, 30);
        if (playerRect.intersects(itemRect)) {
            applyItemEffect(item.type);
            item.active = false;
        }
    }
}

void BossPage::applyItemEffect(Item::Type type)
{
    switch (type) {
    case Item::WallItem:
        m_wallCount = qMin(m_wallCount + 1, 99);
        break;
    case Item::ForceItem:
        m_forceCount = qMin(m_forceCount + 1, 99);
        break;
    case Item::HealthItem:
        m_healthCount = qMin(m_healthCount + 1, 99);
        break;
    case Item::AttackUp:
        m_attackBuff = true;
        m_attackBuffTimer = 600; // 10秒 (60fps)
        break;
    case Item::SpeedUp:
        m_speedBuff = true;
        m_speedBuffTimer = 600;
        break;
    case Item::BreakItem:
        m_breakBuff = true;
        m_breakBuffTimer = 600;
        break;
    default: break;
    }
}
void BossPage::keyPressEvent(QKeyEvent *e)
{
    m_keysPressed.insert(e->key());

    if (e->key() == Qt::Key_A && !e->isAutoRepeat()) {
        m_keyA = true;
        m_wallStartPoint = m_player.pos;
        m_wallAngle = 0;
    }
    if (e->key() == Qt::Key_S && !e->isAutoRepeat()) {
        m_keyS = true;
    }
    if (m_keyA) {
        if (e->key() == Qt::Key_Left) m_wallDirLeft = true;
        if (e->key() == Qt::Key_Right) m_wallDirRight = true;
        // 方向调整
        if (m_wallDirLeft) m_wallAngle = 90;
        if (m_wallDirRight) m_wallAngle = 0;
    }
    if (e->key() == Qt::Key_H && m_healthCount > 0 && !m_healthUsed) {
        useHealth();
    }

    QWidget::keyPressEvent(e);
}

void BossPage::keyReleaseEvent(QKeyEvent *e)
{
    m_keysPressed.remove(e->key());

    if (e->key() == Qt::Key_A && !e->isAutoRepeat()) {
        m_keyA = false;
        if (m_wallCount > 0) {
            useWall();
        }
        m_wallDirLeft = m_wallDirRight = false;
    }
    if (e->key() == Qt::Key_S && !e->isAutoRepeat()) {
        m_keyS = false;
        m_dragging = false;
    }
    if (e->key() == Qt::Key_Left) m_wallDirLeft = false;
    if (e->key() == Qt::Key_Right) m_wallDirRight = false;

    QWidget::keyReleaseEvent(e);
}

void BossPage::mousePressEvent(QMouseEvent *e)
{
    if (m_keyS && e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStart = e->pos();
        m_dragCurrent = e->pos();
    }
}

void BossPage::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging) {
        m_dragCurrent = e->pos();
        update();
    }
}

void BossPage::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_dragging && e->button() == Qt::LeftButton) {
        m_dragging = false;
        if (m_forceCount > 0) {
            useForce(m_dragStart, m_dragCurrent);
        }
        update();
    }
}

void BossPage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 全黑背景
    painter.fillRect(rect(), Qt::black);

    // 擂台区域（灰色）
    painter.fillRect(m_arenaRect, QColor(40, 40, 40));
    painter.setPen(QPen(Qt::white, 3));
    painter.drawRect(m_arenaRect);

    // 绘制墙体
    drawWalls(painter);

    // 绘制道具
    drawItems(painter);

    // 绘制Boss
    drawBoss(painter);

    // 绘制玩家
    drawPlayer(painter);

    // 绘制UI（道具栏、血条已在QLabel中，此处绘制额外信息）
    drawUI(painter);

    // 绘制蓄力拖拽指示线
    if (m_dragging) {
        painter.setPen(QPen(Qt::yellow, 2, Qt::DashLine));
        painter.drawLine(m_dragStart, m_dragCurrent);
    }

    // 绘制墙体预览
    if (m_keyA && m_wallCount > 0) {
        qreal length = m_arenaRect.width() / 5.0;
        QPointF dir;
        if (m_wallAngle == 0) dir = QPointF(length, 0);
        else dir = QPointF(0, length);
        QLineF preview(m_wallStartPoint, m_wallStartPoint + dir);
        painter.setPen(QPen(Qt::green, 2, Qt::DashLine));
        painter.drawLine(preview);
    }
}

void BossPage::drawWalls(QPainter &p)
{
    p.setPen(QPen(Qt::gray, 4));
    for (const Wall &w : m_walls) {
        if (w.placed) p.drawLine(w.line);
    }
}

void BossPage::drawItems(QPainter &p)
{
    for (const Item &item : m_items) {
        if (!item.active) continue;
        QColor color;
        QString label;
        switch (item.type) {
        case Item::WallItem: color = Qt::darkGray; label = "墙"; break;
        case Item::ForceItem: color = Qt::cyan; label = "力"; break;
        case Item::HealthItem: color = Qt::red; label = "血"; break;
        case Item::AttackUp: color = Qt::magenta; label = "攻"; break;
        case Item::SpeedUp: color = Qt::green; label = "速"; break;
        case Item::BreakItem: color = Qt::yellow; label = "破"; break;
        }
        p.setBrush(color);
        p.setPen(Qt::white);
        p.drawEllipse(item.pos, 12, 12);
        p.drawText(QRectF(item.pos.x()-15, item.pos.y()-15, 30, 30), Qt::AlignCenter, label);
    }
}

void BossPage::drawPlayer(QPainter &p)
{
    if (m_player.hasImage) {
        p.save();
        QRectF target(m_player.pos.x() - m_player.radius, m_player.pos.y() - m_player.radius,
                      m_player.radius*2, m_player.radius*2);
        QPainterPath path;
        path.addEllipse(target);
        p.setClipPath(path);
        p.drawPixmap(target.toRect(), m_player.image);
        p.restore();
    } else {
        p.setBrush(Qt::red);
        p.setPen(Qt::NoPen);
        p.drawEllipse(m_player.pos, m_player.radius, m_player.radius);
    }
    // 玩家血条已在QLabel显示，此处不再绘制
}

void BossPage::drawBoss(QPainter &p)
{
    QRectF bossRect(m_boss.pos.x() - m_boss.size/2, m_boss.pos.y() - m_boss.size/2,
                    m_boss.size, m_boss.size);
    // 绘制四个面
    QColor gray(100, 100, 100);
    QColor red(255, 0, 0);
    // 上边
    p.fillRect(bossRect.x(), bossRect.y(), bossRect.width(), 5, (m_boss.faceRed == 0) ? red : gray);
    // 右边
    p.fillRect(bossRect.right()-5, bossRect.y(), 5, bossRect.height(), (m_boss.faceRed == 1) ? red : gray);
    // 下边
    p.fillRect(bossRect.x(), bossRect.bottom()-5, bossRect.width(), 5, (m_boss.faceRed == 2) ? red : gray);
    // 左边
    p.fillRect(bossRect.x(), bossRect.y(), 5, bossRect.height(), (m_boss.faceRed == 3) ? red : gray);
    // 填充中心
    p.fillRect(bossRect.adjusted(5,5,-5,-5), QColor(0, 150, 0));
    // 绘制边框
    p.setPen(QPen(Qt::white, 2));
    p.drawRect(bossRect);
}

void BossPage::drawUI(QPainter &p)
{
    // 道具栏背景
    p.fillRect(m_itemBarRect, QColor(30, 30, 30, 200));
    p.setPen(Qt::white);
    p.drawRect(m_itemBarRect);

    // 显示道具数量
    QFont font = p.font();
    font.setPointSize(12);
    p.setFont(font);
    int y = m_itemBarRect.top() + 25;
    p.drawText(m_itemBarRect.left() + 10, y, QString("墙 (A): %1").arg(m_wallCount));
    y += 20;
    p.drawText(m_itemBarRect.left() + 10, y, QString("力 (S): %1").arg(m_forceCount));
    y += 20;
    p.drawText(m_itemBarRect.left() + 10, y, QString("血 (H): %1").arg(m_healthCount));

    // 显示Buff状态
    y += 30;
    if (m_attackBuff) p.drawText(m_itemBarRect.left() + 10, y, "攻击力x2");
    if (m_speedBuff) p.drawText(m_itemBarRect.left() + 10, y+20, "速度x2");
    if (m_breakBuff) p.drawText(m_itemBarRect.left() + 10, y+40, "破甲");
}