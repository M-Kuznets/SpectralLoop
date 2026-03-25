#ifndef SPECTROGRAMOBJECT_H
#define SPECTROGRAMOBJECT_H
#pragma once

#include <QVector>
#include <QString>

class SpectrogramObject
{
public:
    SpectrogramObject();
    ~SpectrogramObject();

    QVector<float> samples;   // Mono float PCM
    int sampleRate   = 44100;
    int channelCount = 1;
    QString sourceURL;
    bool isLoaded = false;
};

#endif