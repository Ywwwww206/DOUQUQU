#include "audioprocessor.h"
#include <QMediaDevices>
#include <QUrl>
#include <numeric>

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
    format.setChannelCount(1);          // 单声道
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
    m_audioSink->setVolume(m_volume);   // 应用当前音量
    m_audioSink->start(m_pcmBuffer);

    // 节拍分析定时器
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

    double sum = 0.0;
    for (int i = 0; i < count; ++i) {
        double norm = qBound(-1.0, samples[i] / 32768.0, 1.0);
        sum += norm * norm;
    }
    float energy = sum / count;

    const int historySize = 100;
    m_energyHistory.append(energy);
    if (m_energyHistory.size() > historySize)
        m_energyHistory.removeFirst();

    float avg = std::accumulate(m_energyHistory.begin(), m_energyHistory.end(), 0.0f) / m_energyHistory.size();
    float threshold = avg * 1.5f;

    if (energy > threshold && !m_beatFlag) {
        m_beatFlag = true;
        emit beatDetected();
    } else if (energy <= threshold) {
        m_beatFlag = false;
    }
}