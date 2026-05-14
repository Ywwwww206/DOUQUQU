#include "collisionsound.h"
#include <QTimer>
#include <QRandomGenerator>
#include <QtMath>

CollisionSound::CollisionSound(QObject *parent)
    : QObject(parent)
{
    ensureDevice();
    m_lastPlayTimer.start();
}

CollisionSound::~CollisionSound()
{
    if (m_audioSink) {
        m_audioSink->stop();
    }
}

void CollisionSound::ensureDevice()
{
    if (m_audioSink) return;

    QAudioFormat fmt;
    fmt.setSampleRate(44100);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    m_audioSink = new QAudioSink(device, fmt, this);
    connect(m_audioSink, &QAudioSink::stateChanged,
            this, &CollisionSound::onAudioStateChanged);
}

void CollisionSound::setCustomSound(const QByteArray &pcmData, QAudioFormat format)
{
    m_audioSink->stop();

    m_customPcmData = pcmData;
    m_customFormat = format;
    m_useCustom = true;
    m_customPos = 0;

    int bytesPerSample = (format.sampleFormat() == QAudioFormat::Int16) ? 2 : 1;
    m_chunkSize = static_cast<int>(format.sampleRate() * 0.5) * format.channelCount() * bytesPerSample;
    if (m_customPcmData.size() < m_chunkSize)
        m_chunkSize = m_customPcmData.size();

    m_customChunk.clear();
}

void CollisionSound::play()
{
    if (m_audioSink->state() == QAudio::ActiveState)
        return;

    if (m_useCustom) {
        if (m_customPcmData.isEmpty()) return;
        int remaining = m_customPcmData.size() - m_customPos;
        if (remaining < m_chunkSize) {
            QByteArray firstPart = m_customPcmData.mid(m_customPos, remaining);
            int need = m_chunkSize - remaining;
            QByteArray secondPart = m_customPcmData.left(need);
            m_customChunk = firstPart + secondPart;
            m_customPos = need;
        } else {
            m_customChunk = m_customPcmData.mid(m_customPos, m_chunkSize);
            m_customPos += m_chunkSize;
        }

        QBuffer *buf = new QBuffer(&m_customChunk, this);
        buf->open(QIODevice::ReadOnly);
        playBuffer(buf);
    } else {
        generateDefaultSound();
        QBuffer *buf = new QBuffer(&m_defaultSoundData, this);
        buf->open(QIODevice::ReadOnly);
        playBuffer(buf);
    }
}

void CollisionSound::playTuned(float frequency, float volume)
{
    if (m_audioSink->state() == QAudio::ActiveState)
        return;

    if (m_useCustom) {
        m_audioSink->setVolume(qBound(0.0, volume, 1.0));
        play();
        return;
    }

    generateTunedSound(frequency, volume);

    QBuffer *buf = new QBuffer(&m_defaultSoundData, this);
    buf->open(QIODevice::ReadOnly);
    playBuffer(buf);
}

void CollisionSound::playCollision(float relSpeed)
{
    // 冷却检查
    if (m_lastPlayTimer.elapsed() < m_collisionCooldownMs)
        return;
    m_lastPlayTimer.restart();

    // 如果正在播放自定义音效，则忽略合成
    if (m_useCustom) {
        m_audioSink->setVolume(0.8);
        play();
        return;
    }

    generateCollisionSound(relSpeed);

    QBuffer *buf = new QBuffer(&m_defaultSoundData, this);
    buf->open(QIODevice::ReadOnly);
    playBuffer(buf);
}

void CollisionSound::generateDefaultSound()
{
    const int durationMs = 55;
    const int sampleRate = 44100;
    const int sampleCount = sampleRate * durationMs / 1000;
    m_defaultSoundData.resize(sampleCount * sizeof(qint16));
    qint16 *ptr = reinterpret_cast<qint16*>(m_defaultSoundData.data());
    for (int i = 0; i < sampleCount; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double envelope = exp(-t * 45.0);
        double tone = sin(2 * M_PI * 1800 * t) * 0.6
                      + sin(2 * M_PI * 2300 * t) * 0.4;
        double noise = ((QRandomGenerator::global()->bounded(2000)-1000)/1000.0)*0.06;
        double sample = (tone + noise) * envelope * 0.8;
        ptr[i] = static_cast<qint16>(qBound(-1.0, sample, 1.0) * 32767);
    }
}

void CollisionSound::generateTunedSound(float frequency, float volume)
{
    const int durationMs = 80;
    const int sampleRate = 44100;
    const int sampleCount = sampleRate * durationMs / 1000;
    m_defaultSoundData.resize(sampleCount * sizeof(qint16));
    qint16 *ptr = reinterpret_cast<qint16*>(m_defaultSoundData.data());

    float freq = qBound(400.0f, frequency, 1600.0f);
    float vol = qBound(0.5f, volume, 1.0f);

    for (int i = 0; i < sampleCount; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double envelope = exp(-t * 28.0);
        double tone = sin(2 * M_PI * freq * t) * 0.6
                      + sin(2 * M_PI * freq * 2.0 * t) * 0.25
                      + sin(2 * M_PI * freq * 3.0 * t) * 0.1;
        double attackNoise = (i < sampleRate * 0.008)
                                 ? ((QRandomGenerator::global()->bounded(2000) - 1000) / 1000.0) * 0.3 * (1.0 - t * 125.0)
                                 : 0.0;
        double sample = (tone + attackNoise) * envelope * vol * 1.2;
        ptr[i] = static_cast<qint16>(qBound(-1.0, sample, 1.0) * 32767);
    }
}

void CollisionSound::generateCollisionSound(float relSpeed)
{
    const int durationMs = 300;          // 0.3秒
    const int sampleRate = 44100;
    const int sampleCount = sampleRate * durationMs / 1000;
    m_defaultSoundData.resize(sampleCount * sizeof(qint16));
    qint16 *ptr = reinterpret_cast<qint16*>(m_defaultSoundData.data());

    // 频率随相对速度变化：0~15 -> 300~1200 Hz
    float freq = qBound(300.0f, 300.0f + relSpeed * 60.0f, 1200.0f);
    float vol = qBound(0.3f, 0.5f + relSpeed * 0.03f, 1.0f);

    for (int i = 0; i < sampleCount; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double envelope = exp(-t * 12.0);         // 缓慢衰减，持续0.3s
        double tone = sin(2 * M_PI * freq * t) * 0.6
                      + sin(2 * M_PI * freq * 2.0 * t) * 0.2;
        double noise = (i < sampleRate * 0.01)
                           ? ((QRandomGenerator::global()->bounded(2000) - 1000) / 1000.0) * 0.2
                           : 0.0;
        double sample = (tone + noise) * envelope * vol * 1.1;
        ptr[i] = static_cast<qint16>(qBound(-1.0, sample, 1.0) * 32767);
    }
}

void CollisionSound::playBuffer(QBuffer *buffer)
{
    m_audioSink->start(buffer);
    m_activeBuffer = buffer;
}

void CollisionSound::onAudioStateChanged(QAudio::State state)
{
    if (state == QAudio::IdleState) {
        if (m_activeBuffer) {
            m_activeBuffer->close();
            m_activeBuffer->deleteLater();
            m_activeBuffer = nullptr;
        }
        m_audioSink->stop();
    }
}