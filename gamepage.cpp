#include "gamepage.h"
#include <QDebug>

std::mt19937 GamePage::s_randomEngine(std::random_device{}());

GamePage::GamePage(QWidget *parent) : QWidget(parent),
    m_timer(new QTimer(this)), m_trapTimer(nullptr),
    m_gameOver(false), m_restartBtn(nullptr)
{
    m_winnerLabel = new QLabel(this);
    m_winnerLabel->setAlignment(Qt::AlignCenter);
    m_winnerLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: white; background-color: rgba(0,0,0,128);");
    m_winnerLabel->hide();

    connect(m_timer, &QTimer::timeout, this, &GamePage::updateGame);
}

void GamePage::setCharacters(int playerId, int opponentId)
{
    m_player.id = playerId;
    if (playerId == 0) {
        m_player.name = "红色圆形";
        m_player.hp = 100;
        m_player.attack = 25;
        m_player.speed = 20.0;
        m_player.attackCooldownMs = 0;
        m_player.radius = 20;
    } else {
        m_player.name = "蓝色三角形";
        m_player.hp = 80;
        m_player.attack = 20;
        m_player.speed = 5.0;
        m_player.attackCooldownMs = 5000;
        m_player.radius = 25;
    }
    m_player.maxHp = m_player.hp;
    m_player.collisionDamageCooldown = 0;
    m_player.attackTimer = 0;

    m_enemy.id = opponentId;
    if (opponentId == 0) {
        m_enemy.name = "红色圆形";
        m_enemy.hp = 100;
        m_enemy.attack = 25;
        m_enemy.speed = 20.0;
        m_enemy.attackCooldownMs = 0;
        m_enemy.radius = 20;
    } else {
        m_enemy.name = "蓝色三角形";
        m_enemy.hp = 80;
        m_enemy.attack = 20;
        m_enemy.speed = 5.0;
        m_enemy.attackCooldownMs = 5000;
        m_enemy.radius = 25;
    }
    m_enemy.maxHp = m_enemy.hp;
    m_enemy.collisionDamageCooldown = 0;
    m_enemy.attackTimer = 0;
}

void GamePage::setMap(int type, const QRect &arenaRect)
{
    m_mapType = static_cast<MapType>(type);
    m_arenaRect = arenaRect;
    // 计算所需窗口大小：擂台矩形最大坐标 + 边距
    int margin = 50;
    QSize neededSize(arenaRect.right() + margin, arenaRect.bottom() + margin);
    setFixedSize(neededSize);
    emit windowSizeNeeded(neededSize);  // 通知主窗口调整

    initMap();
}

void GamePage::initMap()
{
    m_walls.clear();
    m_trapCells.clear();

    if (m_mapType == Maze) {
        generateMazeWalls();
    } else if (m_mapType == Trap) {
        int cellSize = 40;
        for (int x = m_arenaRect.left(); x < m_arenaRect.right(); x += cellSize) {
            for (int y = m_arenaRect.top(); y < m_arenaRect.bottom(); y += cellSize) {
                m_trapCells.append(QPoint(x + cellSize/2, y + cellSize/2));
            }
        }
        m_trapRedState = QList<bool>(m_trapCells.size(), false);
        if (m_trapTimer) delete m_trapTimer;
        m_trapTimer = new QTimer(this);
        m_trapTimer->setInterval(10000);
        connect(m_trapTimer, &QTimer::timeout, this, &GamePage::updateTraps);
    }
}

void GamePage::generateMazeWalls()
{
    m_walls.clear();
    int w = m_arenaRect.width();
    int h = m_arenaRect.height();
    int left = m_arenaRect.left();
    int top = m_arenaRect.top();

    m_walls.append(QRect(left + w/4, top + h/4, 30, 80));
    m_walls.append(QRect(left + w/2 - 15, top + h/3, 80, 30));
    m_walls.append(QRect(left + w*3/4 - 30, top + h*2/3, 30, 80));
    m_walls.append(QRect(left + w/3, top + h*2/3 - 15, 100, 30));
}

void GamePage::startBattle()
{
    initCharacters();
    m_gameOver = false;
    m_winnerLabel->hide();
    if (m_restartBtn) m_restartBtn->hide();
    m_timer->start(50);
    if (m_trapTimer) m_trapTimer->start();
}

