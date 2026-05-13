#include "arenadrawpage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

ArenaDrawPage::ArenaDrawPage(QWidget *parent)
    : QWidget(parent), m_mapType(0), m_drawing(false)
{
    setMinimumSize(700, 700);
    setMouseTracking(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_hintLabel = new QLabel("按住鼠标左键并拖动，绘制擂台边界", this);
    m_hintLabel->setAlignment(Qt::AlignCenter);
    m_hintLabel->setStyleSheet("background-color: rgba(0,0,0,150); color: white; font-size: 18px; padding: 10px;");
    m_hintLabel->setGeometry(0, 0, width(), 40);

    QWidget *btnWidget = new QWidget(this);
    QHBoxLayout *btnLayout = new QHBoxLayout(btnWidget);
    m_backBtn = new QPushButton("返回", btnWidget);
    m_confirmBtn = new QPushButton("确认并开始", btnWidget);
    m_confirmBtn->setEnabled(false);
    btnLayout->addWidget(m_backBtn);
    btnLayout->addWidget(m_confirmBtn);
    btnWidget->setFixedHeight(50);
    mainLayout->addWidget(btnWidget, 0, Qt::AlignBottom);

    connect(m_backBtn, &QPushButton::clicked, this, &ArenaDrawPage::backClicked);
    connect(m_confirmBtn, &QPushButton::clicked, this, [this]() {
        if (m_drawnRect.isValid() && m_drawnRect.width() > 20 && m_drawnRect.height() > 20) {
            emit arenaConfirmed(m_mapType, m_drawnRect);
        }
    });

    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);
}

void ArenaDrawPage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_drawnRect.isValid()) {
        painter.setPen(QPen(Qt::green, 3));
        painter.setBrush(QColor(0, 255, 0, 30));
        painter.drawRect(m_drawnRect);
    }

    if (m_drawing) {
        QRect tempRect(m_startPoint, m_currentPoint);
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter.setBrush(QColor(255, 0, 0, 20));
        painter.drawRect(tempRect.normalized());
    }

    if (!m_drawnRect.isValid() && !m_drawing) {
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 16));
        painter.drawText(rect(), Qt::AlignCenter, "在此区域按住鼠标左键拖动\n绘制擂台边界");
    }
}

void ArenaDrawPage::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_startPoint = e->pos();
        m_currentPoint = e->pos();
        m_drawing = true;
        m_confirmBtn->setEnabled(false);
        update();
    }
}

void ArenaDrawPage::mouseMoveEvent(QMouseEvent *e)
{
    if (m_drawing) {
        m_currentPoint = e->pos();
        update();
    }
}

void ArenaDrawPage::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && m_drawing) {
        m_drawing = false;
        QRect newRect(m_startPoint, e->pos());
        newRect = newRect.normalized();

        if (newRect.width() > 30 && newRect.height() > 30) {
            m_drawnRect = newRect;
            m_confirmBtn->setEnabled(true);
            m_hintLabel->setText("擂台已绘制，可点击确认开始");
        } else {
            m_hintLabel->setText("矩形太小，请重新绘制");
        }
        update();
    }
}