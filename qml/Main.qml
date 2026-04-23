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

    property string paintTool: "none"

    // Spectral Attenuation selection state
    property bool   attenuationMode:  false
    property bool   hasSelection:     false
    property real   selX1: 0;  property real selY1: 0
    property real   selX2: 0;  property real selY2: 0

    FileDialog {
        id: fileDialog
        title:       "Open WAV File"
        nameFilters: ["WAV files (*.wav)", "All files (*)"]
        onAccepted: {
            root.paintTool      = "none"
            root.attenuationMode = false
            root.hasSelection   = false
            Backend.setTool("none")
            Backend.UserAudioImport(selectedFile.toString())
        }
    }

    Rectangle {
        id: toolbar
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 52
        color:  "#0e1118"
        layer.enabled: true
        layer.effect: null

        Row {
            anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 16 }
            spacing: 10

            // App name
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "SpectralLoop"
                color: "#c8d4f0"
                font { pixelSize: 15; bold: true }
            }

            ToolSep {}

            // Open WAV
            TBtn {
                label:   "Open WAV"
                enabled: !Backend.isLoading
                onActivated: fileDialog.open()
            }

            ToolSep {}

            // Paint tool dropdown
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "Paint"
                color: "#606880"
                font.pixelSize: 12
            }

            ToolDropdown {
                id: paintDropdown
                model: ["None", "Line", "Light Spray", "Heavy Spray", "Eraser"]
                enabled: Backend.isLoaded && !Backend.isLoading && !Backend.isRebuilding && !root.attenuationMode
                onSelected: function(index) {
                    const tools = ["none","line","lightSpray","heavySpray","eraser"]
                    root.paintTool = tools[index]
                    Backend.setTool(tools[index])
                }
            }

            ToolSep {}

            // Features dropdown
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "Features"
                color: "#606880"
                font.pixelSize: 12
            }

            ToolDropdown {
                id: featureDropdown
                model: ["Select Feature", "Harmonic Enhancer", "Spectral Repair", "Spectral Attenuation"]
                enabled: Backend.isLoaded && !Backend.isLoading && !Backend.isRebuilding
                onSelected: function(index) {
                    if (index === 0) return
                    if (index === 1) {
                        Backend.applyHarmonicEnhancer()
                    } else if (index === 2) {
                        Backend.applySpectralRepair()
                    } else if (index === 3) {
                        // Enter attenuation selection mode
                        root.paintTool = "none"
                        Backend.setTool("none")
                        paintDropdown.currentIndex = 0
                        root.attenuationMode  = true
                        root.hasSelection     = false
                    }
                    // Reset feature dropdown to placeholder
                    featureDropdown.currentIndex = 0
                }
            }
        }

        // Loading / rebuilding indicator
        Row {
            anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: 16 }
            spacing: 8
            visible: Backend.isLoading || Backend.isRebuilding
            Rectangle {
                width: 8; height: 8; radius: 4
                color: Backend.isRebuilding ? "#ff9f40" : "#4a7fff"
                anchors.verticalCenter: parent.verticalCenter
                SequentialAnimation on opacity {
                    running: Backend.isLoading || Backend.isRebuilding
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.2; duration: 600; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 1.0; duration: 600; easing.type: Easing.InOutSine }
                }
            }
            Text {
                text:  Backend.isRebuilding ? "Rebuilding audio…" : "Analysing…"
                color: "#606880"; font.pixelSize: 12
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // Attenuation mode badge
        Rectangle {
            anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: 16 }
            visible: root.attenuationMode
            width: attBadgeRow.implicitWidth + 24; height: 28; radius: 6
            color: "#1a2040"; border.color: "#4a7fff"; border.width: 1

            Row {
                id: attBadgeRow
                anchors.centerIn: parent
                spacing: 8
                Text {
                    text: "✦ Draw selection on spectrogram, then Apply"
                    color: "#7aafff"; font.pixelSize: 12
                    anchors.verticalCenter: parent.verticalCenter
                }
                Rectangle {
                    width: 1; height: 16; color: "#3a4060"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "Cancel"
                    color: "#ff6060"; font.pixelSize: 12
                    anchors.verticalCenter: parent.verticalCenter
                    MouseArea {
                        anchors.fill: parent
                        onClicked: { root.attenuationMode = false; root.hasSelection = false }
                        cursorShape: Qt.PointingHandCursor
                    }
                }
            }
        }
    }

    Item {
        id: spectrogramArea
        anchors {
            top:    toolbar.bottom; bottom: controls.top
            left:   parent.left;   right:  parent.right
            topMargin: 1; bottomMargin: 1
        }

        // Empty state
        Column {
            anchors.centerIn: parent
            spacing: 12
            visible: !Backend.isLoaded && !Backend.isLoading
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Open a WAV file to view its spectrogram"
                color: "#2e3450"; font.pixelSize: 18
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Low frequencies → bottom   |   High frequencies → top"
                color: "#1e2235"; font.pixelSize: 12
            }
        }

        // Spectrogram image
        Image {
            id: spectroImg
            anchors.fill: parent
            source:       Backend.spectrogramImagePath
            fillMode:     Image.Stretch
            visible:      Backend.isLoaded
            cache:        false
            smooth:       true
            asynchronous: true
        }

        // Playback cursor
        Rectangle {
            visible: Backend.isLoaded
            x:       spectroImg.x + Backend.playbackProgress * spectroImg.width - 1
            width:   2; height: spectroImg.height; y: spectroImg.y
            color:   "#ffffff"; opacity: 0.5
        }

        // Mouse area for painting & attenuation selection
        MouseArea {
            id: paintArea
            anchors.fill: spectroImg
            enabled: Backend.isLoaded && (root.paintTool !== "none" || root.attenuationMode)
            hoverEnabled: true

            cursorShape: {
                if (root.attenuationMode)        return Qt.CrossCursor
                if (root.paintTool === "eraser") return Qt.BlankCursor
                if (root.paintTool !== "none")   return Qt.BlankCursor
                return Qt.ArrowCursor
            }

            // Paint stroke
            onPressed: function(mouse) {
                if (root.attenuationMode) {
                    // Start selection rect
                    root.selX1 = mouse.x / width
                    root.selY1 = mouse.y / height
                    root.selX2 = root.selX1
                    root.selY2 = root.selY1
                    root.hasSelection = true
                } else if (root.paintTool !== "none") {
                    Backend.paintAt(mouse.x / width, mouse.y / height)
                }
            }

            onPositionChanged: function(mouse) {
                if (root.attenuationMode && pressed) {
                    root.selX2 = Math.max(0, Math.min(1, mouse.x / width))
                    root.selY2 = Math.max(0, Math.min(1, mouse.y / height))
                } else if (root.paintTool !== "none" && pressed) {
                    Backend.paintAt(mouse.x / width, mouse.y / height)
                }
            }

            onReleased: function(mouse) {
                if (!root.attenuationMode && root.paintTool !== "none")
                    Backend.commitPaint()
            }
        }

        // Brush cursor indicator 
        Item {
            id: brushCursor
            visible: Backend.isLoaded && root.paintTool !== "none" && !root.attenuationMode
                     && paintArea.containsMouse
            x: paintArea.mouseX - width/2
            y: paintArea.mouseY - height/2

            width:  brushSize(); height: brushSize()

            function brushSize() {
                if (root.paintTool === "line")       return 6
                if (root.paintTool === "lightSpray") return 16
                if (root.paintTool === "heavySpray") return 40
                if (root.paintTool === "eraser")     return 28
                return 6
            }

            Rectangle {
                anchors.fill: parent
                radius:       width / 2
                color:        "transparent"
                border.color: root.paintTool === "eraser" ? "#ff6060" : "#ffffff"
                border.width: 1.5
                opacity:      0.75
            }
            // Centre dot
            Rectangle {
                anchors.centerIn: parent
                width: 3; height: 3; radius: 1.5
                color: root.paintTool === "eraser" ? "#ff6060" : "#ffffff"
                opacity: 0.9
            }
        }

        // Attenuation selection rectangle 
        Rectangle {
            visible: root.attenuationMode && root.hasSelection
            x:       Math.min(root.selX1, root.selX2) * spectroImg.width  + spectroImg.x
            y:       Math.min(root.selY1, root.selY2) * spectroImg.height + spectroImg.y
            width:   Math.abs(root.selX2 - root.selX1) * spectroImg.width
            height:  Math.abs(root.selY2 - root.selY1) * spectroImg.height
            color:      "#4a7fff18"
            border.color: "#4a7fff"
            border.width: 1.5

            // Apply Attenuation button
            Rectangle {
                anchors { right: parent.right; bottom: parent.bottom; margins: 6 }
                visible: parent.width > 80 && parent.height > 36
                width:  applyLabel.implicitWidth + 24
                height: 28
                radius: 6
                color:  "#1a2550"
                border.color: "#4a7fff"
                border.width: 1

                Text {
                    id: applyLabel
                    anchors.centerIn: parent
                    text:  "Apply Attenuation"
                    color: "#7aafff"
                    font.pixelSize: 12
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        Backend.applySpectralAttenuation(
                            root.selX1, root.selY1,
                            root.selX2, root.selY2
                        )
                        root.attenuationMode = false
                        root.hasSelection    = false
                    }
                }
            }
        }

        // Freq axis labels
        Column {
            anchors { left: spectroImg.left; top: spectroImg.top; bottom: spectroImg.bottom }
            width: 44
            visible: Backend.isLoaded

            Repeater {
                model: ["22k","10k","5k","2k","1k","500","200","50"]
                delegate: Item {
                    width:  44
                    height: spectroImg.height / 8
                    Text {
                        anchors { right: parent.right; rightMargin: 4; verticalCenter: parent.verticalCenter }
                        text: modelData; color: "#404860"; font.pixelSize: 10
                    }
                }
            }
        }
    }

    Rectangle {
        id: controls
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: 68
        color: "#0a0d12"

        Row {
            anchors.centerIn: parent
            spacing: 14

            CtrlBtn { icon: "⟳"; checkable: true; enabled: Backend.isLoaded
                      onToggled: function(on) { Backend.UserPlaybackLoop(on) } }

            CtrlBtn {
                icon:    Backend.isPlaying ? "⏸" : "▶"
                enabled: Backend.isLoaded
                onActivated: Backend.isPlaying ? Backend.UserPlaybackPause()
                                               : Backend.UserPlaybackStart()
            }
        }
    }

    Rectangle {
        id: toast
        anchors { bottom: controls.top; horizontalCenter: parent.horizontalCenter; bottomMargin: 10 }
        width: toastText.implicitWidth + 32; height: 34; radius: 8
        color: "#3d1a1a"; border.color: "#7a2020"; visible: false; opacity: 0
        Text { id: toastText; anchors.centerIn: parent; color: "#ff8080"; font.pixelSize: 13 }
        SequentialAnimation {
            id: toastAnim
            NumberAnimation { target: toast; property: "opacity"; to: 1; duration: 200 }
            PauseAnimation  { duration: 3500 }
            NumberAnimation { target: toast; property: "opacity"; to: 0; duration: 400 }
            onFinished: toast.visible = false
        }
    }

    Connections {
        target: Backend
        function onImportFailed(error) {
            toastText.text = "Import failed: " + error
            toast.visible  = true
            toastAnim.restart()
        }
    }

    // Vertical separator in toolbar
    component ToolSep: Rectangle {
        width: 1; height: 28
        color: "#1e2235"
        anchors.verticalCenter: parent !== null ? parent.verticalCenter : undefined
    }

    // Simple text button
    component TBtn: Rectangle {
        property string label: ""
        property bool   enabled: true
        signal activated()

        width: tBtnLabel.implicitWidth + 22; height: 30; radius: 6
        color: tBtnMa.pressed ? "#1e2540" : tBtnMa.containsMouse ? "#181c2c" : "#13161f"
        border.color: "#2a2f44"; border.width: 1
        opacity: enabled ? 1 : 0.4

        Text {
            id: tBtnLabel
            anchors.centerIn: parent
            text: parent.label; color: "#b0b8d0"; font.pixelSize: 13
        }
        MouseArea {
            id: tBtnMa
            anchors.fill: parent
            hoverEnabled: true
            enabled: parent.enabled
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.activated()
        }
    }

    // Dropdown menu
    component ToolDropdown: Item {
        id: tdRoot
        property var    model:        []
        property int    currentIndex: 0
        property bool   enabled:      true
        signal selected(int index)

        width:  tdLabel.implicitWidth + 32; height: 30

        // Button face
        Rectangle {
            anchors.fill: parent; radius: 6
            color:        tdMa.pressed ? "#1e2540" : tdMa.containsMouse ? "#181c2c" : "#13161f"
            border.color: tdPopup.opened ? "#4a7fff" : "#2a2f44"; border.width: 1
            opacity:      tdRoot.enabled ? 1 : 0.4

            Row {
                anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 10 }
                spacing: 6
                Text {
                    id: tdLabel
                    text:  tdRoot.model[tdRoot.currentIndex] ?? ""
                    color: "#b0b8d0"; font.pixelSize: 13
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "▾"; color: "#606880"; font.pixelSize: 10
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                id: tdMa
                anchors.fill: parent
                hoverEnabled: true
                enabled: tdRoot.enabled
                cursorShape: Qt.PointingHandCursor
                onClicked: tdPopup.opened ? tdPopup.close() : tdPopup.open()
            }
        }

        // Popup
        Popup {
            id: tdPopup
            // Position just below the button face
            y: tdRoot.height + 4
            width:   Math.max(tdRoot.width, 160)
            height:  tdRoot.model.length * 32 + 8
            padding: 0
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

            background: Rectangle {
                color:        "#151820"
                radius:       8
                border.color: "#2a2f44"
                border.width: 1
            }

            contentItem: Column {
                anchors { fill: parent; margins: 4 }
                spacing: 0
                Repeater {
                    model: tdRoot.model
                    delegate: Rectangle {
                        width:  tdPopup.width - 8; height: 32; radius: 5
                        color:  rowMa.containsMouse ? "#1e2540" : "transparent"
                        Text {
                            anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 12 }
                            text:  modelData
                            color: index === tdRoot.currentIndex ? "#7aafff" : "#b0b8d0"
                            font.pixelSize: 13
                        }
                        MouseArea {
                            id: rowMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                tdRoot.currentIndex = index
                                tdPopup.close()
                                tdRoot.selected(index)
                            }
                        }
                    }
                }
            }
        }
    }

    // Circular playback control button
    component CtrlBtn: Rectangle {
        property string icon:      "?"
        property bool   enabled:   true
        property bool   checkable: false
        property bool   checked:   false
        signal activated()
        signal toggled(bool on)

        width: 48; height: 48; radius: 24
        color: ctrlMa.pressed ? "#1a2040"
             : ctrlMa.containsMouse ? "#161c2e"
             : checked ? "#141c38" : "#0e1118"
        border.color: checked ? "#4a7fff" : "#1e2438"; border.width: 1
        opacity: enabled ? 1 : 0.35

        Text {
            anchors.centerIn: parent
            text:  parent.icon
            color: parent.checked ? "#7aafff" : "#9098b8"
            font.pixelSize: 19
        }
        MouseArea {
            id: ctrlMa
            anchors.fill: parent
            hoverEnabled: true
            enabled: parent.enabled
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (parent.checkable) {
                    parent.checked = !parent.checked
                    parent.toggled(parent.checked)
                } else {
                    parent.activated()
                }
            }
        }
    }
}
