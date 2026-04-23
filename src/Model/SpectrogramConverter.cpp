#include "SpectrogramConverter.h"

#include <QAudioFormat>
#include <QStandardPaths>
#include <QUrl>
#include <QImage>
#include <QFile>
#include <QDataStream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>
#include <future>
#include <thread>

static constexpr int   FFT_SIZE    = 2048;
static constexpr int   HOP_SIZE    = 512;
static constexpr int   NUM_BINS    = FFT_SIZE / 2;
static constexpr float DB_FLOOR    = -80.0f;
static constexpr float TAPER_SIGMA = 5.0f;

static int s_imageCounter = 0;
static int s_audioCounter = 0;

SpectrogramConverter::SpectrogramConverter(QObject *parent) : QObject(parent) {}
SpectrogramConverter::~SpectrogramConverter() {}

// ─── Import ───────────────────────────────────────────────────────────────────
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
        connect(m_decoder, &QAudioDecoder::bufferReady,
                this, &SpectrogramConverter::onBufferReady);
        connect(m_decoder, &QAudioDecoder::finished,
                this, &SpectrogramConverter::onFinished);
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
    computeSTFT();
    saveImage();
}

void SpectrogramConverter::onError(QAudioDecoder::Error)
{
    emit conversionFailed(m_decoder->errorString());
}

// ─── FFT / IFFT ───────────────────────────────────────────────────────────────
void SpectrogramConverter::fft(std::vector<std::complex<float>> &x)
{
    const size_t N = x.size();
    if (N <= 1) return;
    for (size_t i = 1, j = 0; i < N; ++i) {
        size_t bit = N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(x[i], x[j]);
    }
    for (size_t len = 2; len <= N; len <<= 1) {
        float ang = -2.0f * float(M_PI) / float(len);
        std::complex<float> wlen(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < N; i += len) {
            std::complex<float> w(1, 0);
            for (size_t j = 0; j < len / 2; ++j) {
                auto u = x[i+j], v = x[i+j+len/2] * w;
                x[i+j] = u+v; x[i+j+len/2] = u-v;
                w *= wlen;
            }
        }
    }
}

void SpectrogramConverter::ifft(std::vector<std::complex<float>> &x)
{
    for (auto &v : x) v = std::conj(v);
    fft(x);
    const float N = float(x.size());
    for (auto &v : x) v = std::conj(v) / N;
}

std::vector<float> SpectrogramConverter::hannWindow(int N)
{
    std::vector<float> w(N);
    for (int i = 0; i < N; ++i)
        w[i] = 0.5f * (1.0f - std::cos(2.0f * float(M_PI) * i / float(N-1)));
    return w;
}

QRgb SpectrogramConverter::magToColor(float n)
{
    if (n <= 0.02f) return qRgb(10,14,20);
    float r,g,b;
    if      (n < 0.40f) { float t=n/0.40f;          r=0;              g=t*0.85f;        b=0; }
    else if (n < 0.70f) { float t=(n-0.40f)/0.30f;  r=t*0.95f;        g=0.85f+t*0.15f;  b=0; }
    else                { float t=(n-0.70f)/0.30f;  r=0.95f+t*0.05f;  g=1.0f-t*0.45f;   b=0; }
    return qRgb(int(r*255), int(g*255), int(b*255));
}

// ─── STFT ─────────────────────────────────────────────────────────────────────
void SpectrogramConverter::computeSTFT()
{
    const int N = m_pending.size();
    int numFrames = (N - FFT_SIZE) / HOP_SIZE + 1;
    if (numFrames <= 0) { emit conversionFailed("Audio file too short."); return; }

    m_target->numFrames = numFrames;
    m_target->numBins   = NUM_BINS;
    m_target->stftMag.assign(numFrames,   std::vector<float>(NUM_BINS, 0.0f));
    m_target->stftPhase.assign(numFrames, std::vector<float>(NUM_BINS, 0.0f));

    auto win = hannWindow(FFT_SIZE);
    float globalMax = 0.0f;

    for (int f = 0; f < numFrames; ++f) {
        int start = f * HOP_SIZE;
        std::vector<std::complex<float>> cx(FFT_SIZE, {0,0});
        for (int i = 0; i < FFT_SIZE && start+i < N; ++i)
            cx[i] = m_pending[start+i] * win[i];
        fft(cx);
        for (int b = 0; b < NUM_BINS; ++b) {
            float mag = std::abs(cx[b]);
            m_target->stftMag[f][b]   = mag;
            m_target->stftPhase[f][b] = std::arg(cx[b]);
            if (mag > globalMax) globalMax = mag;
        }
    }
    m_target->stftGlobalMax   = std::max(globalMax, 1e-10f);
    m_target->stftMagOriginal = m_target->stftMag;
}

