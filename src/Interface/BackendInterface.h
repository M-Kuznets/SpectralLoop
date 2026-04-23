#ifndef BACKENDINTERFACE_H
#define BACKENDINTERFACE_H
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QMediaPlayer>
#include <QFutureWatcher>
#include "SpectrogramObject.h"
#include "SpectrogramConverter.h"
#include "SpectrogramPlayer.h"

class BackendInterface : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(Backend)

    Q_PROPERTY(bool    isLoading        READ isLoading        NOTIFY loadingChanged)
    Q_PROPERTY(bool    isRebuilding     READ isRebuilding     NOTIFY rebuildingChanged)
    Q_PROPERTY(bool    isLoaded         READ isLoaded         NOTIFY spectrogramReady)
    Q_PROPERTY(QString spectrogramImagePath READ spectrogramImagePath NOTIFY spectrogramReady)
    Q_PROPERTY(double  playbackProgress READ playbackProgress  NOTIFY positionChanged)
    Q_PROPERTY(bool    isPlaying        READ isPlaying         NOTIFY playStateChanged)
    Q_PROPERTY(QString currentTool      READ currentTool       NOTIFY toolChanged)

public:
    explicit BackendInterface(QObject *parent = nullptr);

    bool    isLoading()            const { return m_loading; }
    bool    isRebuilding()         const { return m_rebuilding; }
    bool    isLoaded()             const { return m_spectrogramObject.isLoaded; }
    QString spectrogramImagePath() const { return m_imagePath; }
    double  playbackProgress()     const;
    bool    isPlaying()            const { return m_isPlaying; }
    QString currentTool()          const { return m_currentTool; }

    Q_INVOKABLE void UserAudioImport(const QString &sourceURL);
    Q_INVOKABLE void UserPlaybackStart();
    Q_INVOKABLE void UserPlaybackPause();
    Q_INVOKABLE void UserPlaybackLoop(bool toggleOn);

    Q_INVOKABLE void setTool(const QString &tool);
    Q_INVOKABLE void paintAt(double normX, double normY);

    // Call on mouse release: regenerates image + rebuilds audio on background thread
    Q_INVOKABLE void commitPaint();

    Q_INVOKABLE void applyHarmonicEnhancer();
    Q_INVOKABLE void applySpectralRepair();
    Q_INVOKABLE void applySpectralAttenuation(double x1, double y1, double x2, double y2);

signals:
    void spectrogramReady();
    void importFailed(const QString &error);
    void loadingChanged();
    void rebuildingChanged();
    void positionChanged();
    void playStateChanged();
    void toolChanged();

private slots:
    void onSpectrogramReady(const QString &imagePath);
    void onConversionFailed(const QString &error);
    void onPositionChanged(qint64 pos);
    void onPlayStateChanged(QMediaPlayer::PlaybackState state);

private:
    // local=true: only rebuilds the dirty frame region (fast, for paint strokes)
    // local=false: rebuilds the entire file (slower, for whole-file features)
    void startAudioRebuild(bool local, int glIterations);

    SpectrogramObject     m_spectrogramObject;
    SpectrogramConverter *m_converter  = nullptr;
    SpectrogramPlayer    *m_player     = nullptr;

    QString m_imagePath;
    QString m_currentTool = "none";
    bool    m_loading     = false;
    bool    m_rebuilding  = false;
    bool    m_isPlaying   = false;
};

#endif
