#ifndef SPECTROGRAMCONVERTER_H
#define SPECTROGRAMCONVERTER_H
#pragma once

#include <QObject>
#include <QAudioDecoder>
#include <QImage>
#include <complex>
#include <vector>
#include <utility>
#include "SpectrogramObject.h"

class SpectrogramConverter : public QObject
{
    Q_OBJECT

public:
    explicit SpectrogramConverter(QObject *parent = nullptr);
    ~SpectrogramConverter();

    void startImport(const QString &sourceURL, SpectrogramObject *target);

    // Paint
    void paintLine(float normX, float normY);
    void paintSpray(float normX, float normY, bool heavy);
    void erase(float normX, float normY);

    // ── 3. Magnitude tapering ───────────────────────────────────────────────
    // Call before regenerateImage() after any paint stroke.
    // Smooths the paint delta over time so there are no hard onset/offset clicks.
    void applyMagnitudeTaper();

    void regenerateImage();

    // ── 1. Griffin-Lim reconstruction ───────────────────────────────────────
    // Local: rebuilds only the dirty region and splices into cachedAudio (~fast)
    // Full:  rebuilds entire file — use for features that affect the whole file
    QString reconstructLocal(int glIterations = 8);
    QString reconstructFull (int glIterations = 20);

    // Features
    void applyHarmonicEnhancer();
    void applySpectralRepair();
    void applySpectralAttenuation(float selX1, float selY1, float selX2, float selY2);

signals:
    void spectrogramReady(const QString &imageFilePath);
    void conversionFailed(const QString &error);

private slots:
    void onBufferReady();
    void onFinished();
    void onError(QAudioDecoder::Error error);

private:
    QAudioDecoder     *m_decoder = nullptr;
    SpectrogramObject *m_target  = nullptr;
    QVector<float>     m_pending;

    void computeSTFT();
    void saveImage();

    std::pair<int,int> normToIndices(float normX, float normY) const;

    // ── 4. Pitch snapping ───────────────────────────────────────────────────
    int snapToPitch(int bin) const;

    // ── 2. Phase propagation ────────────────────────────────────────────────
    void propagatePhaseCoherence(int frame, int bin);

    // ── 5. Psychoacoustic masking ───────────────────────────────────────────
    void applyPsychoacousticMasking(int frame, int bin, float boostAmount);

    // DSP primitives
    static void fft(std::vector<std::complex<float>> &x);
    static void ifft(std::vector<std::complex<float>> &x);
    static std::vector<float> hannWindow(int N);
    static QRgb  magToColor(float normalised);
    static bool  writeWav(const QString &path,
                          const std::vector<float> &samples,
                          int sampleRate);

    // Internal GL helpers
    std::vector<float> istftParallel(const std::vector<std::vector<float>> &mag,
                                     const std::vector<std::vector<float>> &phase,
                                     int frameStart, int frameEnd) const;
    void               restftParallel(const std::vector<float> &signal,
                                      std::vector<std::vector<float>> &phaseOut,
                                      int frameStart, int frameEnd) const;
    QString            saveWavFromCache();
};

#endif
