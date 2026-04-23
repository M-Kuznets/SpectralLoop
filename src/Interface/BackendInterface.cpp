#include "BackendInterface.h"
#include <QtConcurrent/QtConcurrent>

BackendInterface::BackendInterface(QObject *parent) : QObject(parent)
{
    m_converter = new SpectrogramConverter(this);
    m_player    = new SpectrogramPlayer(this);

    connect(m_converter, &SpectrogramConverter::spectrogramReady,
            this, &BackendInterface::onSpectrogramReady);
    connect(m_converter, &SpectrogramConverter::conversionFailed,
            this, &BackendInterface::onConversionFailed);
    connect(m_player, &SpectrogramPlayer::positionChanged,
            this, &BackendInterface::onPositionChanged);
    connect(m_player, &SpectrogramPlayer::playbackStateChanged,
            this, &BackendInterface::onPlayStateChanged);
}

// Playback
void BackendInterface::UserAudioImport(const QString &sourceURL)
{
    m_spectrogramObject = SpectrogramObject();
    m_spectrogramObject.sourceURL = sourceURL;
    m_loading = true;
    emit loadingChanged();
    m_player->setSource(sourceURL);
    m_converter->startImport(sourceURL, &m_spectrogramObject);
}

void BackendInterface::UserPlaybackStart()       { if (m_spectrogramObject.isLoaded) m_player->play(); }
void BackendInterface::UserPlaybackPause()       { m_player->pause(); }
void BackendInterface::UserPlaybackLoop(bool on) { m_player->setLooping(on); }

double BackendInterface::playbackProgress() const
{
    qint64 dur = m_player->duration();
    return dur > 0 ? double(m_player->position()) / double(dur) : 0.0;
}

// Tool
void BackendInterface::setTool(const QString &tool)
{
    if (m_currentTool == tool) return;
    m_currentTool = tool;
    emit toolChanged();
}

// Paint
void BackendInterface::paintAt(double normX, double normY)
{
    if (!m_spectrogramObject.isLoaded || m_rebuilding) return;
    if      (m_currentTool == "line")       m_converter->paintLine(float(normX), float(normY));
    else if (m_currentTool == "lightSpray") m_converter->paintSpray(float(normX), float(normY), false);
    else if (m_currentTool == "heavySpray") m_converter->paintSpray(float(normX), float(normY), true);
    else if (m_currentTool == "eraser")     m_converter->erase(float(normX), float(normY));
}

void BackendInterface::commitPaint()
{
    if (!m_spectrogramObject.isLoaded) return;
    m_converter->applyMagnitudeTaper();
    m_converter->regenerateImage();
    startAudioRebuild(true, 8);   // local GL
}

void BackendInterface::applyHarmonicEnhancer()
{
    if (!m_spectrogramObject.isLoaded) return;
    m_converter->applyHarmonicEnhancer();
    m_converter->applyMagnitudeTaper();
    m_converter->regenerateImage();
    startAudioRebuild(false, 5);  // full parallel GL
}

void BackendInterface::applySpectralRepair()
{
    if (!m_spectrogramObject.isLoaded) return;
    m_converter->applySpectralRepair();
    m_converter->applyMagnitudeTaper();
    m_converter->regenerateImage();
    startAudioRebuild(false, 5);
}

void BackendInterface::applySpectralAttenuation(double x1, double y1, double x2, double y2)
{
    if (!m_spectrogramObject.isLoaded) return;
    m_converter->applySpectralAttenuation(float(x1), float(y1), float(x2), float(y2));
    m_converter->applyMagnitudeTaper();
    m_converter->regenerateImage();
    startAudioRebuild(false, 5);
}

void BackendInterface::startAudioRebuild(bool local, int glIterations)
{
    if (m_rebuilding) return;
    m_rebuilding = true;
    emit rebuildingChanged();

    qint64 savedPos   = m_player->position();
    bool   wasPlaying = m_isPlaying;
    SpectrogramConverter *conv = m_converter;

    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this,
            [this, watcher, savedPos, wasPlaying]() {
        QString audioPath = watcher->result();
        watcher->deleteLater();
        if (!audioPath.isEmpty())
            m_player->reloadAt("file://" + audioPath, savedPos, wasPlaying);
        m_rebuilding = false;
        emit rebuildingChanged();
    });

    watcher->setFuture(QtConcurrent::run([conv, local, glIterations]() -> QString {
        return local ? conv->reconstructLocal(glIterations)
                     : conv->reconstructFull(glIterations);
    }));
}

// Private slots
void BackendInterface::onSpectrogramReady(const QString &imagePath)
{
    m_imagePath = imagePath;
    m_loading   = false;
    emit loadingChanged();
    emit spectrogramReady();
    startAudioRebuild(false, 3);   // full parallel GL on first load 
}

void BackendInterface::onConversionFailed(const QString &error)
{
    m_loading = false;
    emit loadingChanged();
    emit importFailed(error);
}

void BackendInterface::onPositionChanged(qint64) { emit positionChanged(); }

void BackendInterface::onPlayStateChanged(QMediaPlayer::PlaybackState state)
{
    m_isPlaying = (state == QMediaPlayer::PlayingState);
    emit playStateChanged();
}
