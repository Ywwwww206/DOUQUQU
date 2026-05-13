#ifndef MAPSELECTIONPAGE_H
#define MAPSELECTIONPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QRadioButton>

class MapSelectionPage : public QWidget
{
    Q_OBJECT
public:
    explicit MapSelectionPage(QWidget *parent = nullptr);

signals:
    void mapTypeSelected(int mapType);
    void backClicked();

private:
    QButtonGroup *m_group;
};

#endif // MAPSELECTIONPAGE_H