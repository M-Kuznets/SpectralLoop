#ifndef SPECTROGRAMOBJECT_H
#define SPECTROGRAMOBJECT_H
#pragma once

#include <QVector>
#include <QString>
#include <vector>
#include <climits>

class SpectrogramObject
{
public:
    SpectrogramObject();
    ~SpectrogramObject();

    QVector<float> samples;
    int sampleRate   = 44100;
    int channelCount = 1;
    QString sourceURL;
    bool isLoaded = false;

    std::vector<std::vector<float>> stftMag;
    std::vector<std::vector<float>> stftMagOriginal;
    std::vector<std::vector<float>> stftPhase;

    float stftGlobalMax = 1.0f;
    int numFrames = 0;
    int numBins   = 0;

    // Full reconstructed PCM — rebuilt once on load, then only the dirty
    // slice is replaced on each paint stroke (much faster than full rebuild)
    std::vector<float> cachedAudio;

    // Dirty region set by paint ops, consumed by commitPaint()
    int dirtyFrameMin = INT_MAX;
    int dirtyFrameMax = INT_MIN;

    void markDirty(int frame) {
        if (frame < dirtyFrameMin) dirtyFrameMin = frame;
        if (frame > dirtyFrameMax) dirtyFrameMax = frame;
    }
    void clearDirty() { dirtyFrameMin = INT_MAX; dirtyFrameMax = INT_MIN; }
    bool hasDirty()   { return dirtyFrameMin <= dirtyFrameMax; }
};

#endif
