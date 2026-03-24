#ifndef SPECTROGRAMOBJECT_H
#define SPECTROGRAMOBJECT_H

#pragma once

#include<QtMultimedia/QAudioBuffer>

class SpectrogramObject
{
public:

    QAudioBuffer audioStream;

    SpectrogramObject();
    ~SpectrogramObject();

private:

};

#endif