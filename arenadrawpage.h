#ifndef ARENADRAWPAGE_H
#define ARENADRAWPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QPainter>

class ArenaDrawPage : public QWidget
{
    Q_OBJECT
public:
    explicit ArenaDrawPage(QWidget *parent = nullptr);

    void setMapType(int type) { m_mapType = type; }
    QRect getDrawnRect() const { return m_drawnRect; }

signals:
    void arenaConfirmed(int mapType, QRect arenaRect);
    void backClicked();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    int m_mapType;
    QRect m_drawnRect;
    QPoint m_startPoint;
    QPoint m_currentPoint;
    bool m_drawing;
    QLabel *m_hintLabel;
    QPushButton *m_confirmBtn;
    QPushButton *m_backBtn;
};

#endif // ARENADRAWPAGE_H