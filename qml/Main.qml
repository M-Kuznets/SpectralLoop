import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import SpectralLoop

Window {
    id: root
    width:  1280
    height: 720
    visible: true
    title:   "SpectralLoop"
    color:   "#0b0e14"

    FileDialog {
        id: fileDialog
        title:       "Open WAV File"
        nameFilters: ["WAV files (*.wav)", "All files (*)"]
        onAccepted:  Backend.UserAudioImport(selectedFile.toString())
    }

    Rectangle {
        id: toolbar
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 48
        color:  "#12151e"

        Row {
            anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 16 }
            spacing: 12

            // App name
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text:  "SpectralLoop"
                color: "#e0e4f0"
                font { pixelSize: 16; bold: true; family: "Helvetica Neue" }
            }

            // Separator
            Rectangle { width: 1; height: 24; color: "#2a2f3d"; anchors.verticalCenter: parent.verticalCenter }

            // Open file button
            Button {
                id: openBtn
                anchors.verticalCenter: parent.verticalCenter
                text: "Open WAV"
                enabled: !Backend.isLoading
                onClicked: fileDialog.open()

                contentItem: Text {
                    text:  openBtn.text
                    color: openBtn.enabled ? "#c8d0e8" : "#555"
                    font { pixelSize: 13; family: "Helvetica Neue" }
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment:   Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: openBtn.pressed  ? "#2a3050"
                         : openBtn.hovered  ? "#1e2540"
                         : "#181d2e"
                    border.color: "#3a4060"
                    border.width: 1
                }
                implicitWidth:  96
                implicitHeight: 32
            }
        }

        // Loading indicator
        Row {
            anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: 16 }
            spacing: 8
            visible: Backend.isLoading

            // Spinning dot animation
            Rectangle {
                id: spinner
                width: 10; height: 10; radius: 5
                color: "#4a7fff"
                anchors.verticalCenter: parent.verticalCenter
                SequentialAnimation on opacity {
                    running: Backend.isLoading
                    loops:   Animation.Infinite
                    NumberAnimation { to: 0.2; duration: 600; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 1.0; duration: 600; easing.type: Easing.InOutSine }
                }
            }
            Text {
                text:  "Analysing…"
                color: "#8090b0"
                font { pixelSize: 12; family: "Helvetica Neue" }
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    Item {
        id: spectrogramArea
        anchors {
            top:    toolbar.bottom
            bottom: controls.top
            left:   parent.left
            right:  parent.right
            topMargin:    1
            bottomMargin: 1
        }

        // Placeholder when nothing loaded
        Column {
            anchors.centerIn: parent
            spacing: 14
            visible: !Backend.isLoaded && !Backend.isLoading

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text:  "Open a WAV file to view its spectrogram"
                color: "#3a4060"
                font { pixelSize: 18; family: "Helvetica Neue" }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text:  "Low frequencies → bottom   |   High frequencies → top"
                color: "#2a2f45"
                font { pixelSize: 12; family: "Helvetica Neue" }
            }
        }

        // Spectrogram image
        Image {
            id: spectroImg
            anchors.fill: parent
            source:       Backend.spectrogramImagePath
            fillMode:     Image.Stretch
            visible:      Backend.isLoaded
            cache:        false   // always reread from disk after new analysis
            smooth:       true
            asynchronous: true
        }

        // Frequency axis labels
        Column {
            anchors { left: spectroImg.left; top: spectroImg.top; bottom: spectroImg.bottom }
            width: 44
            visible: Backend.isLoaded

            Repeater {
                model: ["20k", "10k", "5k", "2k", "1k", "500", "200", "50"]
                delegate: Item {
                    width:  44
                    height: spectroImg.height / 8
                    Text {
                        anchors { right: parent.right; rightMargin: 4; verticalCenter: parent.verticalCenter }
                        text:  modelData
                        color: "#404860"
                        font { pixelSize: 10; family: "Helvetica Neue" }
                    }
                }
            }
        }

        // Playback cursor line
        Rectangle {
            visible: Backend.isLoaded
            x:       spectroImg.x + Backend.playbackProgress * spectroImg.width - 1
            y:       spectroImg.y
            width:   2
            height:  spectroImg.height
            color:   "#ffffff"
            opacity: 0.55
            layer.enabled: true
        }
    }

    Rectangle {
        id: controls
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: 72
        color:  "#0e1118"

        Row {
            anchors.centerIn: parent
            spacing: 16

            // Loop button
            ControlButton {
                icon:    "⟳"
                checked: false
                onClicked: {
                    checked = !checked
                    Backend.UserPlaybackLoop(checked)
                }
                enabled: Backend.isLoaded
            }

            // Play Pause 
            ControlButton {
                icon:    Backend.isPlaying ? "⏸" : "▶"
                enabled: Backend.isLoaded
                onClicked: Backend.isPlaying ? Backend.UserPlaybackPause()
                                             : Backend.UserPlaybackStart()
            }
        }
    }

    Rectangle {
        id: errorToast
        anchors { bottom: controls.top; horizontalCenter: parent.horizontalCenter; bottomMargin: 12 }
        width:   errorText.implicitWidth + 32
        height:  36
        radius:  8
        color:   "#3d1a1a"
        border.color: "#7a2020"
        visible: false
        opacity: 0

        Text {
            id: errorText
            anchors.centerIn: parent
            color: "#ff8080"
            font { pixelSize: 13; family: "Helvetica Neue" }
        }

        SequentialAnimation {
            id: toastAnim
            NumberAnimation { target: errorToast; property: "opacity"; to: 1; duration: 200 }
            PauseAnimation  { duration: 3500 }
            NumberAnimation { target: errorToast; property: "opacity"; to: 0; duration: 400 }
            onFinished: errorToast.visible = false
        }
    }

    Connections {
        target: Backend

        function onImportFailed(error) {
            errorText.text   = "Import failed: " + error
            errorToast.visible = true
            toastAnim.restart()
        }
    }

    component ControlButton: Rectangle {
        property string  icon:    "?"
        property bool    checked: false
        signal clicked()

        width:  52; height: 52; radius: 26
        color: pressed  ? "#1e2540"
             : hovered  ? "#181d2e"
             : checked  ? "#1a2545"
             : "#13161f"
        border.color: checked ? "#4a7fff" : "#2a2f44"
        border.width: 1

        property bool hovered: false
        property bool pressed: false
        property bool enabled: true

        Text {
            anchors.centerIn: parent
            text:  parent.icon
            color: parent.enabled ? (parent.checked ? "#7aafff" : "#c0c8e0") : "#3a3f55"
            font.pixelSize: 20
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            enabled:      parent.enabled
            onEntered:    parent.hovered = true
            onExited:     parent.hovered = false
            onPressed:    parent.pressed = true
            onReleased:   parent.pressed = false
            onClicked:    parent.clicked()
        }
    }
}
