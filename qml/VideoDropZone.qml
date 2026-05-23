import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import FluentUI

Rectangle {
    id: root
    color: FluTheme.dark ? "#3A3A3C" : "#F9F9FB"
    radius: 10
    border.width: 1
    border.color: FluTheme.dark ? "#636366" : "#C7C7CC"

    property alias videoPath: pathText.text
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

        RowLayout {
            anchors.centerIn: parent
            spacing: 12

            FluText {
                id: pathText
                text: ""
                color: FluTheme.fontPrimaryColor
                font: FluTextStyle.Body
                elide: Text.ElideMiddle
                Layout.maximumWidth: 200
            }

            FluButton {
                text: videoPath ? "更换" : "选择视频"
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
