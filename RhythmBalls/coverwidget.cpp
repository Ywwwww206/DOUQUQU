#include "coverwidget.h"
#include <QPainter>
#include <QResizeEvent>
#include <QtMath>

CoverWidget::CoverWidget(QWidget *parent)
    : QWidget(parent)
{
    // 深色背景
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(30, 30, 40));
    setPalette(pal);

    // 开始按钮
    m_startBtn = new QPushButton("按此开始", this);
    m_startBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #e94560;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 20px;"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  padding: 10px 30px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #ff6b7a;"
        "}"
        );
    m_startBtn->setCursor(Qt::PointingHandCursor);

    connect(m_startBtn, &QPushButton::clicked, this, &CoverWidget::startClicked);
}

void CoverWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    QPointF center(w / 2.0, h / 2.0);

    // 绘制多个同心圆：中心一组大同心圆，四周四组小同心圆
    auto drawConcentric = [&](QPointF pos, qreal maxR, int count, QColor baseColor) {
        for (int i = 0; i < count; ++i) {
            qreal r = maxR * (1.0 - i * 0.15);
            int alpha = 80 - i * 12;
            if (alpha < 20) alpha = 20;
            QColor c = baseColor;
            c.setAlpha(alpha);
            painter.setPen(QPen(c, 2));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(pos, r, r);
        }
    };

    // 中心同心圆
    drawConcentric(center, 180, 6, QColor(100, 200, 255));
    // 左上
    drawConcentric(QPointF(80, 80), 50, 4, QColor(255, 180, 100));
    // 右上
    drawConcentric(QPointF(w - 80, 80), 50, 4, QColor(255, 180, 100));
    // 左下
    drawConcentric(QPointF(80, h - 80), 50, 4, QColor(255, 180, 100));
    // 右下
    drawConcentric(QPointF(w - 80, h - 80), 50, 4, QColor(255, 180, 100));
    // 左中
    drawConcentric(QPointF(60, h/2.0), 40, 3, QColor(200, 150, 255));
    // 右中
    drawConcentric(QPointF(w - 60, h/2.0), 40, 3, QColor(200, 150, 255));

    // 标题文字
    painter.setPen(Qt::white);
    QFont titleFont("微软雅黑", 32, QFont::Bold);
    painter.setFont(titleFont);
    QString title = "节奏小球 - 纯净版 1.2.3";
    QRectF titleRect(0, center.y() - 120, w, 60);
    painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter, title);

    // 副标题（可选）
    QFont subFont("微软雅黑", 14);
    painter.setFont(subFont);
    painter.setPen(QColor(200, 200, 200));
    QRectF subRect(0, center.y() - 60, w, 30);
    painter.drawText(subRect, Qt::AlignHCenter | Qt::AlignVCenter, "Rhythm Balls - Pure Edition");
}

void CoverWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // 按钮位置：水平居中，垂直方向中偏下
    int btnW = 160;
    int btnH = 50;
    int x = (width() - btnW) / 2;
    int y = height() / 2 + 80;
    m_startBtn->setGeometry(x, y, btnW, btnH);
}