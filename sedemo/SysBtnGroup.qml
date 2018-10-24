import QtQuick 2.0

Row
{
    id: sysbtngroup
    spacing: 0

    signal max
    signal min
    signal close

    SysBtn
    {
        id:min
        picHover: "images/min_hover.png"
        picNormal: "images/min_normal.png"
        picPressed: "images/min_down.png"
        onClicked:
        {
            // //console.log("min btn clicked")
            sysbtngroup.min()
        }
    }

    SysBtn
    {
        id:close
        picHover: "images/close_hover.png"
        picNormal: "images/close_normal.png"
        picPressed: "images/close_down.png"
        onClicked:
        {
            // //console.log("close btn clicked")
            sysbtngroup.close()
        }
    }
}
