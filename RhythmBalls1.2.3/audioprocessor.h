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
#include <QVector>

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
    void beatDetected(float intensity);      // 强度 0~1

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

    // 能量历史（整体和低频）
    QVector<float> m_energyHistory;
    QVector<float> m_lowEnergyHistory;
    bool m_beatFlag = false;
    qreal m_volume = 0.3;

    // 用于低通滤波的状态变量
    float m_lpState = 0.0f;
};

#endif // AUDIOPROCESSOR_H