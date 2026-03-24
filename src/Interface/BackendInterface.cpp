#include"src/Interface/BackendInterface.h"


bool BackendInterface::UserAudioImport(QString sourceURL) {
    if (spectrogramConverter.UserAudioImport(sourceURL, spectrogramObject)) {
        return spectrogramPlayer.SetSpectrogram(spectrogramObject);
    }
    return false;
    //return "Received: " + url;
}

bool BackendInterface::UserAudioExport(QString destinationURL) {
    // TODO
    return spectrogramConverter.UserAudioImport(destinationURL, spectrogramObject);
}

bool BackendInterface::UserPlaybackStart() {
    // TODO
    return spectrogramPlayer.UserPlaybackStart(spectrogramObject);
}

bool BackendInterface::UserPlaybackPause() {
    // TODO
    return spectrogramPlayer.UserPlaybackStart(spectrogramObject);
}

bool BackendInterface::UserPlaybackLoop(bool toggleOn) {
    // TODO
    return spectrogramPlayer.UserPlaybackStart(spectrogramObject);
}