void GamePage::initCharacters()
{
    m_player.pos = QPointF(m_arenaRect.left() + 80, m_arenaRect.top() + 80);
    m_enemy.pos = QPointF(m_arenaRect.right() - 80, m_arenaRect.bottom() - 80);

    auto randomDir = []() -> QPointF {
        qreal angle = QRandomGenerator::global()->bounded(2 * M_PI);
        return QPointF(cos(angle), sin(angle));
    };
    m_player.velocity = randomDir() * m_player.speed;
    m_enemy.velocity = randomDir() * m_enemy.speed;

    m_player.attackTimer = 0;
    m_enemy.attackTimer = 0;
    m_player.collisionDamageCooldown = 0;
    m_enemy.collisionDamageCooldown = 0;
}

void GamePage::updateGame()
{
    if (m_gameOver) return;

    m_player.pos += m_player.velocity;
    m_enemy.pos += m_enemy.velocity;

    handleBoundary(m_player);
    handleBoundary(m_enemy);
    handleCollisionWithWalls(m_player);
    handleCollisionWithWalls(m_enemy);

    if (m_player.attackCooldownMs > 0) {
        m_player.attackTimer += 50;
        if (m_player.attackTimer >= m_player.attackCooldownMs) {
            m_player.attackTimer = 0;
            if (m_player.id == 1) {
                m_enemy.hp -= m_player.attack;
                if (m_enemy.hp < 0) m_enemy.hp = 0;
            }
        }
    }
    if (m_enemy.attackCooldownMs > 0) {
        m_enemy.attackTimer += 50;
        if (m_enemy.attackTimer >= m_enemy.attackCooldownMs) {
            m_enemy.attackTimer = 0;
            if (m_enemy.id == 1) {
                m_player.hp -= m_enemy.attack;
                if (m_player.hp < 0) m_player.hp = 0;
            }
        }
    }

    if (m_player.collisionDamageCooldown > 0) m_player.collisionDamageCooldown--;
    if (m_enemy.collisionDamageCooldown > 0) m_enemy.collisionDamageCooldown--;

    checkCollision();
    checkGameOver();

    update();
}

void GamePage::handleBoundary(Character &c)
{
    if (c.pos.x() - c.radius < m_arenaRect.left()) {
        c.pos.setX(m_arenaRect.left() + c.radius);
        c.velocity.setX(-c.velocity.x());
    } else if (c.pos.x() + c.radius > m_arenaRect.right()) {
        c.pos.setX(m_arenaRect.right() - c.radius);
        c.velocity.setX(-c.velocity.x());
    }
    if (c.pos.y() - c.radius < m_arenaRect.top()) {
        c.pos.setY(m_arenaRect.top() + c.radius);
        c.velocity.setY(-c.velocity.y());
    } else if (c.pos.y() + c.radius > m_arenaRect.bottom()) {
        c.pos.setY(m_arenaRect.bottom() - c.radius);
        c.velocity.setY(-c.velocity.y());
    }
}

void GamePage::handleCollisionWithWalls(Character &c)
{
    for (const QRect &wall : m_walls) {
        QRectF wallF = wall;
        QPointF closest = wallF.center();
        if (c.pos.x() < wallF.left()) closest.setX(wallF.left());
        else if (c.pos.x() > wallF.right()) closest.setX(wallF.right());
        if (c.pos.y() < wallF.top()) closest.setY(wallF.top());
        else if (c.pos.y() > wallF.bottom()) closest.setY(wallF.bottom());

        QLineF line(c.pos, closest);
        if (line.length() < c.radius) {
            QPointF normal = line.p2() - line.p1();
            normal /= line.length();
            c.pos = closest - normal * c.radius;
            qreal dot = QPointF::dotProduct(c.velocity, normal);
            c.velocity = c.velocity - 2 * dot * normal;
        }
    }
}

void GamePage::checkCollision()
{
    qreal dist = QLineF(m_player.pos, m_enemy.pos).length();
    qreal threshold = m_player.radius + m_enemy.radius;
    if (dist < threshold) {
        QPointF delta = m_enemy.pos - m_player.pos;
        delta /= dist;
        qreal overlap = threshold - dist;
        m_player.pos -= delta * (overlap / 2);
        m_enemy.pos += delta * (overlap / 2);

        QPointF v1 = m_player.velocity;
        QPointF v2 = m_enemy.velocity;
        m_player.velocity = v2;
        m_enemy.velocity = v1;

        auto applyDamage = [](Character &attacker, Character &victim) {
            if (attacker.attackCooldownMs == 0 && attacker.collisionDamageCooldown == 0) {
                victim.hp -= attacker.attack;
                if (victim.hp < 0) victim.hp = 0;
                attacker.collisionDamageCooldown = 10;
            }
        };
        applyDamage(m_player, m_enemy);
        applyDamage(m_enemy, m_player);
    }
}

