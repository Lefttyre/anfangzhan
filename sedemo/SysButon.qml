import QtQuick 2.0

Rectangle
{
    id:sysbtn

    property string picCurrent: ""
    property string picNormal: ""
    property string picHover: ""
    property string picPressed: ""

    signal clicked

    width: 27
    height: 22
    color:"#00000000"
    state:"normal"

    Image
    {
        fillMode: Image.PreserveAspectFit
        source: picCurrent
        width: 15
        height: 15
    }

    MouseArea
    {
        anchors.fill: parent
        onEntered: sysbtn.state == "pressed" ? sysbtn.state = "pressed" : sysbtn.state = "hover"
        onExited: sysbtn.state == "pressed" ? sysbtn.state = "pressed" : sysbtn.state = "normal"
        onPressed: sysbtn.state = "pressed"
        onReleased:
        {
            sysbtn.state = "normal"
            sysbtn.clicked()
        }
    }

    states:
    [
        State{
            name:"hover"
            PropertyChanges {
                target: sysbtn
                picCurrent:picHover
            }
        },
        State {
            name: "normal"
            PropertyChanges {
                target: sysbtn
                picCurrent:picNormal
            }
        },
        State {
            name: "pressed"
            PropertyChanges {
                target: sysbtn
                picCurrent:picPressed
            }
        }

    ]
}
