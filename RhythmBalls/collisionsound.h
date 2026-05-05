#ifndef COLLISIONSOUND_H
#define COLLISIONSOUND_H

#include <QObject>
#include <QAudioSink>
#include <QBuffer>
#include <QByteArray>
#include <QAudioFormat>
#include <QMediaDevices>

class CollisionSound : public QObject
{
    Q_OBJECT
public:
    explicit CollisionSound(QObject *parent = nullptr);
    ~CollisionSound();

    // 设置自定义循环音效（PCM数据+格式），会自动替代合成音效
    void setCustomSound(const QByteArray &pcmData, QAudioFormat format);

    // 播放一次碰撞声音（若已在播放则忽略）
    void play();

private slots:
    void onAudioStateChanged(QAudio::State state);

private:
    void generateDefaultSound();   // 生成默认合成音效
    void processCustomPlay();      // 从自定义音效中取0.5秒并播放
    void ensureCustomBuffer();     // 确保临时buffer可用

    QAudioSink *m_audioSink = nullptr;
    QBuffer *m_activeBuffer = nullptr;   // 当前正在播放的buffer
    QByteArray m_defaultSoundData;       // 默认合成音效数据

    // 自定义音效相关
    bool m_useCustom = false;
    QByteArray m_customPcmData;          // 完整自定义音频数据
    QAudioFormat m_customFormat;
    int m_customPos = 0;                 // 自定义音频的下次播放起始位置（字节）
    int m_chunkSize = 0;                 // 0.5秒对应的字节数

    QByteArray m_customChunk;            // 临时存放截取片段的buffer
};

#endif // COLLISIONSOUND_H