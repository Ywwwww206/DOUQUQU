#include "mapselectionpage.h"

MapSelectionPage::MapSelectionPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(30);

    QLabel *title = new QLabel("选择地图类型", this);
    title->setStyleSheet("font-size: 28px; font-weight: bold;");
    mainLayout->addWidget(title);

    m_group = new QButtonGroup(this);
    QStringList names = {"经典擂台", "迷宫", "陷阱"};
    QStringList descs = {"只有边界墙壁", "随机墙体（不堵死）", "每10秒半数格子变红造成10伤害"};

    for (int i = 0; i < names.size(); ++i) {
        QRadioButton *radio = new QRadioButton(names[i] + " - " + descs[i], this);
        radio->setStyleSheet("font-size: 18px; margin: 10px;");
        m_group->addButton(radio, i);
        mainLayout->addWidget(radio);
        if (i == 0) radio->setChecked(true);
    }

    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *btnBack = new QPushButton("返回", this);
    QPushButton *btnNext = new QPushButton("下一步", this);
    btnBack->setFixedSize(120, 40);
    btnNext->setFixedSize(120, 40);
    btnLayout->addWidget(btnBack);
    btnLayout->addWidget(btnNext);
    mainLayout->addLayout(btnLayout);

    connect(btnBack, &QPushButton::clicked, this, &MapSelectionPage::backClicked);
    connect(btnNext, &QPushButton::clicked, this, [this]() {
        emit mapTypeSelected(m_group->checkedId());
    });
}