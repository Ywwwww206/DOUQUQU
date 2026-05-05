#include "mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include <QRandomGenerator>
#include <QGraphicsScene>
#include <QList>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("节奏小球 - 纯净版");
    resize(800, 600);

    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, 800, 600);
    m_scene->setBackgroundBrush(QColor(40, 40, 50));
    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setFrameShape(QFrame::NoFrame);
    setCentralWidget(m_view);

    setupUI();

    m_mapRect = new QGraphicsRectItem();
    m_mapRect->setPen(QPen(Qt::black, 3));
    m_mapRect->setBrush(Qt::white);
    m_mapRect->setPos(100, 50);
    m_mapRect->setRect(0, 0, 0, 0);
    m_scene->addItem(m_mapRect);

    m_targetMapRect = QRectF(0, 0, 600, 450);
    m_mapAnimTimer = new QTimer(this);
    connect(m_mapAnimTimer, &QTimer::timeout, this, &MainWindow::onMapAnimationStep);
    m_mapAnimStep = 0;
    m_mapAnimTimer->start(30);

    m_collisionSound = new CollisionSound(this);
}

MainWindow::~MainWindow()
{
    if (m_audioProcessor)
        m_audioProcessor->stop();
}

void MainWindow::setupUI()
{
    // 打开音乐按钮
    m_openBtn = new QPushButton("选择音乐", this);
    m_openBtn->setGeometry(620, 20, 120, 30);
    m_openBtn->setStyleSheet(
        "QPushButton { background: #e94560; color: white; border: none; border-radius: 5px; }"
        "QPushButton:hover { background: #ff6b7a; }");
    connect(m_openBtn, &QPushButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(this, "选择音频",
                                                        "", "音频文件 (*.mp3 *.wav *.m4a *.flac *.ogg *.aac)");
        if (!filePath.isEmpty()) {
            m_pendingFilePath = filePath;
            if (m_mapReady)
                startAudioFile(filePath);
        }
    });
    //撞击音
    m_soundBtn = new QPushButton("选择撞击音", this);
    m_soundBtn->setGeometry(620, 180, 120, 30);
    connect(m_soundBtn, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, "选择撞击音效", "",
                                                    "音频文件 (*.mp3 *.wav *.m4a *.ogg)");
        if (!file.isEmpty()) {
            // 用 QAudioDecoder 解码整个文件为 PCM（单声道44100Hz16bit）
            QAudioDecoder *decoder = new QAudioDecoder(this);
            QAudioFormat fmt;
            fmt.setSampleRate(44100);
            fmt.setChannelCount(1);
            fmt.setSampleFormat(QAudioFormat::Int16);
            decoder->setAudioFormat(fmt);
            decoder->setSource(QUrl::fromLocalFile(file));

            QByteArray *pcm = new QByteArray;
            connect(decoder, &QAudioDecoder::bufferReady, this, [decoder, pcm]() {
                auto buf = decoder->read();
                pcm->append(buf.constData<char>(), buf.byteCount());
            });
            connect(decoder, &QAudioDecoder::finished, this, [this, pcm, fmt]() {
                if (!pcm->isEmpty())
                    m_collisionSound->setCustomSound(*pcm, fmt);
                delete pcm;
            });
            decoder->start();
        }
    });
    // 音量滑块
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setGeometry(620, 60, 120, 25);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(30);
    m_volumeSlider->setStyleSheet(
        "QSlider::groove:horizontal { background: #555; height: 6px; border-radius: 3px; }"
        "QSlider::handle:horizontal { background: white; width: 14px; margin: -4px 0; border-radius: 7px; }");
    m_volumeSlider->setVisible(false);
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (m_audioProcessor)
            m_audioProcessor->setMusicVolume(value / 100.0);
    });

    // 【新】添加小球按钮
    m_addBallBtn = new QPushButton("添加小球", this);
    m_addBallBtn->setGeometry(620, 100, 120, 30);
    m_addBallBtn->setStyleSheet(
        "QPushButton { background: #4a90e2; color: white; border: none; border-radius: 5px; }"
        "QPushButton:hover { background: #6aaef5; }");
    m_addBallBtn->setVisible(false);
    connect(m_addBallBtn, &QPushButton::clicked, this, &MainWindow::onAddBall);

    // 【新】选择皮肤按钮
    m_skinBtn = new QPushButton("选择皮肤", this);
    m_skinBtn->setGeometry(620, 140, 120, 30);
    m_skinBtn->setStyleSheet(
        "QPushButton { background: #50c878; color: white; border: none; border-radius: 5px; }"
        "QPushButton:hover { background: #70e898; }");
    m_skinBtn->setVisible(false);
    connect(m_skinBtn, &QPushButton::clicked, this, &MainWindow::onSelectSkin);
}

