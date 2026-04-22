import QtQuick

Rectangle {
    id: root

    property double mass: 1.0
    property double energy: 0.5
    property double health: 1.0
    property double stress: 0.0
    property double divisionReadiness: 0.0
    property double nutrientDensity: 1.0
    property int feedBurstSerial: 0

    radius: 20
    color: "#1f2937"

    property double cellSize: Math.max(120, Math.min(280, 120 + (mass - 1.0) * 70))
    property double wobbleAmplitude: 2 + stress * 10
    property double pulseScale: 1.0 + energy * 0.03

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: "#1f2937"
    }

    Item {
        id: stage
        anchors.fill: parent
        anchors.margins: 12

        Repeater {
            model: 28

            delegate: Rectangle {
                width: 6
                height: 6
                radius: 3
                color: "#2a3b52"
                opacity: 0.35

                readonly property real ax: (index * 37) % Math.max(20, stage.width)
                readonly property real ay: (index * 53) % Math.max(20, stage.height)

                x: ax
                y: ay
            }
        }

        Item {
            id: cellHolder
            width: root.cellSize
            height: root.cellSize
            anchors.centerIn: parent

            property real wobbleX: 0
            property real wobbleY: 0
            scale: root.pulseScale

            Behavior on width { NumberAnimation { duration: 600; easing.type: Easing.OutCubic } }
            Behavior on height { NumberAnimation { duration: 600; easing.type: Easing.OutCubic } }
            Behavior on scale { NumberAnimation { duration: 500; easing.type: Easing.OutQuad } }

            x: (stage.width - width) / 2 + wobbleX
            y: (stage.height - height) / 2 + wobbleY

            SequentialAnimation on wobbleX {
                loops: Animation.Infinite
                running: true
                NumberAnimation { to: root.wobbleAmplitude; duration: 650 }
                NumberAnimation { to: -root.wobbleAmplitude; duration: 650 }
                NumberAnimation { to: 0; duration: 650 }
            }

            SequentialAnimation on wobbleY {
                loops: Animation.Infinite
                running: true
                NumberAnimation { to: -root.wobbleAmplitude * 0.7; duration: 720 }
                NumberAnimation { to: root.wobbleAmplitude * 0.8; duration: 720 }
                NumberAnimation { to: 0; duration: 720 }
            }

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: Qt.rgba(
                    Math.max(0.15, 0.25 + root.stress * 0.55),
                    Math.max(0.35, 0.75 * root.health),
                    Math.max(0.2, 0.95 * root.health - root.stress * 0.3),
                    1.0
                )
                border.width: 4
                border.color: Qt.rgba(
                    0.8 + root.divisionReadiness * 0.2,
                    0.9 * root.health,
                    1.0 - root.stress * 0.25,
                    1.0
                )
            }

            Rectangle {
                width: parent.width * 0.55
                height: parent.height * 0.55
                radius: width / 2
                anchors.centerIn: parent
                color: Qt.rgba(1.0, 1.0 - root.stress * 0.5, 1.0, 0.18 + root.energy * 0.12)
                scale: 1.0

                SequentialAnimation on scale {
                    loops: Animation.Infinite
                    running: true
                    NumberAnimation { to: 1.06 + root.energy * 0.08; duration: 700 }
                    NumberAnimation { to: 0.96; duration: 700 }
                }
            }

            Rectangle {
                anchors.centerIn: parent
                width: parent.width * (0.94 + root.divisionReadiness * 0.08)
                height: width
                radius: width / 2
                color: "transparent"
                border.width: 3
                border.color: Qt.rgba(1.0, 0.9, 0.35, 0.15 + root.divisionReadiness * 0.75)
            }

            Repeater {
                model: 7

                delegate: Rectangle {
                    width: 10 + (index % 3) * 2
                    height: width
                    radius: width / 2
                    color: index % 2 === 0 ? "#a7f3d0" : "#bfdbfe"
                    opacity: 0.55

                    property real orbitRadius: cellHolder.width * (0.18 + index * 0.045)
                    property real angle: index * 52

                    x: cellHolder.width / 2 - width / 2 + Math.cos(angle * Math.PI / 180) * orbitRadius
                    y: cellHolder.height / 2 - height / 2 + Math.sin(angle * Math.PI / 180) * orbitRadius

                    SequentialAnimation on angle {
                        loops: Animation.Infinite
                        running: true
                        NumberAnimation { to: angle + 180; duration: 2800 + index * 250 }
                        NumberAnimation { to: angle + 360; duration: 2800 + index * 250 }
                    }
                }
            }
        }

        Repeater {
            model: 14

            delegate: Rectangle {
                id: foodParticle
                width: 8
                height: 8
                radius: 4
                color: "#86efac"
                opacity: 0.0

                property real angleDeg: (index * 27 + root.feedBurstSerial * 11) % 360
                property real startRadius: Math.min(stage.width, stage.height) * (0.38 + (index % 5) * 0.05)

                function resetAndAnimate() {
                    var rad = angleDeg * Math.PI / 180.0
                    var cx = stage.width / 2
                    var cy = stage.height / 2

                    x = cx + Math.cos(rad) * startRadius - width / 2
                    y = cy + Math.sin(rad) * startRadius - height / 2
                    opacity = 0.15 + Math.min(0.8, root.nutrientDensity * 0.25)

                    moveAnimX.to = cx - width / 2
                    moveAnimY.to = cy - height / 2
                    fadeAnim.to = 0.0

                    moveAnimX.restart()
                    moveAnimY.restart()
                    fadeAnim.restart()
                }

                NumberAnimation {
                    id: moveAnimX
                    target: foodParticle
                    property: "x"
                    duration: 850 + (index % 5) * 120
                    easing.type: Easing.InOutQuad
                }

                NumberAnimation {
                    id: moveAnimY
                    target: foodParticle
                    property: "y"
                    duration: 850 + (index % 5) * 120
                    easing.type: Easing.InOutQuad
                }

                NumberAnimation {
                    id: fadeAnim
                    target: foodParticle
                    property: "opacity"
                    duration: 900 + (index % 5) * 120
                    easing.type: Easing.InQuad
                }

                Component.onCompleted: resetAndAnimate()

                Connections {
                    target: root
                    function onFeedBurstSerialChanged() {
                        foodParticle.resetAndAnimate()
                    }
                }
            }
        }
    }

    Text {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 16
        anchors.topMargin: 12
        text: "Live Cell View"
        color: "white"
        font.pixelSize: 18
        font.bold: true
    }

    Text {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 16
        anchors.topMargin: 12
        text: "mass " + Number(root.mass).toFixed(3)
        color: "#d1d5db"
        font.pixelSize: 14
    }
}