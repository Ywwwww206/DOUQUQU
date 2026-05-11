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
#include <QStackedWidget>
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

private slots:
    void startGame();               // 点击封面开始按钮
    void onMapAnimationStep();
    void onMapAnimationFinished();
    void onAddBall();
    void onSelectSkin();
    void updatePhysics();

private:
    void setupScene();
    void setupUI();
    void startAudioFile(const QString &filePath);
    void handleCollisions();
    void applySkinToAll();

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

    QString m_pendingFilePath;
    bool m_mapReady = false;
    QPixmap m_globalSkin;
};

#endif // MAINWINDOW_H