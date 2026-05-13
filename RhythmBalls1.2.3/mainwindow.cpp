#include "mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include <QRandomGenerator>
#include <QGraphicsScene>
#include <QList>
#include <QResizeEvent>
#include "circlecropdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("节奏小球 - 纯净版");
    resize(800, 600);

    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    m_coverWidget = new CoverWidget(this);
    m_stackedWidget->addWidget(m_coverWidget);

    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, 800, 600);
    m_scene->setBackgroundBrush(QColor(40, 40, 50));
    m_view = new QGraphicsView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setFrameShape(QFrame::NoFrame);
    m_stackedWidget->addWidget(m_view);

    m_stackedWidget->setCurrentWidget(m_coverWidget);

    setupUI();

    m_mapRect = new QGraphicsRectItem();
    m_mapRect->setPen(QPen(Qt::black, 3));
    m_mapRect->setBrush(Qt::white);
    m_mapRect->setPos(100, 75);
    m_mapRect->setRect(0, 0, 0, 0);
    m_scene->addItem(m_mapRect);

    m_targetMapRect = QRectF(0, 0, 600, 450);
    m_mapAnimTimer = new QTimer(this);
    connect(m_mapAnimTimer, &QTimer::timeout, this, &MainWindow::onMapAnimationStep);
    m_mapAnimStep = 0;

    m_collisionSound = new CollisionSound(this);

    connect(m_coverWidget, &CoverWidget::startClicked, this, &MainWindow::startGame);
}

MainWindow::~MainWindow()
{
    if (m_audioProcessor)
        m_audioProcessor->stop();
}

void MainWindow::startGame()
{
    m_stackedWidget->setCurrentWidget(m_view);

    m_openBtn->show();
    m_soundBtn->show();
    m_volumeSlider->show();
    m_addBallBtn->show();
    m_skinBtn->show();

    m_mapAnimTimer->start(30);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    m_scene->setSceneRect(0, 0, m_view->width(), m_view->height());
}

void MainWindow::setupUI()
{
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
    m_openBtn->hide();

    m_soundBtn = new QPushButton("选择撞击音", this);
    m_soundBtn->setGeometry(620, 180, 120, 30);
    connect(m_soundBtn, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, "选择撞击音效", "",
                                                    "音频文件 (*.mp3 *.wav *.m4a *.ogg)");
        if (!file.isEmpty()) {
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
    m_soundBtn->hide();

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setGeometry(620, 60, 120, 25);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(30);
    m_volumeSlider->setStyleSheet(
        "QSlider::groove:horizontal { background: #555; height: 6px; border-radius: 3px; }"
        "QSlider::handle:horizontal { background: white; width: 14px; margin: -4px 0; border-radius: 7px; }");
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (m_audioProcessor)
            m_audioProcessor->setMusicVolume(value / 100.0);
    });
    m_volumeSlider->hide();

    m_addBallBtn = new QPushButton("添加小球", this);
    m_addBallBtn->setGeometry(620, 100, 120, 30);
    m_addBallBtn->setStyleSheet(
        "QPushButton { background: #4a90e2; color: white; border: none; border-radius: 5px; }"
        "QPushButton:hover { background: #6aaef5; }");
    connect(m_addBallBtn, &QPushButton::clicked, this, &MainWindow::onAddBall);
    m_addBallBtn->hide();

    m_skinBtn = new QPushButton("选择皮肤", this);
    m_skinBtn->setGeometry(620, 140, 120, 30);
    m_skinBtn->setStyleSheet(
        "QPushButton { background: #50c878; color: white; border: none; border-radius: 5px; }"
        "QPushButton:hover { background: #70e898; }");
    connect(m_skinBtn, &QPushButton::clicked, this, &MainWindow::onSelectSkin);
    m_skinBtn->hide();
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

    // 增大半径至25，降低初始速度，无重力
    Ball *ball = new Ball(m_bounds.center().x(), m_bounds.top() + 80, 25,
                          QColor(255, 100, 80));
    ball->setBounds(m_bounds);
    ball->setVelocity(0.3, 0.3);   // 低速启动
    m_scene->addItem(ball);

    m_physicsTimer = new QTimer(this);
    connect(m_physicsTimer, &QTimer::timeout, this, &MainWindow::updatePhysics);
    m_physicsTimer->start(16);

    m_itemTimer = new QTimer(this);
    connect(m_itemTimer, &QTimer::timeout, this, &MainWindow::spawnItem);
    m_itemTimer->start(5000);

    if (!m_pendingFilePath.isEmpty())
        startAudioFile(m_pendingFilePath);
}

void MainWindow::updatePhysics()
{
    const int subSteps = 4;   // 4次子步骤保证平滑
    for (int step = 0; step < subSteps; ++step) {
        m_scene->advance();
        handleCollisions();
    }

    // 清理死亡小球
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

    checkItemPickups();
}