// ─── Parallel ISTFT ───────────────────────────────────────────────────────────
// IFFTs are independent per frame — split across hardware threads,
// then do overlap-add sequentially.
std::vector<float> SpectrogramConverter::istftParallel(
    const std::vector<std::vector<float>> &mag,
    const std::vector<std::vector<float>> &phase,
    int frameStart, int frameEnd) const
{
    int numLocalFrames = frameEnd - frameStart + 1;
    int outputLen = (numLocalFrames - 1) * HOP_SIZE + FFT_SIZE;
    auto synWin = hannWindow(FFT_SIZE);

    std::vector<std::vector<float>> frameBufs(numLocalFrames,
                                              std::vector<float>(FFT_SIZE, 0.0f));

    int numThreads = std::max(1u, std::thread::hardware_concurrency());
    int chunk = std::max(1, numLocalFrames / numThreads);

    auto processChunk = [&](int start, int end) {
        for (int fi = start; fi < end && fi < numLocalFrames; ++fi) {
            int f = frameStart + fi;
            std::vector<std::complex<float>> cx(FFT_SIZE, {0,0});
            cx[0] = std::polar(mag[f][0], phase[f][0]);
            for (int b = 1; b < NUM_BINS; ++b) {
                cx[b]            = std::polar(mag[f][b], phase[f][b]);
                cx[FFT_SIZE - b] = std::conj(cx[b]);
            }
            ifft(cx);
            for (int i = 0; i < FFT_SIZE; ++i)
                frameBufs[fi][i] = cx[i].real() * synWin[i];
        }
    };

    std::vector<std::future<void>> futures;
    for (int t = 0; t < numThreads; ++t)
        futures.push_back(std::async(std::launch::async,
                                     processChunk, t * chunk, (t+1) * chunk));
    for (auto &f : futures) f.get();

    // Overlap-add
    std::vector<float> out(outputLen, 0.0f);
    std::vector<float> norm(outputLen, 0.0f);
    for (int fi = 0; fi < numLocalFrames; ++fi) {
        int start = fi * HOP_SIZE;
        for (int i = 0; i < FFT_SIZE && start+i < outputLen; ++i) {
            out[start+i]  += frameBufs[fi][i];
            norm[start+i] += synWin[i] * synWin[i];
        }
    }
    for (int i = 0; i < outputLen; ++i)
        if (norm[i] > 1e-8f) out[i] /= norm[i];
    return out;
}

// ─── Parallel re-STFT ─────────────────────────────────────────────────────────
void SpectrogramConverter::restftParallel(const std::vector<float> &signal,
                                          std::vector<std::vector<float>> &phaseOut,
                                          int frameStart, int frameEnd) const
{
    int numLocalFrames = frameEnd - frameStart + 1;
    int sigLen = int(signal.size());
    auto anaWin = hannWindow(FFT_SIZE);

    int numThreads = std::max(1u, std::thread::hardware_concurrency());
    int chunk = std::max(1, numLocalFrames / numThreads);

    auto processChunk = [&](int start, int end) {
        for (int fi = start; fi < end && fi < numLocalFrames; ++fi) {
            int f = frameStart + fi;
            int localStart = fi * HOP_SIZE;
            std::vector<std::complex<float>> cx(FFT_SIZE, {0,0});
            for (int i = 0; i < FFT_SIZE; ++i) {
                int idx = localStart + i;
                cx[i] = (idx < sigLen) ? signal[idx] * anaWin[i] : 0.0f;
            }
            fft(cx);
            for (int b = 0; b < NUM_BINS; ++b)
                phaseOut[f][b] = std::arg(cx[b]);
        }
    };

    std::vector<std::future<void>> futures;
    for (int t = 0; t < numThreads; ++t)
        futures.push_back(std::async(std::launch::async,
                                     processChunk, t * chunk, (t+1) * chunk));
    for (auto &f : futures) f.get();
}

// ─── Save WAV from cachedAudio ────────────────────────────────────────────────
QString SpectrogramConverter::saveWavFromCache()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                   + QString("/spectralloop_audio_%1.wav").arg(s_audioCounter++);
    return writeWav(path, m_target->cachedAudio, m_target->sampleRate) ? path : QString{};
}

