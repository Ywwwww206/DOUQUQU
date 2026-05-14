#ifndef COLLISIONSOUND_H
#define COLLISIONSOUND_H

#include <QObject>
#include <QAudioSink>
#include <QBuffer>
#include <QByteArray>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QElapsedTimer>

class CollisionSound : public QObject
{
    Q_OBJECT
public:
    explicit CollisionSound(QObject *parent = nullptr);
    ~CollisionSound();

    void setCustomSound(const QByteArray &pcmData, QAudioFormat format);
    void play();
    void playTuned(float frequency, float volume);

    // 碰撞触发音效，relSpeed 为碰撞时的相对速度大小（用于动态调整音调和音量）
    void playCollision(float relSpeed);

private slots:
    void onAudioStateChanged(QAudio::State state);

private:
    void generateDefaultSound();
    void generateTunedSound(float frequency, float volume);
    void generateCollisionSound(float relSpeed);
    void playBuffer(QBuffer *buffer);
    void ensureDevice();

    QAudioSink *m_audioSink = nullptr;
    QBuffer *m_activeBuffer = nullptr;

    QByteArray m_defaultSoundData;
    bool m_useCustom = false;
    QByteArray m_customPcmData;
    QAudioFormat m_customFormat;
    int m_customPos = 0;
    int m_chunkSize = 0;
    QByteArray m_customChunk;

    QElapsedTimer m_lastPlayTimer;
    static const int m_collisionCooldownMs = 100;  // 碰撞音最短间隔
};

#endif // COLLISIONSOUND_H