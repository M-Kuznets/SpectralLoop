#include "SpectrogramPlayer.h"

SpectrogramPlayer::SpectrogramPlayer()
{

}

SpectrogramPlayer::~SpectrogramPlayer()
{

}



bool SpectrogramPlayer::SetSpectrogram(SpectrogramObject spectrogram) {
    spectrogramObject = spectrogram;
    return true;
}


/*
    Called when user hits the play button
*/
bool SpectrogramPlayer::UserPlaybackStart(SpectrogramObject spectrogram) {
    return true;
}

/*
    Called when user hits the pause button
*/
bool SpectrogramPlayer::UserPlaybackPause(SpectrogramObject spectrogram) {
    return true;
}

/*
    Called when user toggles the loop button
*/
bool SpectrogramPlayer::UserPlaybackLoop(bool toggleOn, SpectrogramObject spectrogram) {
    return true;
}