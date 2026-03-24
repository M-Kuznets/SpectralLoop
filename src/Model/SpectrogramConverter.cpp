#include "SpectrogramConverter.h"

SpectrogramConverter::SpectrogramConverter()
{

}

SpectrogramConverter::~SpectrogramConverter()
{

}

/*
        Called when user selects a file to import
        Returns sucessful or not
    */
bool SpectrogramConverter::UserAudioImport(QString sourceURL, SpectrogramObject spectrogram) {
    return true;
}

    /*
        Called when user selects a destination folder to export the current audio to
    */
bool SpectrogramConverter::UserAudioExport(QString destinationURL, SpectrogramObject spectrogram) {
    return true;
}