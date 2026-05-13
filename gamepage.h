#ifndef GAMEPAGE_H
#define GAMEPAGE_H

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QPointF>
#include <QRandomGenerator>
#include <QLabel>
#include <QPushButton>
#include <QLineF>
#include <cmath>
#include <random>
#include <algorithm>

class GamePage : public QWidget
{
    Q_OBJECT
public:
    explicit GamePage(QWidget *parent = nullptr);
    void setCharacters(int playerId, int opponentId);
    void setMap(int type, const QRect &arenaRect);
    void startBattle();

signals:
    void restartRequested();
    void windowSizeNeeded(QSize size);  // 新增：通知主窗口调整大小

protected:
    void paintEvent(QPaintEvent *) override;

private slots:
    void updateGame();
    void updateTraps();
    void onRestart();

private:
    struct Character {
        int id;
        QString name;
        int hp;
        int maxHp;
        int attack;
        qreal speed;
        int attackCooldownMs;
        QPointF pos;
        QPointF velocity;
        qreal radius;
        int collisionDamageCooldown;
        int attackTimer;
    };

    enum MapType { Classic, Maze, Trap };
    MapType m_mapType;
    QRect m_arenaRect;

    Character m_player;
    Character m_enemy;

    QTimer *m_timer;
    QTimer *m_trapTimer;
    bool m_gameOver;
    QString m_winnerText;
    QLabel *m_winnerLabel;
    QPushButton *m_restartBtn;

    QList<QRect> m_walls;
    QList<QPoint> m_trapCells;
    QList<bool> m_trapRedState;

    static std::mt19937 s_randomEngine;

    void initCharacters();
    void initMap();
    void generateMazeWalls();
    void checkCollision();
    void handleBoundary(Character &c);
    void handleCollisionWithWalls(Character &c);
    void checkGameOver();
    void drawWalls(QPainter &painter);
    void drawTraps(QPainter &painter);
    void drawCharacter(QPainter &painter, const Character &c);
};

#endif // GAMEPAGE_H