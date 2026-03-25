#include "BackendInterface.h"
#include <QMediaPlayer>

BackendInterface::BackendInterface(QObject *parent) : QObject(parent)
{
    m_converter = new SpectrogramConverter(this);
    m_player    = new SpectrogramPlayer(this);

    connect(m_converter, &SpectrogramConverter::spectrogramReady,
            this,        &BackendInterface::onSpectrogramReady);

    connect(m_converter, &SpectrogramConverter::conversionFailed,
            this,        &BackendInterface::onConversionFailed);

    connect(m_player, &SpectrogramPlayer::positionChanged,
            this,     &BackendInterface::onPositionChanged);

    connect(m_player, &SpectrogramPlayer::playbackStateChanged,
            this,     &BackendInterface::onPlayStateChanged);
}

void BackendInterface::UserAudioImport(const QString &sourceURL)
{
    m_spectrogramObject = SpectrogramObject();
    m_spectrogramObject.sourceURL = sourceURL;
    m_loading = true;
    emit loadingChanged();

    m_player->setSource(sourceURL);
    m_converter->startImport(sourceURL, &m_spectrogramObject);
}

void BackendInterface::UserPlaybackStart()
{
    if (m_spectrogramObject.isLoaded)
        m_player->play();
}

void BackendInterface::UserPlaybackPause()
{
    m_player->pause();
}

void BackendInterface::UserPlaybackLoop(bool toggleOn)
{
    m_player->setLooping(toggleOn);
}

double BackendInterface::playbackProgress() const
{
    qint64 dur = m_player->duration();
    if (dur <= 0) return 0.0;
    return static_cast<double>(m_player->position()) / static_cast<double>(dur);
}

void BackendInterface::onSpectrogramReady(const QString &imagePath)
{
    m_imagePath = imagePath;
    m_loading   = false;
    emit loadingChanged();
    emit spectrogramReady();
}

void BackendInterface::onConversionFailed(const QString &error)
{
    m_loading = false;
    emit loadingChanged();
    emit importFailed(error);
}

void BackendInterface::onPositionChanged(qint64)
{
    emit positionChanged();
}

void BackendInterface::onPlayStateChanged(QMediaPlayer::PlaybackState state)
{
    m_isPlaying = (state == QMediaPlayer::PlayingState);
    emit playStateChanged();
}
