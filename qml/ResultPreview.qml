import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FluentUI

Rectangle {
    id: root
    color: FluTheme.dark ? "#2C2C2E" : "#FFFFFF"
    radius: 14

    property string resultImage: ""
    property bool processing: false
    property int progress: 0
    signal saveRequested()

    // Processing state: show progress ring
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 16
        visible: root.processing && (!root.resultImage || root.resultImage.length === 0)

        FluProgressRing {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 72
            implicitHeight: 72
            indeterminate: false
            progressVisible: true
            from: 0.0
            to: 100.0
            value: root.progress
        }
        FluText {
            Layout.alignment: Qt.AlignHCenter
            text: "正在生成..."
            color: FluTheme.fontSecondaryColor
            font: FluTextStyle.Body
        }
    }

    // Result image
    Flickable {
        id: flick
        anchors.fill: parent
        anchors.margins: 8
        clip: true
        contentWidth: Math.max(resultImg.paintedWidth, flick.width)
        contentHeight: Math.max(resultImg.paintedHeight, flick.height)
        visible: root.resultImage && root.resultImage.length > 0

        Image {
            id: resultImg
            anchors.centerIn: parent
            fillMode: Image.PreserveAspectFit
            cache: false
            source: root.resultImage || ""
            width: Math.min(implicitWidth, flick.width)
            height: Math.min(implicitHeight, flick.height)
        }
    }

    // Placeholder (idle, no video loaded)
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 8
        visible: !root.processing && (!root.resultImage || root.resultImage.length === 0)

        FluText {
            Layout.alignment: Qt.AlignHCenter
            text: "预览区"
            color: FluTheme.fontSecondaryColor
            font: FluTextStyle.Subtitle
        }
        FluText {
            Layout.alignment: Qt.AlignHCenter
            text: "生成结果将显示在此处"
            color: FluTheme.fontSecondaryColor
            font: FluTextStyle.Body
        }
    }

    // Download button (top-right corner)
    Rectangle {
        id: downloadBtn
        anchors { top: parent.top; right: parent.right }
        anchors.margins: 10
        width: 36; height: 36
        radius: 8
        visible: root.resultImage && root.resultImage.length > 0
        color: btnMouse.containsMouse
            ? (FluTheme.dark ? "#48484C" : "#E5E5EA")
            : (FluTheme.dark ? "#3A3A3C" : "#F2F2F7")

        Canvas {
            id: downloadBtnIcon
            anchors.centerIn: parent
            width: 16; height: 16
            onPaint: {
                var ctx = getContext("2d")
                var c = FluTheme.dark ? "#FFFFFF" : "#1C1C1E"
                ctx.strokeStyle = c
                ctx.lineWidth = 2
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                // Arrow shaft
                ctx.beginPath()
                ctx.moveTo(8, 2)
                ctx.lineTo(8, 8)
                ctx.stroke()
                // Arrow head (chevron down)
                ctx.beginPath()
                ctx.moveTo(3, 5)
                ctx.lineTo(8, 10)
                ctx.lineTo(13, 5)
                ctx.stroke()
                // Tray
                ctx.beginPath()
                ctx.moveTo(2, 13)
                ctx.lineTo(14, 13)
                ctx.stroke()
            }
            Connections {
                target: FluTheme
                function onDarkChanged() { downloadBtnIcon.requestPaint() }
            }
        }

        MouseArea {
            id: btnMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.saveRequested()
        }
    }
}
