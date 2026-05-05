#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <QObject>
#include <QAudioDecoder>
#include <QAudioFormat>
#include <QAudioSink>
#include <QBuffer>
#include <QTimer>
#include <QByteArray>
#include <QUrl>

class AudioProcessor : public QObject
{
    Q_OBJECT
public:
    explicit AudioProcessor(QObject *parent = nullptr);
    ~AudioProcessor();

    void loadAndPlay(const QString &filePath);
    void stop();
    void setMusicVolume(qreal volume);      // 0.0 ~ 1.0

signals:
    void beatDetected();

private:
    void onBufferReady();
    void onDecoderFinished();
    void processNewAudioData();

    QAudioDecoder *m_decoder = nullptr;
    QByteArray m_pcmData;
    QBuffer *m_pcmBuffer = nullptr;
    QAudioSink *m_audioSink = nullptr;
    QTimer *m_analysisTimer = nullptr;
    qint64 m_lastCheckedPos = 0;

    QVector<float> m_energyHistory;
    bool m_beatFlag = false;
    qreal m_volume = 0.3;
};

#endif // AUDIOPROCESSOR_H