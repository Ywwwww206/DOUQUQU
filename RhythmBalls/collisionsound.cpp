#include "collisionsound.h"
#include <QTimer>
#include <QRandomGenerator>
#include <QtMath>

CollisionSound::CollisionSound(QObject *parent)
    : QObject(parent)
{
    // 先生成默认音效
    generateDefaultSound();

    // 初始化音频输出设备（格式用默认44100单声道16bit）
    QAudioFormat defaultFmt;
    defaultFmt.setSampleRate(44100);
    defaultFmt.setChannelCount(1);
    defaultFmt.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    m_audioSink = new QAudioSink(device, defaultFmt, this);
    connect(m_audioSink, &QAudioSink::stateChanged,
            this, &CollisionSound::onAudioStateChanged);
}

CollisionSound::~CollisionSound()
{
    m_audioSink->stop();
}

void CollisionSound::generateDefaultSound()
{
    // 清脆敲击声（50ms）
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

void CollisionSound::setCustomSound(const QByteArray &pcmData, QAudioFormat format)
{
    // 停止当前播放
    m_audioSink->stop();

    m_customPcmData = pcmData;
    m_customFormat = format;
    m_useCustom = true;
    m_customPos = 0;

    // 计算0.5秒对应的字节数
    int bytesPerSample = (format.sampleFormat() == QAudioFormat::Int16) ? 2 : 1;
    m_chunkSize = static_cast<int>(format.sampleRate() * 0.5) * format.channelCount() * bytesPerSample;

    // 如果自定义数据太短，调整chunkSize避免越界
    if (m_customPcmData.size() < m_chunkSize)
        m_chunkSize = m_customPcmData.size();

    // 初始化临时buffer（后面在play时填充）
    m_customChunk.clear();
}

void CollisionSound::play()
{
    // 如果正在播放任何声音，直接忽略（避免重叠）
    if (m_audioSink->state() == QAudio::ActiveState)
        return;

    if (m_useCustom) {
        processCustomPlay();
    } else {
        // 使用默认合成音效：每次从buffer开头播放
        QBuffer *buf = new QBuffer(&m_defaultSoundData, this);
        buf->open(QIODevice::ReadOnly);
        m_audioSink->start(buf);
        // 注意：buffer会在stateChanged中自动delete
        m_activeBuffer = buf;
    }
}

void CollisionSound::processCustomPlay()
{
    if (m_customPcmData.isEmpty()) return;

    // 如果剩余数据不足一个chunk，循环回开头补齐（简单处理：直接回绕）
    int remaining = m_customPcmData.size() - m_customPos;
    if (remaining < m_chunkSize) {
        // 先取剩余部分，再从开头取剩余量
        QByteArray firstPart = m_customPcmData.mid(m_customPos, remaining);
        int need = m_chunkSize - remaining;
        QByteArray secondPart = m_customPcmData.left(need);
        m_customChunk = firstPart + secondPart;
        m_customPos = need;   // 下次从need位置开始
    } else {
        m_customChunk = m_customPcmData.mid(m_customPos, m_chunkSize);
        m_customPos += m_chunkSize;
    }

    // 使用一个独立的buffer来播放这个片段
    QBuffer *buf = new QBuffer(&m_customChunk, this);
    buf->open(QIODevice::ReadOnly);
    m_audioSink->start(buf);
    m_activeBuffer = buf;
}

void CollisionSound::onAudioStateChanged(QAudio::State state)
{
    if (state == QAudio::IdleState) {
        // 播放完毕，清理buffer
        if (m_activeBuffer) {
            m_activeBuffer->close();
            m_activeBuffer->deleteLater();
            m_activeBuffer = nullptr;
        }
        m_audioSink->stop();  // 确保回到Stopped状态
    }
}