// ─── Local rebuild — paint strokes (fast) ────────────────────────────────────
// 0 GL iterations: single parallel ISTFT pass using propagated phases.
// Splices result into cachedAudio with cosine crossfade at edges.
QString SpectrogramConverter::reconstructLocal(int /*glIterations*/)
{
    if (!m_target || !m_target->isLoaded || !m_target->hasDirty()) return {};
    if (m_target->cachedAudio.empty()) return reconstructFull(3);

    const int XFADE = 80;
    int f0 = std::max(0,                       m_target->dirtyFrameMin - XFADE);
    int f1 = std::min(m_target->numFrames - 1, m_target->dirtyFrameMax + XFADE);
    m_target->clearDirty();

    auto localSignal = istftParallel(m_target->stftMag, m_target->stftPhase, f0, f1);

    int sampleStart  = f0 * HOP_SIZE;
    int xfadeSamples = XFADE * HOP_SIZE;
    int localLen     = int(localSignal.size());

    for (int i = sampleStart;
         i < sampleStart + localLen && i < int(m_target->cachedAudio.size()); ++i)
    {
        int local = i - sampleStart;
        float alpha = 1.0f;
        if (local < xfadeSamples)
            alpha = 0.5f * (1.0f - std::cos(float(M_PI) * local / float(xfadeSamples)));
        else if (local >= localLen - xfadeSamples)
            alpha = 0.5f * (1.0f - std::cos(float(M_PI) * (localLen-local) / float(xfadeSamples)));
        m_target->cachedAudio[i] = alpha * localSignal[local]
                                  + (1.0f - alpha) * m_target->cachedAudio[i];
    }

    return saveWavFromCache();
}

// ─── Full rebuild — features & first load ─────────────────────────────────────
QString SpectrogramConverter::reconstructFull(int glIterations)
{
    if (!m_target || !m_target->isLoaded) return {};
    m_target->clearDirty();

    auto workPhase = m_target->stftPhase;
    std::vector<float> signal;

    for (int iter = 0; iter <= glIterations; ++iter) {
        signal = istftParallel(m_target->stftMag, workPhase, 0, m_target->numFrames-1);
        if (iter == glIterations) break;
        restftParallel(signal, workPhase, 0, m_target->numFrames-1);
    }

    m_target->cachedAudio = signal;
    return saveWavFromCache();
}

// ─── WAV writer ───────────────────────────────────────────────────────────────
bool SpectrogramConverter::writeWav(const QString &path,
                                    const std::vector<float> &samples,
                                    int sampleRate)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    quint32 num      = quint32(samples.size());
    quint32 dataSize = num * 2;
    quint32 fileSize = 36 + dataSize;
    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::LittleEndian);
    file.write("RIFF",4); ds << fileSize;
    file.write("WAVE",4);
    file.write("fmt ",4); ds << quint32(16) << quint16(1) << quint16(1)
                             << quint32(sampleRate) << quint32(sampleRate*2)
                             << quint16(2) << quint16(16);
    file.write("data",4); ds << dataSize;
    for (float s : samples)
        ds << qint16(qRound(std::clamp(s,-1.0f,1.0f) * 32767.0f));
    return true;
}

// ─── Image ────────────────────────────────────────────────────────────────────
void SpectrogramConverter::saveImage()
{
    if (!m_target || m_target->stftMag.empty()) {
        emit conversionFailed("No STFT data."); return;
    }
    int nF = m_target->numFrames, nB = m_target->numBins;
    float gmax = m_target->stftGlobalMax;
    const float eps = 1e-10f;
    QImage img(nF, nB, QImage::Format_RGB32);
    img.fill(qRgb(10,14,20));
    for (int f = 0; f < nF; ++f)
        for (int b = 0; b < nB; ++b) {
            float dB   = 20.0f * std::log10(m_target->stftMag[f][b] / (gmax+eps) + eps);
            float norm = std::clamp((dB - DB_FLOOR) / (-DB_FLOOR), 0.0f, 1.0f);
            img.setPixel(f, nB-1-b, magToColor(norm));
        }
    QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                   + QString("/spectralloop_img_%1.png").arg(s_imageCounter++);
    if (!img.save(path))
        emit conversionFailed("Could not save spectrogram image.");
    else
        emit spectrogramReady("file://" + path);
}

void SpectrogramConverter::regenerateImage()
{
    float gmax = 0.0f;
    for (auto &row : m_target->stftMag)
        for (float v : row) if (v > gmax) gmax = v;
    m_target->stftGlobalMax = std::max(gmax, 1e-10f);
    saveImage();
}

// ─── Coordinate helper ────────────────────────────────────────────────────────
std::pair<int,int> SpectrogramConverter::normToIndices(float normX, float normY) const
{
    int frame = std::clamp(int(normX * m_target->numFrames), 0, m_target->numFrames-1);
    int bin   = std::clamp(int((1.0f-normY) * m_target->numBins), 0, m_target->numBins-1);
    return {frame, bin};
}

