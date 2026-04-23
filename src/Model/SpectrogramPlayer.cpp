#include "SpectrogramPlayer.h"

SpectrogramPlayer::SpectrogramPlayer(QObject *parent) : QObject(parent)
{
    m_player = new QMediaPlayer(this);
    m_audio  = new QAudioOutput(this);
    m_player->setAudioOutput(m_audio);
    m_audio->setVolume(1.0f);

    connect(m_player, &QMediaPlayer::positionChanged,
            this,     &SpectrogramPlayer::positionChanged);
    connect(m_player, &QMediaPlayer::durationChanged,
            this,     &SpectrogramPlayer::durationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged,
            this,     &SpectrogramPlayer::playbackStateChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged,
            this,     &SpectrogramPlayer::onMediaStatusChanged);
}

SpectrogramPlayer::~SpectrogramPlayer() {}

void SpectrogramPlayer::setSource(const QString &url)
{
    m_pendingPos  = -1;
    m_pendingPlay = false;
    m_player->setSource(QUrl(url));
}

// Reload a new source (edited WAV), seek to posMs, optionally resume playback.
void SpectrogramPlayer::reloadAt(const QString &url, qint64 posMs, bool shouldPlay)
{
    m_pendingPos  = posMs;
    m_pendingPlay = shouldPlay;
    m_player->setSource(QUrl(url));
    // onMediaStatusChanged will seek + play once the file is loaded
}

void SpectrogramPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (m_loop && status == QMediaPlayer::EndOfMedia) {
        m_player->setPosition(0);
        m_player->play();
        return;
    }

    // After reloadAt: seek to saved position and optionally resume
    if ((status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia)
        && m_pendingPos >= 0)
    {
        m_player->setPosition(m_pendingPos);
        m_pendingPos = -1;
        if (m_pendingPlay)
            m_player->play();
    }
}

void SpectrogramPlayer::play()  { m_player->play();  }
void SpectrogramPlayer::pause() { m_player->pause(); }
void SpectrogramPlayer::setLooping(bool on) { m_loop = on; }
qint64 SpectrogramPlayer::position() const { return m_player->position(); }
qint64 SpectrogramPlayer::duration() const { return m_player->duration(); }
