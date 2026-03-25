#include "SpectrogramConverter.h"

#include <QAudioFormat>
#include <QStandardPaths>
#include <QImage>
#include <QUrl>
#include <cmath>
#include <algorithm>

static constexpr int  FFT_SIZE     = 2048;
static constexpr int  HOP_SIZE     = 512;
static constexpr int  DISPLAY_BINS = 768;   // frequency rows in the image
static constexpr float DB_FLOOR    = -80.0f;

SpectrogramConverter::SpectrogramConverter(QObject *parent) : QObject(parent) {}
SpectrogramConverter::~SpectrogramConverter() {}

void SpectrogramConverter::startImport(const QString &sourceURL, SpectrogramObject *target)
{
    m_target = target;
    m_pending.clear();

    if (!m_decoder) {
        m_decoder = new QAudioDecoder(this);

        QAudioFormat fmt;
        fmt.setSampleRate(44100);
        fmt.setChannelCount(1);
        fmt.setSampleFormat(QAudioFormat::Float);
        m_decoder->setAudioFormat(fmt);

        connect(m_decoder, &QAudioDecoder::bufferReady, this, &SpectrogramConverter::onBufferReady);
        connect(m_decoder, &QAudioDecoder::finished,    this, &SpectrogramConverter::onFinished);
        connect(m_decoder, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error),
                this, &SpectrogramConverter::onError);
    } else {
        m_decoder->stop();
    }

    m_decoder->setSource(QUrl(sourceURL));
    m_decoder->start();
}

void SpectrogramConverter::onBufferReady()
{
    QAudioBuffer buf = m_decoder->read();
    const float *data = buf.constData<float>();
    int frames = buf.frameCount();
    m_pending.reserve(m_pending.size() + frames);
    for (int i = 0; i < frames; ++i)
        m_pending.append(data[i]);
}

void SpectrogramConverter::onFinished()
{
    m_target->samples    = m_pending;
    m_target->sampleRate = 44100;
    m_target->isLoaded   = true;
    computeAndSave();
}

void SpectrogramConverter::onError(QAudioDecoder::Error)
{
    emit conversionFailed(m_decoder->errorString());
}

void SpectrogramConverter::fft(std::vector<std::complex<float>> &x)
{
    const size_t N = x.size();
    if (N <= 1) return;

    // bit reversal
    for (size_t i = 1, j = 0; i < N; ++i) {
        size_t bit = N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(x[i], x[j]);
    }

    for (size_t len = 2; len <= N; len <<= 1) {
        float ang = -2.0f * static_cast<float>(M_PI) / static_cast<float>(len);
        std::complex<float> wlen(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < N; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (size_t j = 0; j < len / 2; ++j) {
                auto u = x[i + j];
                auto v = x[i + j + len / 2] * w;
                x[i + j]           = u + v;
                x[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

std::vector<float> SpectrogramConverter::hannWindow(const std::vector<float> &seg)
{
    size_t N = seg.size();
    std::vector<float> out(N);
    for (size_t i = 0; i < N; ++i) {
        float w = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * i / (N - 1)));
        out[i] = seg[i] * w;
    }
    return out;
}

// Map normalised amplitude [0,1] → green → yellow → orange 
QRgb SpectrogramConverter::magToColor(float n)
{
    if (n <= 0.02f) return qRgb(10, 14, 20);

    float r, g, b;
    if (n < 0.40f) {
        float t = n / 0.40f;
        r = 0.0f;          g = t * 0.85f; b = 0.0f;
    } else if (n < 0.70f) {
        float t = (n - 0.40f) / 0.30f;
        r = t * 0.95f;     g = 0.85f + t * 0.15f; b = 0.0f;
    } else {
        float t = (n - 0.70f) / 0.30f;
        r = 0.95f + t * 0.05f; g = 1.0f - t * 0.45f; b = 0.0f;
    }
    return qRgb(int(r * 255), int(g * 255), int(b * 255));
}

void SpectrogramConverter::computeAndSave()
{
    const auto &samples = m_pending;
    const int   N       = static_cast<int>(samples.size());

    if (N < FFT_SIZE) {
        emit conversionFailed("Audio file too short.");
        return;
    }

    int numFrames = (N - FFT_SIZE) / HOP_SIZE + 1;

    std::vector<std::vector<float>> mag(numFrames, std::vector<float>(DISPLAY_BINS, 0.0f));
    float globalMax = 0.0f;

    for (int f = 0; f < numFrames; ++f) {
        int start = f * HOP_SIZE;

        std::vector<float> seg(FFT_SIZE, 0.0f);
        for (int i = 0; i < FFT_SIZE && start + i < N; ++i)
            seg[i] = samples[start + i];

        auto windowed = hannWindow(seg);

        std::vector<std::complex<float>> cx(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) cx[i] = windowed[i];
        fft(cx);

        for (int b = 0; b < DISPLAY_BINS; ++b) {
            float m = std::abs(cx[b]);
            mag[f][b] = m;
            if (m > globalMax) globalMax = m;
        }
    }

    QImage img(numFrames, DISPLAY_BINS, QImage::Format_RGB32);
    img.fill(qRgb(10, 14, 20));

    const float eps = 1e-10f;
    for (int f = 0; f < numFrames; ++f) {
        for (int b = 0; b < DISPLAY_BINS; ++b) {
            float dB  = 20.0f * std::log10(mag[f][b] / (globalMax + eps) + eps);
            float norm = std::clamp((dB - DB_FLOOR) / (-DB_FLOOR), 0.0f, 1.0f);
            img.setPixel(f, DISPLAY_BINS - 1 - b, magToColor(norm));
        }
    }

    QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                   + "/spectralloop_spectrogram.png";
    if (!img.save(path)) {
        emit conversionFailed("Could not save spectrogram image to " + path);
        return;
    }

    emit spectrogramReady("file://" + path);
}