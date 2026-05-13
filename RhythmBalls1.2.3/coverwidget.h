#ifndef COVERWIDGET_H
#define COVERWIDGET_H

#include <QWidget>
#include <QPushButton>

class CoverWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CoverWidget(QWidget *parent = nullptr);

signals:
    void startClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QPushButton *m_startBtn;
};

#endif // COVERWIDGET_H