void MainWindow::handleCollisions()
{
    QList<QGraphicsItem*> items = m_scene->items();
    QList<Ball*> balls;
    for (auto *item : items) {
        if (auto *ball = dynamic_cast<Ball*>(item)) {
            balls.append(ball);
        }
    }

    // 多次迭代修正，彻底消除穿透
    for (int iteration = 0; iteration < 3; ++iteration) {
        bool anyCollision = false;
        for (int i = 0; i < balls.size(); ++i) {
            Ball *a = balls[i];
            for (int j = i + 1; j < balls.size(); ++j) {
                Ball *b = balls[j];
                qreal dx = a->x() - b->x();
                qreal dy = a->y() - b->y();
                qreal dist = qSqrt(dx*dx + dy*dy);
                qreal minDist = (a->boundingRect().width()/2 + b->boundingRect().width()/2);
                if (dist < minDist && dist > 0) {
                    anyCollision = true;
                    QPointF va = a->vel();
                    QPointF vb = b->vel();
                    QPointF relVel = va - vb;
                    qreal vn = (relVel.x()*dx + relVel.y()*dy) / dist;
                    if (vn > 0) {   // 相对接近才处理
                        // 伤害
                        int atkA = a->computeAttack();
                        int atkB = b->computeAttack();
                        a->takeDamage(atkB);
                        b->takeDamage(atkA);

                        // 弹性碰撞
                        QPointF n(dx/dist, dy/dist);
                        qreal ma = 1.0, mb = 1.0;
                        qreal v1n = va.x()*n.x() + va.y()*n.y();
                        qreal v2n = vb.x()*n.x() + vb.y()*n.y();
                        qreal v1n_after = (v1n*(ma-mb) + 2*mb*v2n) / (ma+mb);
                        qreal v2n_after = (v2n*(mb-ma) + 2*ma*v1n) / (ma+mb);

                        QPointF v1t = va - n*v1n;
                        QPointF v2t = vb - n*v2n;
                        a->setVelocity((v1t + n*v1n_after).x(), (v1t + n*v1n_after).y());
                        b->setVelocity((v2t + n*v2n_after).x(), (v2t + n*v2n_after).y());

                        // 完全推离
                        qreal overlap = minDist - dist;
                        a->setPos(a->pos() + n * (overlap/2));
                        b->setPos(b->pos() - n * (overlap/2));

                        // 播放碰撞音效
                        qreal relSpeed = qSqrt(relVel.x()*relVel.x() + relVel.y()*relVel.y());
                        m_collisionSound->playCollision(relSpeed);
                    }
                }
            }
        }
        if (!anyCollision) break;
    }
}

void MainWindow::checkItemPickups()
{
    QList<Ball*> balls;
    const auto sceneItems = m_scene->items();
    for (auto *item : sceneItems) {
        if (auto *ball = dynamic_cast<Ball*>(item))
            balls.append(ball);
    }

    for (int i = m_itemList.size() - 1; i >= 0; --i) {
        QGraphicsItem *item = m_itemList.at(i);
        QGraphicsEllipseItem *ellipse = dynamic_cast<QGraphicsEllipseItem*>(item);
        if (!ellipse) continue;

        for (Ball *ball : balls) {
            if (ball->isDead()) continue;
            qreal dx = ball->x() - ellipse->x();
            qreal dy = ball->y() - ellipse->y();
            qreal dist = qSqrt(dx*dx + dy*dy);
            if (dist < ball->boundingRect().width()/2 + ellipse->rect().width()/2) {
                QString type = ellipse->data(0).toString();
                if (type == "health") {
                    ball->heal(20);
                } else if (type == "speed") {
                    ball->applySpeedBuff(1.5f, 5000);
                }
                m_scene->removeItem(ellipse);
                m_itemList.removeAt(i);
                delete ellipse;
                break;
            }
        }
    }
}

void MainWindow::spawnItem()
{
    if (!m_mapReady) return;
    qreal radius = 12;
    qreal x = m_bounds.left() + radius + QRandomGenerator::global()->bounded(m_bounds.width() - 2*radius);
    qreal y = m_bounds.top() + radius + QRandomGenerator::global()->bounded(m_bounds.height() - 2*radius);

    bool isHealth = (QRandomGenerator::global()->bounded(2) == 0);
    QGraphicsEllipseItem *item = new QGraphicsEllipseItem(-radius, -radius, radius*2, radius*2);
    item->setPos(x, y);
    if (isHealth) {
        item->setBrush(Qt::green);
        item->setData(0, "health");
    } else {
        item->setBrush(QColor(255, 215, 0));
        item->setData(0, "speed");
    }
    item->setPen(Qt::NoPen);
    m_scene->addItem(item);
    m_itemList.append(item);
}

void MainWindow::onAddBall()
{
    if (!m_mapReady) return;
    qreal radius = 25;   // 统一半径
    qreal x = m_bounds.left() + radius + QRandomGenerator::global()->bounded(m_bounds.width() - 2*radius);
    qreal y = m_bounds.top() + 50;
    Ball *ball = new Ball(x, y, radius, QColor(QRandomGenerator::global()->bounded(256),
                                               QRandomGenerator::global()->bounded(256),
                                               QRandomGenerator::global()->bounded(256)));
    ball->setBounds(m_bounds);
    ball->setVelocity(QRandomGenerator::global()->bounded(1.0)-0.5, QRandomGenerator::global()->bounded(1.0)-0.5);
    if (!m_globalSkin.isNull())
        ball->setSkin(m_globalSkin);
    m_scene->addItem(ball);
}

void MainWindow::onSelectSkin()
{
    CircleCropDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QPixmap cropped = dlg.croppedPixmap();
        if (!cropped.isNull()) {
            m_globalSkin = cropped;
            applySkinToAll();
        }
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

    connect(m_audioProcessor, &AudioProcessor::beatDetected, this, [this](float intensity) {
        // 随机方向冲量，强度 0.3~1.0 映射为 2.0~5.0
        qreal angle = QRandomGenerator::global()->bounded(2 * M_PI);
        qreal strength = 2.0 + intensity * 3.0;
        qreal vx = cos(angle) * strength;
        qreal vy = sin(angle) * strength;

        QList<QGraphicsItem*> items = m_scene->items();
        for (auto *item : items) {
            if (auto *ball = dynamic_cast<Ball*>(item)) {
                ball->addImpulse(vx, vy);
            }
        }
        // 节拍不再触发音效，音效仅由碰撞产生
    });

    m_audioProcessor->loadAndPlay(filePath);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->ignore();
}