import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

ApplicationWindow {
    width: 1380
    height: 900
    visible: true
    title: "MyCells Dashboard"

    property double feedAmount: 0.5
    property double targetTemperature: appController.temperature
    property double targetPh: appController.ph
    property string targetName: appController.cellName

    Rectangle {
        anchors.fill: parent
        color: "#111827"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 16

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 72
                color: "#1f2937"
                radius: 16

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 14

                    Text {
                        text: "MyCells"
                        color: "white"
                        font.pixelSize: 26
                        font.bold: true
                    }

                    Rectangle {
                        width: 1
                        Layout.fillHeight: true
                        color: "#374151"
                    }

                    Text {
                        text: "Name: " + appController.cellName
                        color: "#d1d5db"
                        font.pixelSize: 16
                    }

                    Text {
                        text: "Status: " + appController.cellStatus
                        color: appController.alive ? "#86efac" : "#fca5a5"
                        font.pixelSize: 16
                    }

                    Text {
                        text: "Phase: " + appController.growthPhase
                        color: "#93c5fd"
                        font.pixelSize: 16
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: appController.running ? "Pause" : "Resume"
                        onClicked: appController.toggleRunning()
                    }

                    Button {
                        text: "Tick +1s"
                        onClicked: appController.tickOnce()
                    }

                    Button {
                        text: "Refresh"
                        onClicked: appController.refresh()
                    }

                    Button {
                        text: "Reset / Initialize"
                        highlighted: true
                        onClicked: resetDialog.open()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 2
                    spacing: 16

                    CellScene {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 360
                        mass: appController.mass
                        energy: appController.energy
                        health: appController.health
                        stress: appController.stress
                        divisionReadiness: appController.divisionReadiness
                        nutrientDensity: appController.nutrientDensity
                        feedBurstSerial: appController.feedBurstSerial
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 180
                        color: "#1f2937"
                        radius: 16

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 20
                            columns: 2
                            rowSpacing: 10
                            columnSpacing: 26

                            Text { text: "Mass"; color: "#e5e7eb"; font.pixelSize: 18 }
                            Text { text: Number(appController.mass).toFixed(6); color: "white"; font.pixelSize: 18 }

                            Text { text: "Energy"; color: "#e5e7eb"; font.pixelSize: 18 }
                            Text { text: Number(appController.energy).toFixed(6); color: "white"; font.pixelSize: 18 }

                            Text { text: "Health"; color: "#e5e7eb"; font.pixelSize: 18 }
                            Text { text: Number(appController.health).toFixed(6); color: "white"; font.pixelSize: 18 }

                            Text { text: "Stress"; color: "#e5e7eb"; font.pixelSize: 18 }
                            Text { text: Number(appController.stress).toFixed(6); color: "white"; font.pixelSize: 18 }

                            Text { text: "Division readiness"; color: "#e5e7eb"; font.pixelSize: 18 }
                            Text { text: Number(appController.divisionReadiness).toFixed(6); color: "white"; font.pixelSize: 18 }

                            Text { text: "Last simulated"; color: "#e5e7eb"; font.pixelSize: 18 }
                            Text { text: appController.lastSimulatedAt; color: "white"; font.pixelSize: 18 }
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        columns: 2
                        rowSpacing: 16
                        columnSpacing: 16

                        MiniLineChart {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            title: "Mass"
                            seriesData: appController.massSeries
                            lineColor: "#60a5fa"
                        }

                        MiniLineChart {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            title: "Energy"
                            seriesData: appController.energySeries
                            lineColor: "#34d399"
                        }

                        MiniLineChart {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            title: "Health"
                            seriesData: appController.healthSeries
                            lineColor: "#fbbf24"
                        }

                        MiniLineChart {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            title: "Stress"
                            seriesData: appController.stressSeries
                            lineColor: "#f87171"
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    spacing: 16

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 360
                        color: "#1f2937"
                        radius: 16

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 20
                            spacing: 12

                            Text {
                                text: "Actions"
                                color: "white"
                                font.pixelSize: 22
                                font.bold: true
                            }

                            Text {
                                text: "Feed amount: " + Number(feedAmount).toFixed(2)
                                color: "#e5e7eb"
                            }

                            Slider {
                                Layout.fillWidth: true
                                from: 0.1
                                to: 3.0
                                value: 0.5
                                onValueChanged: feedAmount = value
                            }

                            Button {
                                text: "Feed"
                                Layout.fillWidth: true
                                onClicked: appController.feed(feedAmount)
                            }

                            Text {
                                text: "Temperature: " + Number(targetTemperature).toFixed(2)
                                color: "#e5e7eb"
                            }

                            Slider {
                                Layout.fillWidth: true
                                from: 10.0
                                to: 50.0
                                value: appController.temperature
                                onValueChanged: targetTemperature = value
                            }

                            Button {
                                text: "Apply Temperature"
                                Layout.fillWidth: true
                                onClicked: appController.setTemperature(targetTemperature)
                            }

                            Text {
                                text: "pH: " + Number(targetPh).toFixed(2)
                                color: "#e5e7eb"
                            }

                            Slider {
                                Layout.fillWidth: true
                                from: 3.0
                                to: 10.0
                                value: appController.ph
                                onValueChanged: targetPh = value
                            }

                            Button {
                                text: "Apply pH"
                                Layout.fillWidth: true
                                onClicked: appController.setPh(targetPh)
                            }

                            Button {
                                text: "Reduce Toxin"
                                Layout.fillWidth: true
                                onClicked: appController.reduceToxin(0.1)
                            }

                            TextField {
                                Layout.fillWidth: true
                                placeholderText: "New cell name"
                                text: targetName
                                onTextChanged: targetName = text
                            }

                            Button {
                                text: "Rename Cell"
                                Layout.fillWidth: true
                                onClicked: appController.renameCell(targetName)
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1f2937"
                        radius: 16

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 20
                            spacing: 12

                            Text {
                                text: "Recent Events"
                                color: "white"
                                font.pixelSize: 20
                                font.bold: true
                            }

                            ListView {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 180
                                clip: true
                                spacing: 8
                                model: appController.recentEvents

                                delegate: Rectangle {
                                    width: ListView.view.width
                                    height: 82
                                    radius: 12
                                    color: "#111827"

                                    Column {
                                        anchors.fill: parent
                                        anchors.margins: 12
                                        spacing: 4

                                        Text {
                                            text: modelData.title + " (" + modelData.severity + ")"
                                            color: "white"
                                            font.pixelSize: 15
                                            font.bold: true
                                        }

                                        Text {
                                            text: modelData.description
                                            color: "#d1d5db"
                                            font.pixelSize: 13
                                            wrapMode: Text.Wrap
                                        }

                                        Text {
                                            text: modelData.time
                                            color: "#9ca3af"
                                            font.pixelSize: 12
                                        }
                                    }
                                }
                            }

                            Text {
                                text: "Recent User Actions"
                                color: "white"
                                font.pixelSize: 20
                                font.bold: true
                            }

                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                spacing: 8
                                model: appController.recentActions

                                delegate: Rectangle {
                                    width: ListView.view.width
                                    height: 74
                                    radius: 12
                                    color: "#111827"

                                    Column {
                                        anchors.fill: parent
                                        anchors.margins: 12
                                        spacing: 4

                                        Text {
                                            text: modelData.type
                                            color: "white"
                                            font.pixelSize: 15
                                            font.bold: true
                                        }

                                        Text {
                                            text: modelData.payload
                                            color: "#d1d5db"
                                            font.pixelSize: 12
                                            wrapMode: Text.WrapAnywhere
                                        }

                                        Text {
                                            text: modelData.time
                                            color: "#9ca3af"
                                            font.pixelSize: 11
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                color: appController.errorMessage.length > 0 ? "#7f1d1d" : "#1f2937"
                radius: 12

                Text {
                    anchors.centerIn: parent
                    text: appController.errorMessage.length > 0 ? appController.errorMessage : "Ready"
                    color: "white"
                    font.pixelSize: 14
                }
            }
        }
    }

    Dialog {
        id: resetDialog
        modal: true
        title: "Reset Simulation"
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: appController.resetSimulation()

        Label {
            text: "Reset the current cell and initialize the simulation again?"
            wrapMode: Text.Wrap
            width: 320
        }
    }
}
