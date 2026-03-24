#ifndef BACKENDINTERFACE_H
#define BACKENDINTERFACE_H

#include<QObject>
//#include<qqml.h>
#include<QTQmL>

#include<src/Model/SpectrogramObject.h>
#include<src/Model/SpectrogramPlayer.h>
#include<src/Model/SpectrogramConverter.h>

class BackendInterface : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

public:

    // Variables
    SpectrogramObject spectrogramObject;
    SpectrogramPlayer spectrogramPlayer;
    SpectrogramConverter spectrogramConverter;

    // Singleton constructor
    BackendInterface(QObject *parent = nullptr) : QObject(parent) {
        spectrogramObject = SpectrogramObject();
        spectrogramPlayer = SpectrogramPlayer();
        spectrogramConverter = SpectrogramConverter();
    };

    /*
        Called when user selects a file to import
        Returns sucessful or not
    */
    Q_INVOKABLE bool UserAudioImport(QString sourceURL);

    /*
        Called when user selects a destination folder to export the current audio to
    */
    Q_INVOKABLE bool UserAudioExport(QString destinationURL);



    /*
        Called when user hits the play button
    */
    Q_INVOKABLE bool UserPlaybackStart();

    /*
        Called when user hits the pause button
    */
    Q_INVOKABLE bool UserPlaybackPause();

    /*
        Called when user toggles the loop button
    */
    Q_INVOKABLE bool UserPlaybackLoop(bool toggleOn);
};

#endif