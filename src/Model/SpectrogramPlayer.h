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

    void    setSource(const QString &url);
    void    play();
    void    pause();
    void    setLooping(bool on);

    qint64  position() const;
    qint64  duration() const;

signals:
    void positionChanged(qint64 posMs);
    void durationChanged(qint64 durMs);
    void playbackStateChanged(QMediaPlayer::PlaybackState state);

private:
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audio  = nullptr;
    bool          m_loop   = false;
};

#endif
