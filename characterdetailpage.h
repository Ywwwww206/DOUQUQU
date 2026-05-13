#ifndef CHARACTERDETAILPAGE_H
#define CHARACTERDETAILPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

class CharacterDetailPage : public QWidget
{
    Q_OBJECT
public:
    explicit CharacterDetailPage(QWidget *parent = nullptr) : QWidget(parent), m_currentId(-1)
    {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignCenter);
        layout->setSpacing(20);

        m_nameLabel = new QLabel(this);
        m_nameLabel->setStyleSheet("font-size: 28px; font-weight: bold;");
        m_descLabel = new QLabel(this);
        m_descLabel->setStyleSheet("font-size: 16px;");
        m_descLabel->setAlignment(Qt::AlignCenter);

        QPushButton *btnSelect = new QPushButton("选择", this);
        btnSelect->setFixedSize(120, 40);
        QPushButton *btnBack = new QPushButton("返回", this);
        btnBack->setFixedSize(120, 40);

        layout->addWidget(m_nameLabel);
        layout->addWidget(m_descLabel);
        layout->addWidget(btnSelect, 0, Qt::AlignCenter);
        layout->addWidget(btnBack, 0, Qt::AlignCenter);

        connect(btnSelect, &QPushButton::clicked, this, [this]() {
            emit characterSelected(m_currentId);
        });
        connect(btnBack, &QPushButton::clicked, this, &CharacterDetailPage::backClicked);
    }

    void setCharacterId(int id)
    {
        m_currentId = id;
        if (id == 0) {
            m_nameLabel->setText("红色圆形");
            m_descLabel->setText("生命: 100\n攻击: 25\n速度: 20\n攻击间隔: 无\n攻击方式: 碰撞");
        } else if (id == 1) {
            m_nameLabel->setText("蓝色三角形");
            m_descLabel->setText("生命: 80\n攻击: 20\n速度: 5\n攻击间隔: 5秒\n攻击方式: 全屏索敌自动扣血");
        }
    }

signals:
    void characterSelected(int id);
    void backClicked();

private:
    int m_currentId;
    QLabel *m_nameLabel;
    QLabel *m_descLabel;
};

#endif // CHARACTERDETAILPAGE_H