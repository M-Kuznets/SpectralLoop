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

    // Loop support
    connect(m_player, &QMediaPlayer::mediaStatusChanged,
            this, [this](QMediaPlayer::MediaStatus status) {
        if (m_loop && status == QMediaPlayer::EndOfMedia) {
            m_player->setPosition(0);
            m_player->play();
        }
    });
}

SpectrogramPlayer::~SpectrogramPlayer() {}

void SpectrogramPlayer::setSource(const QString &url)
{
    m_player->setSource(QUrl(url));
}

void SpectrogramPlayer::play()  { m_player->play();  }
void SpectrogramPlayer::pause() { m_player->pause(); }

void SpectrogramPlayer::setLooping(bool on) { m_loop = on; }

qint64 SpectrogramPlayer::position() const { return m_player->position(); }
qint64 SpectrogramPlayer::duration() const { return m_player->duration(); }
