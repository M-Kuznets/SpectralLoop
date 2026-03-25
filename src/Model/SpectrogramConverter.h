#ifndef SPECTROGRAMCONVERTER_H
#define SPECTROGRAMCONVERTER_H
#pragma once

#include <QObject>
#include <QAudioDecoder>
#include <QImage>
#include <complex>
#include <vector>
#include "SpectrogramObject.h"

class SpectrogramConverter : public QObject
{
    Q_OBJECT

public:
    explicit SpectrogramConverter(QObject *parent = nullptr);
    ~SpectrogramConverter();

    void startImport(const QString &sourceURL, SpectrogramObject *target);

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

    void computeAndSave();

    static void fft(std::vector<std::complex<float>> &x);
    static std::vector<float> hannWindow(const std::vector<float> &seg);
    static QRgb  magToColor(float normalized);
};

#endif
