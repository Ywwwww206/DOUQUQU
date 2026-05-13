#ifndef BOSSSELECTIONPAGE_H
#define BOSSSELECTIONPAGE_H

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

class BossSelectionPage : public QWidget
{
    Q_OBJECT
public:
    explicit BossSelectionPage(QWidget *parent = nullptr) : QWidget(parent)
    {
        QGridLayout *grid = new QGridLayout(this);
        grid->setSpacing(20);

        QLabel *title = new QLabel("选择 Boss", this);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("font-size: 24px; font-weight: bold;");
        grid->addWidget(title, 0, 0, 1, 3);

        // 目前只有一个Boss：神龟
        QPushButton *btnTurtle = new QPushButton("神龟", this);
        btnTurtle->setFixedSize(150, 150);
        btnTurtle->setStyleSheet("font-size: 16px; background-color: green; color: white;");
        connect(btnTurtle, &QPushButton::clicked, this, [this]() {
            emit bossSelected(0);
        });
        grid->addWidget(btnTurtle, 1, 0);

        // 预留更多Boss位置（敬请期待）
        for (int i = 1; i < 3; ++i) {
            QPushButton *btnComing = new QPushButton("敬请期待", this);
            btnComing->setFixedSize(150, 150);
            btnComing->setEnabled(false);
            btnComing->setStyleSheet("background-color: #e0e0e0; color: #888; font-size: 16px;");
            grid->addWidget(btnComing, 1, i);
        }

        QPushButton *btnBack = new QPushButton("返回", this);
        connect(btnBack, &QPushButton::clicked, this, &BossSelectionPage::backClicked);
        grid->addWidget(btnBack, 2, 1);
    }

signals:
    void bossSelected(int id);
    void backClicked();
};

#endif // BOSSSELECTIONPAGE_H