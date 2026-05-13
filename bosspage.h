#ifndef BOSSPAGE_H
#define BOSSPAGE_H

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QLineF>
#include <QPixmap>
#include <QSet>
#include <vector>

class BossPage : public QWidget
{
    Q_OBJECT
public:
    explicit BossPage(QWidget *parent = nullptr);

    void setBossId(int id);
    void setPlayerImage(const QPixmap &img);
    void startBattle();

signals:
    void backToMenu();

protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private slots:
    void gameUpdate();
    void spawnRandomItem();
    void healthRegen();

private:
    // 擂台参数
    static constexpr int VIEW_WIDTH = 1000;
    static constexpr int VIEW_HEIGHT = 800;
    static constexpr int ARENA_SIZE = 600;

    // 玩家结构
    struct Player {
        QPointF pos;
        QPointF vel;
        qreal radius = 20;
        int hp = 100;
        int maxHp = 100;
        int attack = 25;
        qreal speed = 10.0;
        QPixmap image;
        bool hasImage = false;
    };

    // Boss结构
    struct Boss {
        int id = 0;             // 0:神龟
        QPointF pos;
        qreal size = 60;        // 正方形边长
        int hp = 1000;
        int maxHp = 1000;
        int faceRed = 0;        // 0:上,1:右,2:下,3:左
        // 技能状态
        int skillState = 0;     // 0:待机,1:冲锋一准备,10:冲锋一执行,2:旋转,3:冲锋二准备,30:冲锋二执行
        int skillTimer = 0;
        QPointF chargeTarget;
        QPointF chargeDir;
        qreal chargeSpeed = 0;
        bool invincible = false;
        bool reflectDamage = false;
    };

    // 墙体道具
    struct Wall {
        QLineF line;
        bool placed = true;
    };

    // 掉落道具
    struct Item {
        enum Type { WallItem, ForceItem, HealthItem, AttackUp, SpeedUp, BreakItem };
        Type type;
        QPointF pos;
        bool active = true;
    };

    // 核心数据
    QRectF m_arenaRect;
    QRect m_itemBarRect;
    Player m_player;
    Boss m_boss;

    std::vector<Wall> m_walls;
    std::vector<Item> m_items;

    int m_wallCount;
    int m_forceCount;
    int m_healthCount;
    int m_forceRegenCounter;

    // 玩家Buff
    bool m_attackBuff;
    bool m_speedBuff;
    bool m_breakBuff;
    int m_attackBuffTimer;
    int m_speedBuffTimer;
    int m_breakBuffTimer;

    // 键盘/鼠标状态
    QSet<int> m_keysPressed;
    bool m_keyA;
    bool m_keyS;
    bool m_wallDirLeft;
    bool m_wallDirRight;
    int m_wallAngle;                // 0:水平, 90:垂直
    QPointF m_wallStartPoint;
    bool m_dragging;
    QPointF m_dragStart;
    QPointF m_dragCurrent;
    bool m_healthUsed;
    int m_healthRegenTimer;

    // UI元素
    QLabel *m_bossHpLabel;
    QLabel *m_playerHpLabel;
    QPushButton *m_backBtn;

    // 定时器
    QTimer *m_gameTimer;
    QTimer *m_spawnTimer;
    QTimer *m_regenTimer;

    // 私有方法
    void initArena();
    void initBoss();
    void updatePlayerMovement();
    void updateBossAI();
    void checkCollisions();
    void useWall();
    void useForce(const QPointF &from, const QPointF &to);
    void useHealth();
    void applyItemEffect(Item::Type type);
    void drawWalls(QPainter &p);
    void drawItems(QPainter &p);
    void drawPlayer(QPainter &p);
    void drawBoss(QPainter &p);
    void drawUI(QPainter &p);
};

#endif // BOSSPAGE_H