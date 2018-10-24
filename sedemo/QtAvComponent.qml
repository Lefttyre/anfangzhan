import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import CambriconControl 1.0
import "."
Item {
    id: root
    width: 1082
    height: 922
    property bool playstate: false
    property bool handleMove: false
    property double handleMoveX: 0.0
    property point startPoint: Qt.point(0,0)
    property point offsetPoint: Qt.point(0,0)
    property double valuewidth: 0.0
    property int videofps: 0
    signal sendPlayMessage();

    Rectangle{
        id: rectMain
        anchors.fill: parent
        //color: "#19212e"
        color: "transparent"
        Image {
                id: image1
                fillMode: Image.Stretch
                anchors.fill: parent
                VideoCapture{
                    anchors.fill: parent
                    id : videoArea
                    objectName: "videoView"
                }
        }
    }
}
