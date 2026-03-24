import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import SpectralLoop 1.0

ApplicationWindow {
    width: 1280; height: 720
    visible: true
    title: qsTr("SpectralLoop")

    // File button and menu
    Button {
        id: fileButton
        width: 70; height: 30

        text: "file"
        onClicked: fileContextMenu.open()
    }

    Menu {
        id: fileContextMenu
        y: fileButton.y

        MenuItem {
            text: "New"
            onClicked: notImplementedDialog.open()
        }

        MenuItem {
            text: "Import"
            onClicked: importFileDialog.open()
        }

        MenuItem {
            text: "Export"
            onClicked: notImplementedDialog.open()
        }
    }

    FileDialog {
        id: importFileDialog
        title: "Choose file to import"
        
        onAccepted: {

            importLabel.text = BackendInterface.UserAudioImport(selectedFile) ? "Imported" : "Failed to import"
        }
    }


    // Test label
    Label {
        id: importLabel
        text: "none"
        y: 100
    }

    
    //non-implemented dialog
    Dialog {
        id: notImplementedDialog
        title: "Feature not implemented"
        standardButtons: Dialog.Ok
    }



    //Playback

    Button {
        id: playButton
        width: 70; height: 70
        y: 600; x: 100
        text: "Play"
        onClicked: BackendInterface.UserPlaybackStart()
    }

    Button {
        id: pauseButton
        width: 70; height: 70
        y: 600; x: 170
        text: "Pause"
        onClicked: BackendInterface.UserPlaybackPause()
    }
}