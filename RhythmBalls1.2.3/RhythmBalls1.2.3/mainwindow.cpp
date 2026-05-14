#include "mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include <QRandomGenerator>
#include <QGraphicsScene>
#include <QList>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QFont>
#include <QPen>
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
    setFocusPolicy(Qt::StrongFocus);

    setupUI();

    m_mapRect = new QGraphicsRectItem();
    m_mapRect->setPen(QPen(Qt::black, 3));
    m_mapRect->setBrush(Qt::white);
    m_mapRect->setPos(10, 75);
    m_mapRect->setRect(0, 0, 0, 0);
    m_scene->addItem(m_mapRect);

    m_targetMapRect = QRectF(0, 0, 580, 450);
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
    m_editMapBtn->show();
    m_mapAnimTimer->start(30);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    m_scene->setSceneRect(0, 0, m_view->width(), m_view->height());
}

void MainWindow::setupUI()
{
    // 选择音乐
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

    // 撞击音
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

    // 音量滑块
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

    // 添加小球
    m_addBallBtn = new QPushButton("添加小球", this);
    m_addBallBtn->setGeometry(620, 100, 120, 30);
    m_addBallBtn->setStyleSheet(
        "QPushButton { background: #4a90e2; color: white; border: none; border-radius: 5px; }"
        "QPushButton:hover { background: #6aaef5; }");
    connect(m_addBallBtn, &QPushButton::clicked, this, &MainWindow::onAddBall);
    m_addBallBtn->hide();

    // 选择皮肤
    m_skinBtn = new QPushButton("选择皮肤", this);
    m_skinBtn->setGeometry(620, 140, 120, 30);
    m_skinBtn->setStyleSheet(
        "QPushButton { background: #50c878; color: white; border: none; border-radius: 5px; }"
        "QPushButton:hover { background: #70e898; }");
    connect(m_skinBtn, &QPushButton::clicked, this, &MainWindow::onSelectSkin);
    m_skinBtn->hide();

    // 编辑地图
    m_editMapBtn = new QPushButton("编辑地图", this);
    m_editMapBtn->setGeometry(620, 220, 120, 30);
    m_editMapBtn->setStyleSheet(
        "QPushButton { background: #ff9800; color: white; border: none; border-radius: 5px; }"
        "QPushButton:hover { background: #ffb74d; }");
    connect(m_editMapBtn, &QPushButton::clicked, this, &MainWindow::toggleEditMode);
    m_editMapBtn->hide();
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

    Ball *ball = new Ball(m_bounds.center().x(), m_bounds.top() + 80, 25,
                          QColor(255, 100, 80));
    ball->setBounds(m_bounds);
    ball->setVelocity(0.3, 0.3);
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

void MainWindow::toggleEditMode()
{
    m_editMode = !m_editMode;
    if (m_editMode) {
        if (m_physicsTimer) m_physicsTimer->stop();
        if (m_itemTimer) m_itemTimer->stop();
        if (m_audioProcessor) m_audioProcessor->stop();   // 停止音乐

        if (!m_editHint) {
            m_editHint = new QGraphicsTextItem("编辑模式：空格放置墙体 | 右键旋转90° | 方向键移动预览 | 再次点击按钮退出");
            m_editHint->setDefaultTextColor(Qt::yellow);
            m_editHint->setFont(QFont("Arial", 14, QFont::Bold));
            m_editHint->setZValue(100);
            m_scene->addItem(m_editHint);
        }
        m_editHint->setPos(m_bounds.center().x() - 350, m_bounds.top() - 40);
        m_editHint->show();

        m_wallPosition = m_bounds.center();
        m_wallAngle = 0;
        if (!m_previewWall) {
            m_previewWall = new QGraphicsLineItem();
            m_previewWall->setPen(QPen(Qt::green, 4));
            m_previewWall->setZValue(99);
            m_scene->addItem(m_previewWall);
        }
        m_previewWall->show();
        QPointF dir = (m_wallAngle == 0) ? QPointF(m_wallLength/2, 0) : QPointF(0, m_wallLength/2);
        m_previewWall->setLine(QLineF(m_wallPosition - dir, m_wallPosition + dir));
        setFocus();
    } else {
        if (m_editHint) m_editHint->hide();
        if (m_previewWall) m_previewWall->hide();

        if (m_mapReady) {
            if (m_physicsTimer) m_physicsTimer->start();
            if (m_itemTimer) m_itemTimer->start();
            // 音乐不自动恢复，用户需重新选择
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (!m_editMode) {
        QMainWindow::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Space:
        if (m_previewWall) {
            QLineF line = m_previewWall->line();
            QGraphicsLineItem *wall = new QGraphicsLineItem(line);
            wall->setPen(QPen(Qt::darkGray, 4));
            m_scene->addItem(wall);
            m_walls.append(wall);
        }
        break;
    case Qt::Key_Right:
        m_wallAngle = (m_wallAngle + 90) % 180;
        break;
    case Qt::Key_Left:
        m_wallAngle = (m_wallAngle - 90 + 180) % 180;
        break;
    case Qt::Key_Up:
    case Qt::Key_W:
        m_wallPosition.ry() -= 10;
        break;
    case Qt::Key_Down:
    case Qt::Key_S:
        m_wallPosition.ry() += 10;
        break;
    case Qt::Key_A:
        m_wallPosition.rx() -= 10;
        break;
    case Qt::Key_D:
        m_wallPosition.rx() += 10;
        break;
    default:
        break;
    }

    m_wallPosition.setX(qBound(m_bounds.left(), m_wallPosition.x(), m_bounds.right()));
    m_wallPosition.setY(qBound(m_bounds.top(), m_wallPosition.y(), m_bounds.bottom()));

    if (m_previewWall) {
        QPointF dir = (m_wallAngle == 0) ? QPointF(m_wallLength/2, 0) : QPointF(0, m_wallLength/2);
        m_previewWall->setLine(QLineF(m_wallPosition - dir, m_wallPosition + dir));
    }
}

void MainWindow::updatePhysics()
{
    if (m_editMode) return;

    const int subSteps = 4;
    for (int step = 0; step < subSteps; ++step) {
        m_scene->advance();
        handleCollisions();
        handleWallCollisions();
    }

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
    clearDamageTexts();
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
                    if (vn > 0) {
                        int atkA = a->computeAttack();
                        int atkB = b->computeAttack();
                        a->takeDamage(atkB);
                        b->takeDamage(atkA);

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

                        qreal overlap = minDist - dist;
                        a->setPos(a->pos() + n * (overlap/2));
                        b->setPos(b->pos() - n * (overlap/2));

                        qreal relSpeed = qSqrt(relVel.x()*relVel.x() + relVel.y()*relVel.y());
                        m_collisionSound->playCollision(relSpeed);

                        createDamageText(a->pos(), atkB);
                        createDamageText(b->pos(), atkA);
                    }
                }
            }
        }
        if (!anyCollision) break;
    }
}

