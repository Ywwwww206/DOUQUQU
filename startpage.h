#ifndef STARTPAGE_H
#define STARTPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

class StartPage : public QWidget
{
    Q_OBJECT
public:
    explicit StartPage(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignCenter);
        layout->setSpacing(20);

        QLabel *title = new QLabel("简易擂台", this);
        title->setStyleSheet("font-size: 32px; font-weight: bold;");
        layout->addWidget(title);

        QPushButton *btnSim = new QPushButton("模拟对战", this);
        btnSim->setFixedSize(200, 60);
        layout->addWidget(btnSim, 0, Qt::AlignCenter);

        QPushButton *btnBoss = new QPushButton("Boss挑战", this);
        btnBoss->setFixedSize(200, 60);
        layout->addWidget(btnBoss, 0, Qt::AlignCenter);

        connect(btnSim, &QPushButton::clicked, this, &StartPage::simulationClicked);
        connect(btnBoss, &QPushButton::clicked, this, &StartPage::bossClicked);
    }

signals:
    void simulationClicked();
    void bossClicked();
};

#endif // STARTPAGE_H