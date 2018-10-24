import QtQuick 2.0

Row
{
    id: sysbtngroup
    spacing: 5

    signal hover
    signal max
    signal min
    signal close

//    SysButon
//    {
//        id:hover
//        picHover: "qrc:/素材/u91.png"
//        picNormal: "qrc:/素材/u91.png"
//        picPressed: "qrc:/素材/u91.png"
//        onClicked:
//        {
//            sysbtngroup.hover()
//        }
//    }

    SysButon
    {
        id:menu
        picHover: "qrc:/素材/0911-单兵2.1_16.png"
        picNormal: "qrc:/素材/0911-单兵2.1_16.png"
        picPressed: "qrc:/素材/0911-单兵2.1_16.png"
        onClicked:
        {
            sysbtngroup.min()
        }
    }

    SysButon
    {
        id:min
        picHover: "qrc:/素材/0911-单兵2.1_18.png"
        picNormal: "qrc:/素材/0911-单兵2.1_18.png"
        picPressed: "qrc:/素材/0911-单兵2.1_18.png"
        onClicked:
        {
            sysbtngroup.max()
        }
    }

    SysButon
    {
        id:close
        picHover: "qrc:/素材/0911-单兵2.1_20.png"
        picNormal: "qrc:/素材/0911-单兵2.1_20.png"
        picPressed: "qrc:/素材/0911-单兵2.1_20.png"
        onClicked:
        {
            sysbtngroup.close()
        }
    }
}