void MainWindow::onMapAnimationStep()
{
    m_mapAnimStep++;
    qreal progress = qMin(1.0, m_mapAnimStep / (qreal)MapAnimTotalSteps);
    qreal eased = 1.0 - (1.0 - progress) * (1.0 - progress);
    qreal w = m_targetMapRect.width() * eased;
    qreal h = m_targetMapRect.height() * eased;
    m_mapRect->setRect(0, 0, w, h);

    if (m_mapAnimStep >= MapAnimTotalSteps) {
        m_mapAnimTimer->stop();
        onMapAnimationFinished();
    }
}

void MainWindow::onMapAnimationFinished()
{
    m_mapReady = true;
    m_mapRect->setRect(m_targetMapRect);

    qreal border = 3;
    m_bounds = QRectF(m_mapRect->pos().x() + border,
                      m_mapRect->pos().y() + border,
                      m_targetMapRect.width() - 2 * border,
                      m_targetMapRect.height() - 2 * border);

    // 创建第一个球
    Ball *ball = new Ball(m_bounds.center().x(), m_bounds.top() + 50, 18,
                          QColor(255, 100, 80));
    ball->setBounds(m_bounds);
    ball->setVelocity(0.5, 0);
    m_scene->addItem(ball);

    // 显示功能按钮
    m_volumeSlider->setVisible(true);
    m_addBallBtn->setVisible(true);
    m_skinBtn->setVisible(true);

    // 启动物理定时器（60fps）
    m_physicsTimer = new QTimer(this);
    connect(m_physicsTimer, &QTimer::timeout, this, &MainWindow::updatePhysics);
    m_physicsTimer->start(16);

    if (!m_pendingFilePath.isEmpty())
        startAudioFile(m_pendingFilePath);
}

// 统一物理更新 + 碰撞检测
void MainWindow::updatePhysics()
{
    // 1. 所有球自由运动
    m_scene->advance();

    // 2. 碰撞处理
    handleCollisions();

    // 3. 清理死亡的小球
    QList<Ball*> deadBalls;
    const auto items = m_scene->items();
    for (auto *item : items) {
        if (auto *ball = dynamic_cast<Ball*>(item)) {
            if (ball->isDead())
                deadBalls.append(ball);
        }
    }
    for (auto *ball : deadBalls) {
        m_scene->removeItem(ball);
        delete ball;
    }
}

