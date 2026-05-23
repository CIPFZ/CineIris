import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import FluentUI

Rectangle {
    id: root
    color: FluTheme.dark ? "#3A3A3C" : "#F9F9FB"
    radius: 14
    border.width: 2
    border.color: dropArea.containsDrag ? FluTheme.primaryColor : FluTheme.dark ? "#636366" : "#C7C7CC"

    property alias videoPath: pathText.text
    property string videoLabel: ""

    signal fileSelected(string path)

    DropArea {
        id: dropArea
        anchors.fill: parent

        onDropped: function(drop) {
            if (drop.urls.length > 0) {
                var path = drop.urls[0].toString()
                if (path.startsWith("file://"))
                    path = path.substring(7)
                root.fileSelected(path)
            }
        }

        Column {
            anchors.centerIn: parent
            spacing: 12

            FluText {
                anchors.horizontalCenter: parent.horizontalCenter
                text: dropArea.containsDrag ? "📂 释放以导入" : (videoPath ? "🎬" : "📁")
                font.pixelSize: 36
            }

            FluText {
                id: pathText
                anchors.horizontalCenter: parent.horizontalCenter
                text: videoPath || "拖拽视频到此处\n或点击下方按钮选择文件"
                color: videoPath ? FluTheme.fontPrimaryColor : FluTheme.fontSecondaryColor
                font: FluTextStyle.Body
                horizontalAlignment: Text.AlignHCenter
                lineHeight: 1.4
                elide: Text.ElideMiddle
                maximumLineCount: 2
            }

            FluButton {
                anchors.horizontalCenter: parent.horizontalCenter
                text: videoPath ? "更换视频" : "选择视频文件"
                onClicked: {
                    var dialog = fileDialog.createObject(root)
                    dialog.open()
                }
            }
        }
    }

    Component {
        id: fileDialog
        FileDialog {
            title: "选择视频文件"
            nameFilters: ["Video (*.mp4 *.mkv *.avi *.mov)"]
            onAccepted: {
                if (selectedFile) {
                    var path = selectedFile.toString()
                    if (path.startsWith("file://"))
                        path = path.substring(7)
                    root.fileSelected(path)
                }
                destroy()
            }
            onRejected: destroy()
        }
    }
}
