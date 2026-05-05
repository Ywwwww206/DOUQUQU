#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QTimer>
#include <QSlider>
#include <QPushButton>
#include <QDragEnterEvent>
#include <QPixmap>
#include "ball.h"
#include "audioprocessor.h"
#include "collisionsound.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;

private slots:
    void onMapAnimationStep();
    void onMapAnimationFinished();
    void onAddBall();              // 添加小球
    void onSelectSkin();          // 选择皮肤图片
    void updatePhysics();         // 物理更新 + 碰撞检测

private:
    void setupScene();
    void setupUI();
    void startAudioFile(const QString &filePath);
    void handleCollisions();      // 检测并处理所有小球碰撞
    void applySkinToAll();        // 将当前皮肤应用到所有小球

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

    QString m_pendingFilePath;
    bool m_mapReady = false;
    QPixmap m_globalSkin;          // 全局皮肤图片
};

#endif // MAINWINDOW_H