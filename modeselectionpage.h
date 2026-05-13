#ifndef MODESELECTIONPAGE_H
#define MODESELECTIONPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>

class ModeSelectionPage : public QWidget
{
    Q_OBJECT
public:
    explicit ModeSelectionPage(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignCenter);
        layout->setSpacing(30);

        QLabel *title = new QLabel("选择游戏模式", this);
        title->setStyleSheet("font-size: 28px; font-weight: bold;");
        layout->addWidget(title);

        QPushButton *btnSim = new QPushButton("模拟对战", this);
        btnSim->setFixedSize(200, 60);
        QPushButton *btnBoss = new QPushButton("Boss挑战", this);
        btnBoss->setFixedSize(200, 60);
        QPushButton *btnBack = new QPushButton("返回", this);
        btnBack->setFixedSize(120, 40);

        layout->addWidget(btnSim);
        layout->addWidget(btnBoss);
        layout->addWidget(btnBack);

        connect(btnSim, &QPushButton::clicked, this, &ModeSelectionPage::simulationMode);
        connect(btnBoss, &QPushButton::clicked, this, &ModeSelectionPage::bossMode);
        connect(btnBack, &QPushButton::clicked, this, &ModeSelectionPage::backClicked);
    }

signals:
    void simulationMode();
    void bossMode();
    void backClicked();
};

#endif // MODESELECTIONPAGE_H