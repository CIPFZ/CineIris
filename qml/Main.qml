import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs
import FluentUI

ApplicationWindow {
    id: mainWindow

    visible: true
    width: 1100
    height: 720
    minimumWidth: 800
    minimumHeight: 500
    title: "CineIris Studio"

    // Set initial dark mode via FluTheme
    Component.onCompleted: FluTheme.darkMode = 2

    Action {
        shortcut: "Ctrl+T"
        onTriggered: FluTheme.darkMode = (FluTheme.darkMode === 2) ? 1 : 2
    }

    Connections {
        target: controller
        function onErrorOccurred(msg) {
            errorToast.text = msg
            errorToast.visible = true
            errorToastTimer.restart()
        }
    }

    // Background
    Rectangle {
        anchors.fill: parent
        color: FluTheme.dark ? "#1C1C1E" : "#F5F5F7"
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        // === Left Panel (340px) ===
        Rectangle {
            Layout.preferredWidth: 340
            Layout.fillHeight: true
            color: FluTheme.dark ? "#2C2C2E" : "#FFFFFF"
            radius: 14

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                // Header
                RowLayout {
                    FluText {
                        text: "CineIris"
                        font: FluTextStyle.TitleLarge
                    }
                    FluText {
                        text: "v2.0"
                        color: FluTheme.fontSecondaryColor
                        font: FluTextStyle.Caption
                    }
                    Item { Layout.fillWidth: true }

                    // Theme toggle
                    FluButton {
                        text: FluTheme.dark ? "☀" : "☾"
                        onClicked: FluTheme.darkMode = (FluTheme.darkMode === 2) ? 1 : 2
                    }
                }

                FluDivider { size: 1 }

                // Drop zone
                VideoDropZone {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    videoPath: {
                        if (!controller.videoPath) return ""
                        var parts = controller.videoPath.split("/")
                        return parts[parts.length - 1]
                    }
                    onFileSelected: function(path) { controller.loadVideo(path) }
                }

                // Params panel
                ParamsPanel {
                    id: paramsPanel
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    isIris: controller.isIris
                    outputSize: controller.outputSize
                    sampleCount: controller.sampleCount
                    status: controller.status

                    onTypeChanged: function(v) { controller.isIris = v }
                    onSizeChanged: function(v) { controller.outputSize = v }
                    onCountChanged: function(v) { controller.sampleCount = v }
                    onStartClicked: controller.start()
                    onPauseClicked: controller.pause()
                    onResumeClicked: controller.resume()
                    onCancelClicked: controller.cancel()
                    onSaveClicked: saveDialog.open()
                }

                // Progress
                FluProgressBar {
                    Layout.fillWidth: true
                    value: controller.progress
                    indeterminate: controller.status === 1
                    visible: controller.status === 1 || controller.status === 2
                }
            }
        }

        // === Right Panel: Result Preview ===
        ResultPreview {
            id: resultPreview
            Layout.fillWidth: true
            Layout.fillHeight: true
            resultImage: controller.status === 3 ? controller.resultImage : null
            onSaveRequested: saveDialog.open()
        }
    }

    // Error toast
    Rectangle {
        id: errorToast
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
        anchors.bottomMargin: 20
        width: errorText.implicitWidth + 32; height: 40
        radius: 6
        color: "#FF453A"
        visible: false

        FluText {
            id: errorText
            anchors.centerIn: parent
            text: ""
            color: "#FFFFFF"
            font: FluTextStyle.Body
        }

        Timer {
            id: errorToastTimer
            interval: 3000
            onTriggered: errorToast.visible = false
        }
    }

    // Save file dialog
    FileDialog {
        id: saveDialog
        title: "保存结果"
        fileMode: FileDialog.SaveFile
        nameFilters: ["PNG (*.png)"]
        defaultSuffix: "png"
        onAccepted: {
            if (selectedFile) {
                var path = selectedFile.toString()
                if (path.startsWith("file://"))
                    path = path.substring(7)
                controller.saveResult(path)
            }
        }
    }
}
