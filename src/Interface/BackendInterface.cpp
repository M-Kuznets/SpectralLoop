#include"src/Interface/BackendInterface.h"

bool BackendInterface::UserAudioImport(QString sourceURL) {
    return true;
    //return "Received: " + url;
}



bool BackendInterface::UserAudioExport(QString destinationURL) {
    // TODO
    return false;
}

bool BackendInterface::UserPlaybackStart() {
    // TODO
    return false;
}

bool BackendInterface::UserPlaybackPause() {
    // TODO
    return false;
}

bool BackendInterface::UserPlaybackLoop(bool toggleOn) {
    // TODO
    return false;
}
