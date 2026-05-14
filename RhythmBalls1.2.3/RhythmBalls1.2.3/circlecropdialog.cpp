#include "circlecropdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QPainter>
#include <QBitmap>

CircleCropDialog::CircleCropDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("选择皮肤 - 圆形裁剪");
    setFixedSize(280, 350);

    auto *mainLayout = new QVBoxLayout(this);

    // 预览区域
    m_previewLabel = new QLabel;
    m_previewLabel->setFixedSize(180, 180);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("background: #333; border: 2px solid #555; border-radius: 90px;");
    mainLayout->addWidget(m_previewLabel, 0, Qt::AlignCenter);

    // 按钮区
    auto *btnLayout = new QHBoxLayout;
    m_selectBtn = new QPushButton("选择图片");
    m_selectBtn->setStyleSheet(
        "QPushButton { background: #4a90e2; color: white; border: none; border-radius: 4px; padding: 6px; }"
        "QPushButton:hover { background: #6aaef5; }");
    btnLayout->addWidget(m_selectBtn);

    m_okBtn = new QPushButton("确定");
    m_okBtn->setEnabled(false);
    m_okBtn->setStyleSheet(
        "QPushButton { background: #50c878; color: white; border: none; border-radius: 4px; padding: 6px; }"
        "QPushButton:hover { background: #70e898; }"
        "QPushButton:disabled { background: #555; }");
    btnLayout->addWidget(m_okBtn);

    m_cancelBtn = new QPushButton("取消");
    m_cancelBtn->setStyleSheet(
        "QPushButton { background: #888; color: white; border: none; border-radius: 4px; padding: 6px; }"
        "QPushButton:hover { background: #aaa; }");
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_selectBtn, &QPushButton::clicked, this, &CircleCropDialog::onSelectImage);
    connect(m_okBtn, &QPushButton::clicked, this, &CircleCropDialog::onAccept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

QPixmap CircleCropDialog::croppedPixmap() const
{
    return m_croppedPixmap;
}

void CircleCropDialog::onSelectImage()
{
    QString path = QFileDialog::getOpenFileName(this, "选择小球皮肤",
                                                "", "图片 (*.png *.jpg *.jpeg *.bmp)");
    if (path.isEmpty()) return;

    QPixmap source(path);
    if (source.isNull()) return;

    m_sourcePixmap = source;
    m_sourcePath = path;
    updatePreview();
    m_okBtn->setEnabled(true);
}

void CircleCropDialog::onAccept()
{
    accept();
}

void CircleCropDialog::updatePreview()
{
    if (m_sourcePixmap.isNull()) return;

    // 裁剪为圆形并缩放到预览大小（180x180）
    QPixmap circular = cropToCircle(m_sourcePixmap);
    QPixmap scaled = circular.scaled(180, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_croppedPixmap = circular; // 保留原始裁剪后的图片（未缩放，后续再缩放）

    m_previewLabel->setPixmap(scaled);
}

QPixmap CircleCropDialog::cropToCircle(const QPixmap &source)
{
    // 取正方形区域
    int side = qMin(source.width(), source.height());
    int x = (source.width() - side) / 2;
    int y = (source.height() - side) / 2;
    QPixmap square = source.copy(x, y, side, side);

    // 创建透明底图并绘制圆形
    QPixmap result(side, side);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(square));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, side, side);
    painter.end();

    return result;
}