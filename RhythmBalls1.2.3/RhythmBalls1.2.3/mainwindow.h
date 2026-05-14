#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QTimer>
#include <QSlider>
#include <QPushButton>
#include <QDragEnterEvent>
#include <QKeyEvent>
#include <QPixmap>
#include <QStackedWidget>
#include <QElapsedTimer>
#include "ball.h"
#include "audioprocessor.h"
#include "collisionsound.h"
#include "coverwidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void startGame();
    void onMapAnimationStep();
    void onMapAnimationFinished();
    void onAddBall();
    void onSelectSkin();
    void updatePhysics();
    void spawnItem();
    void toggleEditMode();

private:
    void setupScene();
    void setupUI();
    void startAudioFile(const QString &filePath);
    void handleCollisions();
    void handleWallCollisions();
    void applySkinToAll();
    void checkItemPickups();
    void createDamageText(const QPointF &pos, int damage);
    void clearDamageTexts();

    QStackedWidget *m_stackedWidget;
    CoverWidget *m_coverWidget;

    QGraphicsScene *m_scene;
    QGraphicsView *m_view;
    QGraphicsRectItem *m_mapRect = nullptr;

    QTimer *m_mapAnimTimer = nullptr;
    QRectF m_targetMapRect;
    int m_mapAnimStep = 0;
    static const int MapAnimTotalSteps = 60;

    QTimer *m_physicsTimer = nullptr;
    QRectF m_bounds;

    AudioProcessor *m_audioProcessor = nullptr;
    QPushButton *m_soundBtn = nullptr;
    CollisionSound *m_collisionSound = nullptr;

    QSlider *m_volumeSlider = nullptr;
    QPushButton *m_openBtn = nullptr;
    QPushButton *m_addBallBtn = nullptr;
    QPushButton *m_skinBtn = nullptr;
    QPushButton *m_editMapBtn = nullptr;

    QString m_pendingFilePath;
    bool m_mapReady = false;
    QPixmap m_globalSkin;

    QTimer *m_itemTimer = nullptr;
    QList<QGraphicsItem*> m_itemList;

    // 地图编辑
    bool m_editMode = false;
    QGraphicsLineItem *m_previewWall = nullptr;
    qreal m_wallLength = 100;
    int m_wallAngle = 0;
    QPointF m_wallPosition;
    QList<QGraphicsLineItem*> m_walls;
    QGraphicsTextItem *m_editHint = nullptr;

    // 伤害跳字
    struct DamageText {
        QGraphicsTextItem *text;
        QElapsedTimer timer;
        QPointF worldPos;
    };
    QList<DamageText> m_damageTexts;
};

#endif // MAINWINDOW_H