// ─── Pitch snapping ───────────────────────────────────────────────────────────
int SpectrogramConverter::snapToPitch(int bin) const
{
    if (bin <= 0) return bin;
    float freq = float(bin) * float(m_target->sampleRate) / float(FFT_SIZE);
    if (freq < 20.0f || freq > 20000.0f) return bin;
    float semitones   = 12.0f * std::log2(freq / 440.0f);
    float snappedFreq = 440.0f * std::pow(2.0f, std::round(semitones) / 12.0f);
    int   snappedBin  = int(snappedFreq * FFT_SIZE / m_target->sampleRate);
    return std::clamp(snappedBin, 0, m_target->numBins-1);
}

// ─── Phase propagation ────────────────────────────────────────────────────────
void SpectrogramConverter::propagatePhaseCoherence(int frame, int bin)
{
    if (!m_target || bin < 0 || bin >= m_target->numBins) return;
    float advance = 2.0f * float(M_PI) * float(bin) * HOP_SIZE / float(FFT_SIZE);
    if (frame > 0)
        m_target->stftPhase[frame][bin] = m_target->stftPhase[frame-1][bin] + advance;
    const int PROPAGATE = 60;
    for (int f = frame+1; f < std::min(frame+PROPAGATE, m_target->numFrames); ++f)
        m_target->stftPhase[f][bin] = m_target->stftPhase[f-1][bin] + advance;
}

// ─── Psychoacoustic masking ───────────────────────────────────────────────────
void SpectrogramConverter::applyPsychoacousticMasking(int frame, int bin, float boostAmount)
{
    if (!m_target || bin <= 0 || frame < 0) return;
    float centerFreq = float(bin) * m_target->sampleRate / float(FFT_SIZE);
    if (centerFreq < 20.0f) return;
    float maskStrength = boostAmount / (m_target->stftGlobalMax + 1e-10f);
    if (maskStrength < 0.04f) return;
    for (int b = 0; b < m_target->numBins; ++b) {
        if (b == bin) continue;
        float freq = float(b) * m_target->sampleRate / float(FFT_SIZE);
        if (freq <= 0.0f) continue;
        float octaves = std::log2(freq / centerFreq);
        float maskDB  = (octaves > 0) ? octaves * 18.0f : -octaves * 28.0f;
        if (maskDB > 24.0f) continue;
        float suppression = boostAmount * std::pow(10.0f, -maskDB / 20.0f) * 0.08f;
        m_target->stftMag[frame][b] = std::max(0.0f,
                                                m_target->stftMag[frame][b] - suppression);
    }
}

// ─── Magnitude tapering ───────────────────────────────────────────────────────
void SpectrogramConverter::applyMagnitudeTaper()
{
    if (!m_target || m_target->stftMag.empty()) return;
    int nF = m_target->numFrames, nB = m_target->numBins;
    const int RADIUS = int(TAPER_SIGMA * 3.0f);
    std::vector<float> gauss(2*RADIUS+1);
    float wsum = 0.0f;
    for (int k = -RADIUS; k <= RADIUS; ++k) {
        gauss[k+RADIUS] = std::exp(-float(k*k) / (2.0f * TAPER_SIGMA * TAPER_SIGMA));
        wsum += gauss[k+RADIUS];
    }
    for (auto &w : gauss) w /= wsum;

    for (int b = 0; b < nB; ++b) {
        std::vector<float> delta(nF);
        bool anyEdit = false;
        for (int f = 0; f < nF; ++f) {
            delta[f] = m_target->stftMag[f][b] - m_target->stftMagOriginal[f][b];
            if (std::abs(delta[f]) > 1e-6f) anyEdit = true;
        }
        if (!anyEdit) continue;
        std::vector<float> smoothed(nF, 0.0f);
        for (int f = 0; f < nF; ++f)
            for (int k = -RADIUS; k <= RADIUS; ++k) {
                int ff = f+k;
                if (ff >= 0 && ff < nF) smoothed[f] += delta[ff] * gauss[k+RADIUS];
            }
        for (int f = 0; f < nF; ++f)
            m_target->stftMag[f][b] = std::max(0.0f,
                m_target->stftMagOriginal[f][b] + smoothed[f]);
    }
}

