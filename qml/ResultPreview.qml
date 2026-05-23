import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FluentUI

Rectangle {
    id: root
    color: FluTheme.dark ? "#2C2C2E" : "#FFFFFF"
    radius: 14

    property var resultImage: null
    signal saveRequested()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Item { Layout.fillHeight: true }

        // Preview area
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: FluTheme.dark ? "#1C1C1E" : "#F5F5F7"
            radius: 10

            // Placeholder
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 8
                visible: !root.resultImage || root.resultImage.width === 0

                FluText {
                    Layout.alignment: Qt.AlignHCenter
                    text: "🎨"
                    font.pixelSize: 48
                }
                FluText {
                    Layout.alignment: Qt.AlignHCenter
                    text: "生成结果将显示在此处"
                    color: FluTheme.fontSecondaryColor
                    font: FluTextStyle.Body
                }
            }

            // Result image with zoom
            Flickable {
                id: flick
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                contentWidth: imageContainer.width
                contentHeight: imageContainer.height
                visible: root.resultImage && root.resultImage.width > 0

                Item {
                    id: imageContainer
                    width: Math.max(flick.width, resultImg.implicitWidth * flick.scaleFactor)
                    height: Math.max(flick.height, resultImg.implicitHeight * flick.scaleFactor)

                    Image {
                        id: resultImg
                        anchors.centerIn: parent
                        fillMode: Image.PreserveAspectFit
                        width: Math.min(implicitWidth, flick.width)
                        height: Math.min(implicitHeight, flick.height)
                        cache: false
                        source: root.resultImage ? root.resultImage : ""
                    }
                }

                property real scaleFactor: 1.0

                MouseArea {
                    anchors.fill: parent
                    onWheel: function(wheel) {
                        var delta = wheel.angleDelta.y / 120
                        flick.scaleFactor = Math.max(0.5, Math.min(3.0, flick.scaleFactor + delta * 0.2))
                        wheel.accepted = true
                    }
                }
            }
        }

        // Save button
        FluFilledButton {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            visible: root.resultImage && root.resultImage.width > 0
            text: "保存图片"
            onClicked: root.saveRequested()
        }

        Item { Layout.fillHeight: true }
    }
}
