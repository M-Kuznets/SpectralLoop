#ifndef BACKENDINTERFACE_H
#define BACKENDINTERFACE_H
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include "SpectrogramObject.h"
#include "SpectrogramConverter.h"
#include "SpectrogramPlayer.h"

class BackendInterface : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(Backend)

    Q_PROPERTY(bool    isLoading            READ isLoading            NOTIFY loadingChanged)
    Q_PROPERTY(bool    isLoaded             READ isLoaded             NOTIFY spectrogramReady)
    Q_PROPERTY(QString spectrogramImagePath READ spectrogramImagePath NOTIFY spectrogramReady)
    Q_PROPERTY(double  playbackProgress     READ playbackProgress     NOTIFY positionChanged)
    Q_PROPERTY(bool    isPlaying            READ isPlaying            NOTIFY playStateChanged)

public:
    explicit BackendInterface(QObject *parent = nullptr);

    bool    isLoading()            const { return m_loading; }
    bool    isLoaded()             const { return m_spectrogramObject.isLoaded; }
    QString spectrogramImagePath() const { return m_imagePath; }
    double  playbackProgress()     const;
    bool    isPlaying()            const { return m_isPlaying; }

    Q_INVOKABLE void UserAudioImport(const QString &sourceURL);
    Q_INVOKABLE void UserPlaybackStart();
    Q_INVOKABLE void UserPlaybackPause();
    Q_INVOKABLE void UserPlaybackLoop(bool toggleOn);

signals:
    void spectrogramReady();
    void importFailed(const QString &error);
    void loadingChanged();
    void positionChanged();
    void playStateChanged();

private slots:
    void onSpectrogramReady(const QString &imagePath);
    void onConversionFailed(const QString &error);
    void onPositionChanged(qint64 pos);
    void onPlayStateChanged(QMediaPlayer::PlaybackState state);

private:
    SpectrogramObject  m_spectrogramObject;
    SpectrogramConverter *m_converter = nullptr;
    SpectrogramPlayer    *m_player    = nullptr;

    QString m_imagePath;
    bool    m_loading   = false;
    bool    m_isPlaying = false;
};

#endif