#ifndef SPECTROGRAMCONVERTER_H
#define SPECTROGRAMCONVERTER_H

#pragma once

#include<QObject>
#include<QtMultimedia/QAudioBuffer>
#include"SpectrogramObject.h"

class SpectrogramConverter
{
public:
    SpectrogramConverter();
    ~SpectrogramConverter();

    /*
        Called when user selects a file to import
        Returns sucessful or not
    */
    bool UserAudioImport(QString sourceURL, SpectrogramObject spectrogram);

    /*
        Called when user selects a destination folder to export the current audio to
    */
    bool UserAudioExport(QString destinationURL, SpectrogramObject spectrogram);

private:

};

#endif