// ─── Paint ────────────────────────────────────────────────────────────────────
void SpectrogramConverter::paintLine(float normX, float normY)
{
    if (!m_target || !m_target->isLoaded) return;
    auto [frame, rawBin] = normToIndices(normX, normY);
    int bin = snapToPitch(rawBin);
    float addAmount = m_target->stftGlobalMax * 0.25f;
    m_target->stftMag[frame][bin] = std::min(m_target->stftGlobalMax,
                                             m_target->stftMag[frame][bin] + addAmount);
    if (bin > 0)
        m_target->stftMag[frame][bin-1] = std::min(m_target->stftGlobalMax,
            m_target->stftMag[frame][bin-1] + addAmount * 0.4f);
    if (bin < m_target->numBins-1)
        m_target->stftMag[frame][bin+1] = std::min(m_target->stftGlobalMax,
            m_target->stftMag[frame][bin+1] + addAmount * 0.4f);
    propagatePhaseCoherence(frame, bin);
    applyPsychoacousticMasking(frame, bin, addAmount);
    m_target->markDirty(frame);
}

void SpectrogramConverter::paintSpray(float normX, float normY, bool heavy)
{
    if (!m_target || !m_target->isLoaded) return;
    auto [cf, cb] = normToIndices(normX, normY);
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    int   radius     = heavy ? 6  : 14;
    float density    = heavy ? 0.55f : 0.15f;
    float dropAmount = m_target->stftGlobalMax * (heavy ? 0.20f : 0.09f);
    for (int df = -radius; df <= radius; ++df)
        for (int db = -radius; db <= radius; ++db) {
            if (df*df + db*db > radius*radius) continue;
            int f = cf+df, b = cb+db;
            if (f < 0 || f >= m_target->numFrames) continue;
            if (b < 0 || b >= m_target->numBins)   continue;
            if (chance(rng) > density) continue;
            m_target->stftMag[f][b] = std::min(m_target->stftGlobalMax,
                                                m_target->stftMag[f][b] + dropAmount);
            propagatePhaseCoherence(f, b);
            m_target->markDirty(f);
        }
}

void SpectrogramConverter::erase(float normX, float normY)
{
    if (!m_target || !m_target->isLoaded) return;
    auto [cf, cb] = normToIndices(normX, normY);
    const int radius = 5;
    for (int df = -radius; df <= radius; ++df)
        for (int db = -radius; db <= radius; ++db) {
            int f = cf+df, b = cb+db;
            if (f < 0 || f >= m_target->numFrames) continue;
            if (b < 0 || b >= m_target->numBins)   continue;
            if (df*df + db*db <= radius*radius) {
                m_target->stftMag[f][b] = 0.0f;
                m_target->markDirty(f);
            }
        }
}

// ─── Features ─────────────────────────────────────────────────────────────────
void SpectrogramConverter::applyHarmonicEnhancer()
{
    if (!m_target || !m_target->isLoaded) return;
    auto snapshot = m_target->stftMag;
    float threshold = m_target->stftGlobalMax * 0.10f;
    for (int f = 0; f < m_target->numFrames; ++f)
        for (int b = 1; b < m_target->numBins; ++b) {
            if (snapshot[f][b] < threshold) continue;
            for (int k = 2; k <= 4; ++k) {
                int hb = b * k;
                if (hb >= m_target->numBins) break;
                m_target->stftMag[f][hb] = std::min(m_target->stftGlobalMax,
                    m_target->stftMag[f][hb] + snapshot[f][b] * (0.30f / float(k-1)));
                propagatePhaseCoherence(f, hb);
            }
        }
}

void SpectrogramConverter::applySpectralRepair()
{
    if (!m_target || !m_target->isLoaded) return;
    for (int b = 0; b < m_target->numBins; ++b) {
        std::vector<float> col(m_target->numFrames);
        for (int f = 0; f < m_target->numFrames; ++f) col[f] = m_target->stftMag[f][b];
        std::sort(col.begin(), col.end());
        float noise = col[std::max(0, m_target->numFrames/20)];
        for (int f = 0; f < m_target->numFrames; ++f)
            m_target->stftMag[f][b] = std::max(0.0f, m_target->stftMag[f][b] - noise*1.5f);
    }
}

void SpectrogramConverter::applySpectralAttenuation(float selX1, float selY1,
                                                     float selX2, float selY2)
{
    if (!m_target || !m_target->isLoaded) return;
    float x1=std::min(selX1,selX2), x2=std::max(selX1,selX2);
    float y1=std::min(selY1,selY2), y2=std::max(selY1,selY2);
    auto [f1,binHi] = normToIndices(x1,y1);
    auto [f2,binLo] = normToIndices(x2,y2);
    int bMin=std::min(binLo,binHi), bMax=std::max(binLo,binHi);
    for (int f=f1; f<=f2 && f<m_target->numFrames; ++f)
        for (int b=bMin; b<=bMax && b<m_target->numBins; ++b)
            m_target->stftMag[f][b] *= 0.05f;
}