#include "audioprocessor.h"
#include <QMediaDevices>
#include <QUrl>
#include <numeric>
#include <QtMath>

AudioProcessor::AudioProcessor(QObject *parent)
    : QObject(parent) {}

AudioProcessor::~AudioProcessor()
{
    stop();
}

void AudioProcessor::loadAndPlay(const QString &filePath)
{
    stop();

    m_decoder = new QAudioDecoder(this);
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    m_decoder->setAudioFormat(format);
    m_decoder->setSource(QUrl::fromLocalFile(filePath));

    connect(m_decoder, &QAudioDecoder::bufferReady, this, &AudioProcessor::onBufferReady);
    connect(m_decoder, &QAudioDecoder::finished, this, &AudioProcessor::onDecoderFinished);
    connect(m_decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error),
            this, [](QAudioDecoder::Error error) {
                qWarning("解码错误: %d", error);
            });

    m_pcmData.clear();
    m_lpState = 0.0f;
    m_decoder->start();
}

void AudioProcessor::stop()
{
    if (m_analysisTimer) {
        m_analysisTimer->stop();
        delete m_analysisTimer;
        m_analysisTimer = nullptr;
    }
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
    }
    if (m_pcmBuffer) {
        m_pcmBuffer->close();
        delete m_pcmBuffer;
        m_pcmBuffer = nullptr;
    }
    if (m_decoder) {
        m_decoder->stop();
        delete m_decoder;
        m_decoder = nullptr;
    }
    m_lastCheckedPos = 0;
    m_energyHistory.clear();
    m_lowEnergyHistory.clear();
}

void AudioProcessor::setMusicVolume(qreal volume)
{
    m_volume = qBound(0.0, volume, 1.0);
    if (m_audioSink)
        m_audioSink->setVolume(m_volume);
}

void AudioProcessor::onBufferReady()
{
    auto buffer = m_decoder->read();
    m_pcmData.append(buffer.constData<char>(), buffer.byteCount());
}

void AudioProcessor::onDecoderFinished()
{
    if (m_pcmData.isEmpty()) {
        qWarning("解码后无数据");
        return;
    }

    m_pcmBuffer = new QBuffer(&m_pcmData, this);
    if (!m_pcmBuffer->open(QIODevice::ReadOnly)) {
        qWarning("无法打开播放缓冲区");
        return;
    }

    QAudioFormat fmt;
    fmt.setSampleRate(44100);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (!device.isFormatSupported(fmt)) {
        qWarning("音频格式不支持");
        return;
    }

    m_audioSink = new QAudioSink(device, fmt, this);
    m_audioSink->setVolume(m_volume);
    m_audioSink->start(m_pcmBuffer);

    m_lastCheckedPos = 0;
    m_analysisTimer = new QTimer(this);
    connect(m_analysisTimer, &QTimer::timeout, this, &AudioProcessor::processNewAudioData);
    m_analysisTimer->start(10);
}

void AudioProcessor::processNewAudioData()
{
    if (!m_pcmBuffer || !m_pcmBuffer->isOpen()) return;

    qint64 currentPos = m_pcmBuffer->pos();
    if (currentPos <= m_lastCheckedPos) return;

    QByteArray newData = m_pcmData.mid(m_lastCheckedPos, currentPos - m_lastCheckedPos);
    m_lastCheckedPos = currentPos;

    const qint16 *samples = reinterpret_cast<const qint16*>(newData.constData());
    int count = newData.size() / 2;
    if (count == 0) return;

    // 计算整体能量
    double sumEnergy = 0.0;
    // 计算低频能量（通过简单一阶低通：y[n] = y[n-1] + alpha*(x[n] - y[n-1])
    const float alpha = 0.02f;  // 截止频率约 100Hz @44.1kHz
    double sumLow = 0.0;
    float lp = m_lpState;

    for (int i = 0; i < count; ++i) {
        double norm = samples[i] / 32768.0;
        double energy = norm * norm;
        sumEnergy += energy;

        // 低通滤波
        lp += alpha * (norm - lp);
        double lowEnergy = lp * lp;
        sumLow += lowEnergy;
    }
    m_lpState = lp;

    float energy = sumEnergy / count;
    float lowEnergy = sumLow / count;

    const int historySize = 100;
    m_energyHistory.append(energy);
    m_lowEnergyHistory.append(lowEnergy);
    if (m_energyHistory.size() > historySize) {
        m_energyHistory.removeFirst();
        m_lowEnergyHistory.removeFirst();
    }

    float avgEnergy = std::accumulate(m_energyHistory.begin(), m_energyHistory.end(), 0.0f) / m_energyHistory.size();
    float avgLow = std::accumulate(m_lowEnergyHistory.begin(), m_lowEnergyHistory.end(), 0.0f) / m_lowEnergyHistory.size();

    // 加权检测：低频变化更敏感，但也要超过整体平均
    float combined = lowEnergy * 0.7f + energy * 0.3f;
    float avgCombined = avgLow * 0.7f + avgEnergy * 0.3f;

    float threshold = avgCombined * 1.4f;

    if (!m_beatFlag && combined > threshold && energy > avgEnergy * 1.1f) {
        m_beatFlag = true;
        // 计算强度（映射到 0.3~1.0）
        float intensity = qBound(0.3f, (combined - threshold) / (threshold + 0.0001f) + 0.3f, 1.0f);
        emit beatDetected(intensity);
    } else if (combined <= threshold) {
        m_beatFlag = false;
    }
}