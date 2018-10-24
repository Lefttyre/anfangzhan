import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2
import "."
import CambriconControl 1.0
ApplicationWindow {
    id:mainwindiow
    visible: true
    width: 1920
    height: 1080
    color: "#080403"
    title: qsTr("CMABRICON")
    flags: Qt.FramelessWindowHint|Qt.Window|Qt.WindowSystemMenuHint|Qt.WindowMinimizeButtonHint

    property point startPoint1: Qt.point(0,0)
    property point offsetPoint1: Qt.point(0,0)
    property bool isMoveWindow1: false
    property bool isScale1: false
    property int enableSize1: 4
    property bool maxornormal1: true

    Rectangle{
        id: rectTitleBar
        anchors.top: parent.top
        anchors.left: parent.left
        width: parent.width
        height: 67
        color: "#19212f"
        z: 2

        /*界面拖动*/
        MouseArea{
            id: mouseMoveWindowArea
            anchors.fill: parent

            /*鼠标点击后初始化开始移动的起点,移动窗体isMoveWindow置为true*/
            onPressed: {
                cursorShape = Qt.SizeAllCursor
                startPoint1 = Qt.point(mouseX,mouseY)
                isMoveWindow1 = !isMoveWindow1
            }

            /*鼠标按住移动计算位置偏移量,窗体随之移动*/
            onPositionChanged: {
                offsetPoint1 = Qt.point(mouseX-startPoint1.x,mouseY-startPoint1.y)
                if(isMoveWindow1){
                    mainwindiow.x += offsetPoint1.x
                    mainwindiow.y += offsetPoint1.y
                }
            }

            /*释放时isMoveWindow置为false*/
            onReleased: {
                cursorShape = Qt.ArrowCursor
                isMoveWindow1=!isMoveWindow1
            }
        }
       Text {
            id: txtMainTitle
            y: 26
            color: "#e6e5e5"
            text: qsTr("TESLA P4")
            anchors.left: parent.left
            anchors.leftMargin: 40
            font.bold: true
            font.weight: Font.Bold
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 30
        }
       Text {
           id: txtModels
           text: qsTr("Resnet34-SSD")
           anchors.right: txtFps.left
           anchors.rightMargin: 50
           anchors.verticalCenter: parent.verticalCenter
           font.bold: true
           font.weight:Font.Bold
           color: "white"
       }
       Text {
           id: txtFps
           text: qsTr("500FPS")
           anchors.right: sysBtnGroup.left
           anchors.rightMargin: 50
           anchors.verticalCenter: parent.verticalCenter
           font.bold: true
           font.weight:Font.Bold
           color: "white"
           Connections {
               target: fps_updater
               onSig_refresh_fps: {
                   txtFps.text = "TOTAL FPS: " + fps;
               }
           }
       }
       SysBtnGroup {
            id: sysBtnGroup
            x: 1163
            y: 25


            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter


            onMin: mainwindiow.showMinimized()
            onClose: {
                Qt.quit();
                mainwindiow.close()
            }
            onMax: {
                mainwindiow.maxornormal1?mainwindiow.showMaximized():mainwindiow.showNormal()
                mainwindiow.maxornormal1 = !mainwindiow.maxornormal1
            }
        }
    }
    Rectangle {
        id: rectSeparator
        width: parent.width
        height: 3
        color: "#080404"
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: rectTitleBar.bottom
        anchors.topMargin: 0
    }
    Rectangle{
        id:rectContent
        anchors.top: rectSeparator.bottom
        anchors.left:parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
         color: "#060205"
         Rectangle{
             id:rectChns
             width: 896
             height: parent
             color: "#19212e"
             border.width: 0
             anchors.left: parent.left
             anchors.leftMargin: 0
             anchors.bottom: parent.bottom
             anchors.bottomMargin: 0
             anchors.top: parent.top
             anchors.topMargin: 0
             Rectangle{
                 id:grdChns
                 visible: true
                 anchors.top: parent.top
                 anchors.left: parent.left
                 anchors.right: parent.right
                 height: parent.height
                 width: parent.width
                 VideoCapture{
                     anchors.fill:parent
                     id : videoArea
                     objectName: "videoView0"
                     MouseArea{
                         anchors.fill: parent
                         onClicked: {
                             videoArea.selectChns(mouseX,mouseY);
                         }
                     }
                 }
             }
         }
         Rectangle{
             id:rectContentRight
             anchors.left: rectChns.right
             anchors.top: parent.top
             anchors.bottom:parent.bottom
             anchors.right: parent.right
             color:"transparent"
             Rectangle{
                 id:rectQtAV
                 anchors.left: parent.left
                 anchors.right: parent.right
                 anchors.top: parent.top
                 anchors.bottom: parent.bottom
                 anchors.bottomMargin:299
                 color: "transparent"
                 anchors.rightMargin: 0
                 Image {
                      id: imgDetail
                      width: parent.width
                      height: parent.height
                      cache: false
                      clip: true
                      source : "images/car.jpg";
                      fillMode : Image.Stretch
                 }
                 Timer{
                     //定时器触发时间 单位毫秒
                     interval: 30;
                     //触发定时器
                     running: true;
                     //不断重复
                     repeat: true;
                     //定时器触发时执行
                     onTriggered: {
                         imgDetail.source = "";
                         imgDetail.source = "image://detailimg/detail";
                     }
                 }
             }
             Rectangle{
                 id:rectSelectedDetail
                 color:"transparent"
                 anchors.left: parent.left
                 anchors.right: parent.right
                 anchors.top:rectQtAV.bottom
                 anchors.bottom: parent.bottom
                height: 300
                 Timer{
                     //定时器触发时间 单位毫秒
                     interval: 900;
                     //触发定时器
                     running: true;
                     //不断重
                     repeat: true;
                     //定时器触发时执行
                     onTriggered: {
                         image4.source = "";
                         image4.source = "image://detailimg/1";
                         image5.source = "";
                         image5.source = "image://detailimg/3";
                         image6.source = "";
                         image6.source = "image://detailimg/5";
                         image7.source = "";
                         image7.source = "image://detailimg/7";
                     }
                 }

                 Timer{
                     //定时器触发时间 单位毫秒
                     interval: 800;
                     //触发定时器
                     running: true;
                     //不断重
                     repeat: true;
                     //定时器触发时执行
                     onTriggered: {
                         image0.source = "";
                         image0.source = "image://detailimg/0";
                         image1.source = "";
                         image1.source = "image://detailimg/2";
                         image2.source = "";
                         image2.source = "image://detailimg/4";
                         image3.source = "";
                         image3.source = "image://detailimg/6";
                     }
                 }

                 Image {
                     id: image0
                     source: "images/car.jpg"
                     fillMode: Image.PreserveAspectFit
                     clip: true
                     x: 0
                     y: 0
                     width: 256
                     height: 150
                     cache: false
                 }

                 Image {
                     id: image1
                     x: 256
                     y: 0
                     width: 256
                     height: 150
                     source: "images/car.jpg"
                     cache: false
                     fillMode: Image.PreserveAspectFit
                 }

                 Image {
                     id: image2
                     x: 512
                     y: 0
                     width: 256
                     height: 150
                     source: "images/car.jpg"
                     cache: false
                     fillMode: Image.PreserveAspectFit
                 }

                 Image {
                     id: image3
                     x: 768
                     y: 0
                     width: 256
                     height: 150
                     source: "images/car.jpg"
                     cache: false
                     fillMode: Image.PreserveAspectFit

                 }

                 Image {
                     id: image4
                     x: 0
                     y: 150
                     width: 256
                     height: 150
                     source: "images/car.jpg"
                     cache: false
                     fillMode: Image.PreserveAspectFit
                 }

                 Image {
                     id: image5
                     x: 256
                     y: 150
                     width: 256
                     height: 150
                     source: "images/car.jpg"
                     cache: false
                     fillMode: Image.PreserveAspectFit
                 }

                 Image {
                     id: image6
                     x: 512
                     y: 150
                     width: 256
                     height: 150
                     source: "images/car.jpg"
                     cache: false
                     fillMode: Image.PreserveAspectFit
                 }

                 Image {
                     id: image7
                     x: 768
                     y: 150
                     width: 256
                     height: 150
                     source: "images/car.jpg"
                     cache: false
                     fillMode: Image.PreserveAspectFit
                 }
                 }
             }

         }

}
