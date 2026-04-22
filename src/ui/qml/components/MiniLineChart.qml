import QtQuick

Rectangle {
    id: root
    property string title: ""
    property var seriesData: []
    property color lineColor: "#60a5fa"
    property string valueSuffix: ""

    radius: 16
    color: "#1f2937"

    function latestValueText() {
        if (!seriesData || seriesData.length === 0)
            return "-"
        var last = seriesData[seriesData.length - 1]
        return Number(last.y).toFixed(4) + valueSuffix
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        anchors.margins: 12

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()

            var w = width
            var h = height

            ctx.fillStyle = "#1f2937"
            ctx.fillRect(0, 0, w, h)

            ctx.strokeStyle = "#374151"
            ctx.lineWidth = 1
            ctx.beginPath()
            ctx.moveTo(0, h - 1)
            ctx.lineTo(w, h - 1)
            ctx.stroke()

            if (!root.seriesData || root.seriesData.length < 2)
                return

            var minX = root.seriesData[0].x
            var maxX = root.seriesData[root.seriesData.length - 1].x
            var minY = root.seriesData[0].y
            var maxY = root.seriesData[0].y

            for (var i = 1; i < root.seriesData.length; i++) {
                minY = Math.min(minY, root.seriesData[i].y)
                maxY = Math.max(maxY, root.seriesData[i].y)
            }

            if (Math.abs(maxY - minY) < 1e-9) {
                maxY += 1.0
                minY -= 1.0
            }

            if (Math.abs(maxX - minX) < 1e-9) {
                maxX += 1.0
            }

            function mapX(v) {
                return ((v - minX) / (maxX - minX)) * w
            }

            function mapY(v) {
                return h - ((v - minY) / (maxY - minY)) * (h - 8) - 4
            }

            ctx.strokeStyle = root.lineColor
            ctx.lineWidth = 2.5
            ctx.beginPath()
            ctx.moveTo(mapX(root.seriesData[0].x), mapY(root.seriesData[0].y))
            for (var j = 1; j < root.seriesData.length; j++) {
                ctx.lineTo(mapX(root.seriesData[j].x), mapY(root.seriesData[j].y))
            }
            ctx.stroke()

            var last = root.seriesData[root.seriesData.length - 1]
            ctx.fillStyle = root.lineColor
            ctx.beginPath()
            ctx.arc(mapX(last.x), mapY(last.y), 3.5, 0, Math.PI * 2)
            ctx.fill()
        }

        Connections {
            target: root
            function onSeriesDataChanged() { canvas.requestPaint() }
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    Text {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 14
        anchors.topMargin: 10
        text: root.title
        color: "white"
        font.pixelSize: 16
        font.bold: true
    }

    Text {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 14
        anchors.topMargin: 10
        text: root.latestValueText()
        color: root.lineColor
        font.pixelSize: 14
        font.bold: true
    }
}