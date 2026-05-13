#ifndef CUSTOMCHARPAGE_H
#define CUSTOMCHARPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QPixmap>
#include <QPainter>
#include <QBitmap>

class CustomCharPage : public QWidget
{
    Q_OBJECT
public:
    explicit CustomCharPage(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignCenter);

        QLabel *title = new QLabel("自定义角色外观", this);
        title->setStyleSheet("font-size: 24px;");
        layout->addWidget(title);

        m_imageLabel = new QLabel(this);
        m_imageLabel->setFixedSize(200, 200);
        m_imageLabel->setStyleSheet("border: 2px dashed gray;");
        m_imageLabel->setAlignment(Qt::AlignCenter);
        m_imageLabel->setText("点击导入图片");
        layout->addWidget(m_imageLabel);

        QPushButton *btnImport = new QPushButton("导入图片", this);
        QPushButton *btnConfirm = new QPushButton("确认", this);
        QPushButton *btnBack = new QPushButton("返回", this);
        layout->addWidget(btnImport);
        layout->addWidget(btnConfirm);
        layout->addWidget(btnBack);

        connect(btnImport, &QPushButton::clicked, this, &CustomCharPage::importImage);
        connect(btnConfirm, &QPushButton::clicked, this, [this]() {
            emit characterCustomized(m_croppedPixmap);
        });
        connect(btnBack, &QPushButton::clicked, this, &CustomCharPage::backClicked);
    }

signals:
    void characterCustomized(const QPixmap &pixmap);
    void backClicked();

private slots:
    void importImage()
    {
        QString fileName = QFileDialog::getOpenFileName(this, "选择角色图片", "", "Images (*.png *.jpg *.bmp)");
        if (fileName.isEmpty()) return;
        QPixmap original(fileName);
        if (original.isNull()) return;
        // 缩放至200x200并裁剪为圆形
        QPixmap scaled = original.scaled(200, 200, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        QPixmap circle(200, 200);
        circle.fill(Qt::transparent);
        QPainter painter(&circle);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QBrush(scaled));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(0, 0, 200, 200);
        m_croppedPixmap = circle;
        m_imageLabel->setPixmap(circle.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

private:
    QLabel *m_imageLabel;
    QPixmap m_croppedPixmap;
};

#endif // CUSTOMCHARPAGE_H