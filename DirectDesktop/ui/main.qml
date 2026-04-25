import QtQuick
import QtQuick.Controls

Window {
    id: root
    visible: true
    width: Screen.width
    height: Screen.height
    flags: Qt.FramelessWindowHint | Qt.WindowTransparentForInput
    color: "transparent"

    Rectangle {
        id: desktopOverlay
        anchors.fill: parent
        color: "transparent"

        // Animated pulsing indicator to confirm it's running
        Rectangle {
            width: 300
            height: 300
            radius: 150
            color: "#400078D7"
            anchors.centerIn: parent
            
            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { from: 0.2; to: 0.8; duration: 2000; easing.type: Easing.InOutQuad }
                NumberAnimation { from: 0.8; to: 0.2; duration: 2000; easing.type: Easing.InOutQuad }
            }

            Text {
                anchors.centerIn: parent
                text: "DirectDesktop\nQt6 Active"
                color: "white"
                font.pixelSize: 24
                horizontalAlignment: Text.AlignHCenter
                font.weight: Font.DemiBold
            }
        }
        
        // Modern Sidebar
        Rectangle {
            width: 80
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            color: "#CC1E1E1E"
            
            Column {
                anchors.centerIn: parent
                spacing: 30
                
                Repeater {
                    model: ["🏠", "🔍", "⚙️", "🌙"]
                    delegate: Text {
                        text: modelData
                        font.pixelSize: 32
                        color: "white"
                        scale: 1.0
                        
                        Behavior on scale { NumberAnimation { duration: 100 } }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent.scale = 1.2
                            onExited: parent.scale = 1.0
                        }
                    }
                }
            }
        }
    }
}
