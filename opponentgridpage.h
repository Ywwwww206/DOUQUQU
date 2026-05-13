#ifndef OPPONENTGRIDPAGE_H
#define OPPONENTGRIDPAGE_H

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

class OpponentGridPage : public QWidget
{
    Q_OBJECT
public:
    explicit OpponentGridPage(QWidget *parent = nullptr) : QWidget(parent)
    {
        QGridLayout *grid = new QGridLayout(this);
        grid->setSpacing(20);

        QLabel *title = new QLabel("选择对手", this);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("font-size: 24px; font-weight: bold;");
        grid->addWidget(title, 0, 0, 1, 3);

        QStringList names = {"红色圆形", "蓝色三角形"};
        for (int i = 0; i < names.size(); ++i) {
            QPushButton *btn = new QPushButton(names[i], this);
            btn->setFixedSize(150, 150);
            btn->setStyleSheet("font-size: 16px;");
            connect(btn, &QPushButton::clicked, this, [this, i]() {
                emit opponentClicked(i);
            });
            grid->addWidget(btn, 1, i);
        }

        QPushButton *btnComing = new QPushButton("敬请期待", this);
        btnComing->setFixedSize(150, 150);
        btnComing->setEnabled(false);
        btnComing->setStyleSheet("background-color: #e0e0e0; color: #888; font-size: 16px;");
        grid->addWidget(btnComing, 1, 2);

        QPushButton *btnBack = new QPushButton("返回", this);
        connect(btnBack, &QPushButton::clicked, this, &OpponentGridPage::backClicked);
        grid->addWidget(btnBack, 2, 1, 1, 1);
    }

signals:
    void opponentClicked(int id);
    void backClicked();
};

#endif // OPPONENTGRIDPAGE_H