// 碰撞检测与伤害
void MainWindow::handleCollisions()
{
    QList<QGraphicsItem*> items = m_scene->items();
    QList<Ball*> balls;
    for (auto *item : items) {
        if (auto *ball = dynamic_cast<Ball*>(item)) {
            balls.append(ball);
        }
    }

    for (int i = 0; i < balls.size(); ++i) {
        Ball *a = balls[i];
        for (int j = i + 1; j < balls.size(); ++j) {
            Ball *b = balls[j];
            // 检查是否重叠（简单圆形碰撞）
            qreal dx = a->x() - b->x();
            qreal dy = a->y() - b->y();
            qreal dist = qSqrt(dx*dx + dy*dy);
            qreal minDist = a->boundingRect().width()/2 + b->boundingRect().width()/2; // 半径和
            if (dist < minDist && dist > 0) {
                // 计算相对速度在连线方向上的投影，判断是否正在靠近
                QPointF va = a->vel();
                QPointF vb = b->vel();
                QPointF relVel = va - vb;
                qreal vn = relVel.x()*(dx/dist) + relVel.y()*(dy/dist);
                if (vn > 0) {  // 正在相互靠近
                    // 伤害计算（各自攻击力）
                    int atkA = a->computeAttack();
                    int atkB = b->computeAttack();
                    a->takeDamage(atkB);
                    b->takeDamage(atkA);

                    // 简单弹性碰撞（分开并交换连线方向速度分量）
                    // 归一化方向向量
                    QPointF n(dx/dist, dy/dist);
                    qreal ma = 1.0, mb = 1.0; // 假设质量相同
                    qreal v1n = va.x()*n.x() + va.y()*n.y();
                    qreal v2n = vb.x()*n.x() + vb.y()*n.y();
                    qreal v1n_after = (v1n*(ma-mb) + 2*mb*v2n) / (ma+mb);
                    qreal v2n_after = (v2n*(mb-ma) + 2*ma*v1n) / (ma+mb);

                    // 更新速度（只改变法向分量，切向不变）
                    QPointF v1t = va - n*v1n;
                    QPointF v2t = vb - n*v2n;
                    QPointF newVa = v1t + n*v1n_after;
                    QPointF newVb = v2t + n*v2n_after;
                    a->setVelocity(newVa.x(), newVa.y());
                    b->setVelocity(newVb.x(), newVb.y());

                    // 位置修正（防止黏连）
                    qreal overlap = minDist - dist;
                    a->setPos(a->pos() + n * (overlap/2));
                    b->setPos(b->pos() - n * (overlap/2));
                }
            }
        }
    }
}

// 添加小球
void MainWindow::onAddBall()
{
    if (!m_mapReady) return;
    qreal radius = 18;
    qreal x = m_bounds.left() + radius + QRandomGenerator::global()->bounded(m_bounds.width() - 2*radius);
    qreal y = m_bounds.top() + 50;
    Ball *ball = new Ball(x, y, radius, QColor(QRandomGenerator::global()->bounded(256),
                                               QRandomGenerator::global()->bounded(256),
                                               QRandomGenerator::global()->bounded(256)));
    ball->setBounds(m_bounds);
    ball->setVelocity(QRandomGenerator::global()->bounded(3.0)-1.5, 0);
    // 应用全局皮肤
    if (!m_globalSkin.isNull())
        ball->setSkin(m_globalSkin);
    m_scene->addItem(ball);
}

// 选择皮肤图片
void MainWindow::onSelectSkin()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择小球皮肤",
                                                    "", "图片 (*.png *.jpg *.jpeg *.bmp)");
    if (!filePath.isEmpty()) {
        m_globalSkin.load(filePath);
        applySkinToAll();
    }
}

void MainWindow::applySkinToAll()
{
    QList<QGraphicsItem*> items = m_scene->items();
    for (auto *item : items) {
        if (auto *ball = dynamic_cast<Ball*>(item)) {
            ball->setSkin(m_globalSkin);
        }
    }
}

void MainWindow::startAudioFile(const QString &filePath)
{
    qDebug() << "[MainWindow] 开始播放:" << filePath;
    if (m_audioProcessor) {
        m_audioProcessor->stop();
        delete m_audioProcessor;
    }
    m_audioProcessor = new AudioProcessor(this);
    m_audioProcessor->setMusicVolume(m_volumeSlider->value() / 100.0);

    connect(m_audioProcessor, &AudioProcessor::beatDetected, this, [this]() {
        // 节拍冲击：对所有存活的球施加随机向上冲量
        QList<QGraphicsItem*> items = m_scene->items();
        for (auto *item : items) {
            if (auto *ball = dynamic_cast<Ball*>(item)) {
                qreal impulse = 14 + QRandomGenerator::global()->bounded(8.0);
                ball->applyImpulse(impulse);
            }
        }
        m_collisionSound->play();
    });

    m_audioProcessor->loadAndPlay(filePath);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->ignore();
}