#ifndef SPECTROGRAMPLAYER_H
#define SPECTROGRAMPLAYER_H
#pragma once

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>

class SpectrogramPlayer : public QObject
{
    Q_OBJECT

public:
    explicit SpectrogramPlayer(QObject *parent = nullptr);
    ~SpectrogramPlayer();

    void setSource(const QString &url);

    // Reload a new source (reconstructed WAV) at a given position in ms,
    // optionally resuming playback. Called after every edit.
    void reloadAt(const QString &url, qint64 posMs, bool shouldPlay);

    void   play();
    void   pause();
    void   setLooping(bool on);
    qint64 position() const;
    qint64 duration() const;

signals:
    void positionChanged(qint64 posMs);
    void durationChanged(qint64 durMs);
    void playbackStateChanged(QMediaPlayer::PlaybackState state);

private slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

private:
    QMediaPlayer *m_player      = nullptr;
    QAudioOutput *m_audio       = nullptr;
    bool          m_loop        = false;
    qint64        m_pendingPos  = -1;
    bool          m_pendingPlay = false;
};

#endif
