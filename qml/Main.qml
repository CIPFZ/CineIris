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
    minimumWidth: 960
    minimumHeight: 640
    title: "CineIris Studio"

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

        // ============================================================
        // Left Panel
        // ============================================================
        Rectangle {
            Layout.preferredWidth: 340
            Layout.minimumWidth: 280
            Layout.fillHeight: true
            color: FluTheme.dark ? "#2C2C2E" : "#FFFFFF"
            radius: 14
            clip: true

            // Fixed header + scrollable body + fixed footer
            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // --- Header (fixed) ---
                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: 0

                    FluText {
                        text: "CineIris"
                        font: FluTextStyle.Subtitle
                    }
                    FluText {
                        text: "v2.0"
                        color: FluTheme.primaryColor
                        font: FluTextStyle.Caption
                    }
                    Item { Layout.fillWidth: true }
                    FluButton {
                        text: FluTheme.dark ? "☀" : "☾"
                        onClicked: FluTheme.darkMode = (FluTheme.darkMode === 2) ? 1 : 2
                    }
                }

                FluDivider {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    Layout.topMargin: 12
                    Layout.bottomMargin: 0
                    size: 1
                }

                // --- Drop zone (fixed) ---
                VideoDropZone {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 90
                    Layout.margins: 16
                    Layout.bottomMargin: 0
                    videoPath: {
                        if (!controller.videoPath) return ""
                        var parts = controller.videoPath.split("/")
                        return parts[parts.length - 1]
                    }
                    onFileSelected: function(path) { controller.loadVideo(path) }
                }

                FluDivider {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    Layout.topMargin: 12
                    Layout.bottomMargin: 0
                    size: 1
                }

                // --- Scrollable params area ---
                Flickable {
                    id: scrollArea
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 16
                    Layout.topMargin: 8
                    Layout.bottomMargin: 4
                    clip: true
                    contentWidth: width
                    contentHeight: paramsContent.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds

                    Column {
                        id: paramsContent
                        width: scrollArea.width
                        spacing: 12

                        // Type selector
                        RowLayout {
                            width: parent.width
                            FluText {
                                text: "生成类型"
                                font: FluTextStyle.Subtitle
                            }
                            Item { Layout.fillWidth: true }
                            FluRadioButton {
                                text: "条纹图"
                                checked: !controller.isIris
                                onClicked: controller.isIris = false
                            }
                            FluRadioButton {
                                text: "虹膜图"
                                checked: controller.isIris
                                onClicked: controller.isIris = true
                            }
                        }

                        FluDivider { size: 1; width: parent.width }

                        // Output size
                        RowLayout {
                            width: parent.width
                            FluText {
                                text: "输出尺寸"
                                font: FluTextStyle.Subtitle
                            }
                            Item { Layout.fillWidth: true }
                            FluButton {
                                text: "↺"
                                implicitWidth: 28; implicitHeight: 28
                                visible: controller.videoPath !== ""
                                onClicked: controller.resetToDefaults()
                            }
                        }
                        RowLayout {
                            width: parent.width; spacing: 8
                            FluSlider {
                                id: sizeSlider
                                Layout.fillWidth: true
                                from: 500; to: 4096; stepSize: 2
                                value: controller.outputSize
                                onMoved: controller.outputSize = value
                            }
                            FluSpinBox {
                                id: sizeBox
                                Layout.preferredWidth: 100
                                from: 500; to: 4096; stepSize: 2
                                editable: true
                                value: controller.outputSize
                                onValueChanged: {
                                    if (value % 2 !== 0) value -= 1
                                    controller.outputSize = value
                                }
                            }
                        }

                        FluDivider { size: 1; width: parent.width }

                        // Sample count
                        RowLayout {
                            width: parent.width
                            FluText {
                                text: "采样数量"
                                font: FluTextStyle.Subtitle
                            }
                            Item { Layout.fillWidth: true }
                            FluButton {
                                text: "↺"
                                implicitWidth: 28; implicitHeight: 28
                                visible: controller.videoPath !== ""
                                onClicked: controller.resetToDefaults()
                            }
                        }
                        RowLayout {
                            width: parent.width; spacing: 8
                            FluSlider {
                                id: sampleSlider
                                Layout.fillWidth: true
                                from: 100; to: 5000; stepSize: 10
                                value: controller.sampleCount
                                onMoved: controller.sampleCount = value
                            }
                            FluSpinBox {
                                id: sampleBox
                                Layout.preferredWidth: 100
                                from: 100; to: 5000; stepSize: 10
                                editable: true
                                value: controller.sampleCount
                                onValueChanged: controller.sampleCount = value
                            }
                        }

                        FluDivider { size: 1; width: parent.width }

                        // Metadata
                        FluText {
                            visible: controller.videoMeta
                                && controller.videoMeta["duration"] !== undefined
                            text: controller.videoMeta ? (
                                "时长: " + (controller.videoMeta["duration"] || "-")
                                + "  分辨率: " + (controller.videoMeta["resolution"] || "-")
                                + "\n编码: " + (controller.videoMeta["codec"] || "-")
                                + "  帧率: " + (controller.videoMeta["frameRate"] || "-")
                                + "  大小: " + (controller.videoMeta["fileSize"] || "-")
                            ) : ""
                            color: FluTheme.fontSecondaryColor
                            font: FluTextStyle.Caption
                            wrapMode: Text.WordWrap
                            width: parent.width
                        }
                        FluText {
                            visible: !controller.videoMeta
                                || controller.videoMeta["duration"] === undefined
                            text: "拖入视频后将显示元数据"
                            color: FluTheme.fontSecondaryColor
                            font: FluTextStyle.Caption
                        }

                        // Spacer to ensure scrollable area
                        Item { width: 1; height: 8 }
                    }
                }

                // --- Action buttons (fixed at bottom) ---
                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    Layout.topMargin: 8
                    spacing: 8

                    // Start (idle/finished)
                    FluFilledButton {
                        text: "开始生成"
                        Layout.fillWidth: true
                        enabled: controller.status === 0 || controller.status === 3
                        visible: controller.status !== 1 && controller.status !== 2
                        onClicked: controller.start()
                    }
                    // Pause
                    FluButton {
                        text: "暂停"
                        Layout.fillWidth: true
                        visible: controller.status === 1
                        onClicked: controller.pause()
                    }
                    // Resume
                    FluFilledButton {
                        text: "继续"
                        Layout.fillWidth: true
                        visible: controller.status === 2
                        onClicked: controller.resume()
                    }
                    // Cancel
                    FluButton {
                        text: "取消"
                        Layout.fillWidth: true
                        visible: controller.status === 1 || controller.status === 2
                        onClicked: controller.cancel()
                    }
                }
            }
        }

        // ============================================================
        // Right Panel: Result Preview
        // ============================================================
        ResultPreview {
            id: resultPreview
            Layout.fillWidth: true
            Layout.fillHeight: true
            resultImage: controller.status === 3 && controller.resultImagePath
                ? "file://" + controller.resultImagePath + "?t=" + Date.now()
                : ""
            processing: controller.status === 1 || controller.status === 2
            progress: controller.progress
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
