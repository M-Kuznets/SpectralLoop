#ifndef SPECTROGRAMPLAYER_H
#define SPECTROGRAMPLAYER_H

#pragma once

#include<QObject>
#include<QtMultimedia/QMediaPlayer>
#include<QtMultimedia/QAudioBuffer>
#include"SpectrogramObject.h"

class SpectrogramPlayer
{
public:
    SpectrogramObject spectrogramObject;

    SpectrogramPlayer();
    ~SpectrogramPlayer();

    /*
        
    */
    bool SetSpectrogram(SpectrogramObject spectrogram);

    /*
        Called when user hits the play button
    */
    bool UserPlaybackStart(SpectrogramObject spectrogram);

    /*
        Called when user hits the pause button
    */
    bool UserPlaybackPause(SpectrogramObject spectrogram);

    /*
        Called when user toggles the loop button
    */
    bool UserPlaybackLoop(bool toggleOn, SpectrogramObject spectrogram);

private:

};

#endif