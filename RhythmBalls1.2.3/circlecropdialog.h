#ifndef CIRCLECROPDIALOG_H
#define CIRCLECROPDIALOG_H

#include <QDialog>
#include <QPixmap>

class QLabel;
class QPushButton;

class CircleCropDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CircleCropDialog(QWidget *parent = nullptr);

    QPixmap croppedPixmap() const;

private slots:
    void onSelectImage();
    void onAccept();

private:
    void updatePreview();
    static QPixmap cropToCircle(const QPixmap &source);

    QLabel *m_previewLabel;
    QPushButton *m_selectBtn;
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;

    QString m_sourcePath;
    QPixmap m_sourcePixmap;
    QPixmap m_croppedPixmap;
};

#endif // CIRCLECROPDIALOG_H