void MainWindow::handleWallCollisions()
{
    QList<QGraphicsItem*> items = m_scene->items();
    for (auto *item : items) {
        Ball *ball = dynamic_cast<Ball*>(item);
        if (!ball || ball->isDead()) continue;

        QRectF bbox = ball->boundingRect().translated(ball->pos());
        qreal r = bbox.width()/2;
        QPointF center = ball->pos();

        for (const auto &wall : m_walls) {
            QLineF line = wall->line();
            QPointF p1 = line.p1();
            QPointF p2 = line.p2();
            QPointF d = p2 - p1;
            qreal len2 = d.x()*d.x() + d.y()*d.y();
            if (len2 == 0) continue;

            qreal t = QPointF::dotProduct(center - p1, d) / len2;
            t = qBound(0.0, t, 1.0);
            QPointF closest = p1 + d * t;
            QPointF diff = center - closest;
            qreal dist = qSqrt(diff.x()*diff.x() + diff.y()*diff.y());

            if (dist < r) {
                QPointF normal = diff / dist;
                ball->setPos(closest + normal * r);

                QPointF vel = ball->vel();
                qreal dot = QPointF::dotProduct(vel, normal);
                ball->setVelocity(vel.x() - 2*dot*normal.x(), vel.y() - 2*dot*normal.y());
            }
        }
    }
}

void MainWindow::createDamageText(const QPointF &pos, int damage)
{
    QGraphicsTextItem *text = new QGraphicsTextItem(QString("-%1").arg(damage));
    text->setDefaultTextColor(Qt::red);
    QFont font("Arial", 20, QFont::Bold);
    text->setFont(font);
    text->setPos(pos.x() - 20, pos.y() - 40);
    text->setZValue(50);
    m_scene->addItem(text);

    DamageText dt;
    dt.text = text;
    dt.timer.start();
    dt.worldPos = pos;
    m_damageTexts.append(dt);
}

void MainWindow::clearDamageTexts()
{
    for (int i = m_damageTexts.size() - 1; i >= 0; --i) {
        if (m_damageTexts[i].timer.elapsed() > 1000) {
            m_scene->removeItem(m_damageTexts[i].text);
            delete m_damageTexts[i].text;
            m_damageTexts.removeAt(i);
        }
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
                    ball->heal(0);
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
    if (!m_mapReady || m_editMode) return;
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
    if (!m_mapReady || m_editMode) return;
    qreal radius = 25;
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
    });

    m_audioProcessor->loadAndPlay(filePath);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->ignore();
}