void GamePage::updateTraps()
{
    QList<int> indices;
    for (int i = 0; i < m_trapCells.size(); ++i) indices.append(i);
    std::shuffle(indices.begin(), indices.end(), s_randomEngine);
    int half = m_trapCells.size() / 2;
    for (int i = 0; i < m_trapCells.size(); ++i) {
        m_trapRedState[i] = (i < half);
    }

    int cellSize = 40;
    for (int i = 0; i < m_trapCells.size(); ++i) {
        if (!m_trapRedState[i]) continue;
        QPoint cellCenter = m_trapCells[i];
        QRect cellRect(cellCenter.x() - cellSize/2, cellCenter.y() - cellSize/2, cellSize, cellSize);
        if (cellRect.contains(m_player.pos.toPoint())) {
            m_player.hp -= 10;
            if (m_player.hp < 0) m_player.hp = 0;
        }
        if (cellRect.contains(m_enemy.pos.toPoint())) {
            m_enemy.hp -= 10;
            if (m_enemy.hp < 0) m_enemy.hp = 0;
        }
    }
    update();
}

void GamePage::checkGameOver()
{
    if (m_player.hp <= 0 || m_enemy.hp <= 0) {
        m_gameOver = true;
        m_timer->stop();
        if (m_trapTimer) m_trapTimer->stop();
        m_winnerText = (m_player.hp <= 0) ? m_enemy.name + " 胜利！" : m_player.name + " 胜利！";
        m_winnerLabel->setText(m_winnerText);
        m_winnerLabel->setGeometry(m_arenaRect);
        m_winnerLabel->show();

        if (!m_restartBtn) {
            m_restartBtn = new QPushButton("重新开始", this);
            m_restartBtn->setGeometry(m_arenaRect.center().x() - 50, m_arenaRect.center().y() + 50, 100, 40);
            connect(m_restartBtn, &QPushButton::clicked, this, &GamePage::onRestart);
        }
        m_restartBtn->show();
    }
}

void GamePage::onRestart()
{
    m_timer->stop();
    if (m_trapTimer) m_trapTimer->stop();
    emit restartRequested();
}

void GamePage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), Qt::black);
    painter.fillRect(m_arenaRect, QColor(30, 30, 30));
    painter.setPen(QPen(Qt::white, 3));
    painter.drawRect(m_arenaRect);

    drawWalls(painter);
    drawTraps(painter);
    drawCharacter(painter, m_player);
    drawCharacter(painter, m_enemy);
}

void GamePage::drawWalls(QPainter &painter)
{
    painter.setBrush(Qt::darkGray);
    painter.setPen(Qt::NoPen);
    for (const QRect &wall : m_walls) {
        painter.drawRect(wall);
    }
}

void GamePage::drawTraps(QPainter &painter)
{
    if (m_mapType != Trap) return;
    int cellSize = 40;
    for (int i = 0; i < m_trapCells.size(); ++i) {
        QPoint center = m_trapCells[i];
        QRect cellRect(center.x() - cellSize/2, center.y() - cellSize/2, cellSize, cellSize);
        if (m_trapRedState.value(i, false)) {
            painter.fillRect(cellRect, QColor(255, 0, 0, 100));
        }
        painter.setPen(QPen(Qt::gray, 1));
        painter.drawRect(cellRect);
    }
}

void GamePage::drawCharacter(QPainter &painter, const Character &c)
{
    int barWidth = 80;
    int barHeight = 10;
    qreal healthPercent = (qreal)c.hp / c.maxHp;
    QPointF barPos(c.pos.x() - barWidth/2, c.pos.y() - c.radius - 15);
    painter.fillRect(barPos.x(), barPos.y(), barWidth, barHeight, Qt::darkGray);
    painter.fillRect(barPos.x(), barPos.y(), barWidth * healthPercent, barHeight,
                     (c.id == 0) ? Qt::red : Qt::blue);
    painter.setPen(Qt::white);
    painter.drawText(barPos.x(), barPos.y() - 5, QString("%1/%2").arg(c.hp).arg(c.maxHp));

    painter.setPen(Qt::NoPen);
    if (c.id == 0) {
        painter.setBrush(Qt::red);
        painter.drawEllipse(c.pos, c.radius, c.radius);
    } else {
        painter.setBrush(Qt::blue);
        QPolygonF triangle;
        qreal r = c.radius;
        triangle << QPointF(c.pos.x(), c.pos.y() - r)
                 << QPointF(c.pos.x() + r * 0.866, c.pos.y() + r * 0.5)
                 << QPointF(c.pos.x() - r * 0.866, c.pos.y() + r * 0.5);
        painter.drawPolygon(triangle);
    }
}