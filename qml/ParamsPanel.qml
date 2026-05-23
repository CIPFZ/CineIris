import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import FluentUI

Rectangle {
    id: root
    color: FluTheme.dark ? "#2C2C2E" : "#FFFFFF"

    property bool isIris: false
    property int outputSize: 1080
    property int sampleCount: 300
    property int status: 0 // 0=Idle, 1=Processing, 2=Paused, 3=Finished

    signal typeChanged(bool v)
    signal sizeChanged(int v)
    signal countChanged(int v)
    signal startClicked()
    signal pauseClicked()
    signal resumeClicked()
    signal cancelClicked()
    signal saveClicked()

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        // Type selector
        FluText {
            text: "生成类型"
            font: FluTextStyle.Subtitle
        }

        RowLayout {
            // Barcode toggle
            FluButton {
                text: "条纹图"
                Layout.fillWidth: true
                opacity: !root.isIris ? 1.0 : 0.5
                onClicked: root.typeChanged(false)
            }
            // Iris toggle
            FluButton {
                text: "虹膜图"
                Layout.fillWidth: true
                opacity: root.isIris ? 1.0 : 0.5
                onClicked: root.typeChanged(true)
            }
        }

        FluDivider { size: 1 }

        // Output size slider
        FluText {
            text: "输出尺寸: " + root.outputSize + "px"
            font: FluTextStyle.Subtitle
        }

        FluSlider {
            Layout.fillWidth: true
            from: 500
            to: 4096
            value: root.outputSize
            stepSize: 2
            onMoved: root.sizeChanged(value)
        }

        FluDivider { size: 1 }

        // Sample count slider
        FluText {
            text: "采样数量: " + root.sampleCount
            font: FluTextStyle.Subtitle
        }

        FluSlider {
            Layout.fillWidth: true
            from: 100
            to: 5000
            value: root.sampleCount
            stepSize: 10
            onMoved: root.countChanged(value)
        }

        FluDivider { size: 1 }

        // Metadata info (placeholder)
        FluText {
            text: "拖入视频后将显示元数据"
            color: FluTheme.fontSecondaryColor
            font: FluTextStyle.Caption
        }

        // Action buttons
        Item { Layout.fillHeight: true }

        RowLayout {
            spacing: 8

            // Start button
            FluFilledButton {
                text: "开始生成"
                Layout.fillWidth: true
                enabled: root.status === 0 || root.status === 3
                visible: root.status !== 1 && root.status !== 2
                onClicked: root.startClicked()
            }

            // Pause button
            FluButton {
                text: "暂停"
                Layout.fillWidth: true
                visible: root.status === 1
                onClicked: root.pauseClicked()
            }

            // Resume button
            FluFilledButton {
                text: "继续"
                Layout.fillWidth: true
                visible: root.status === 2
                onClicked: root.resumeClicked()
            }

            // Cancel button
            FluButton {
                text: "取消"
                Layout.fillWidth: true
                visible: root.status === 1 || root.status === 2
                onClicked: root.cancelClicked()
            }

            // Cancel
            FluButton {
                text: "取消并重新开始"
                Layout.fillWidth: true
                visible: root.status === 1 || root.status === 2
                onClicked: root.cancelClicked()
            }
        }

        // Save button
        FluFilledButton {
            Layout.fillWidth: true
            text: "保存结果"
            visible: root.status === 3
            enabled: root.status === 3
            onClicked: root.saveClicked()
        }
    }
}
