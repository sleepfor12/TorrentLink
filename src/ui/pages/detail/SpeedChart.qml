import QtQuick 2.15

Item {
    id: root
    property var periodOptions: ["1min", "5min", "30min", "1h", "3h", "6h", "12h", "24h"]
    property int periodIndex: 1

    Rectangle {
        anchors.fill: parent
        color: "#ffffff"

        Rectangle {
            id: topBar
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 54
            color: "#f8fafc"
            border.color: "#e2e8f0"
            border.width: 1

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: "速度趋势（总上传/总下载）"
                font.pixelSize: 13
                font.bold: true
                color: "#0f172a"
            }

            Text {
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: chartModelRef ? ("采样: " + chartModelRef.sampleCount + " / 4096") : "采样: 0 / 4096"
                font.pixelSize: 11
                color: "#64748b"
            }
        }

        Row {
            id: metricRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: topBar.bottom
            anchors.margins: 8
            spacing: 8

            Rectangle {
                width: (metricRow.width - 16) / 3
                height: 42
                radius: 6
                color: "#eff6ff"
                border.color: "#bfdbfe"
                Text { anchors.left: parent.left; anchors.leftMargin: 10; anchors.top: parent.top; anchors.topMargin: 6; text: "下载"; font.pixelSize: 10; color: "#0369a1" }
                Text { anchors.left: parent.left; anchors.leftMargin: 10; anchors.bottom: parent.bottom; anchors.bottomMargin: 7; text: chartModelRef ? chartModelRef.dlCurrent : "0 B/s"; font.pixelSize: 12; color: "#0f172a"; font.bold: true }
            }
            Rectangle {
                width: (metricRow.width - 16) / 3
                height: 42
                radius: 6
                color: "#fff7ed"
                border.color: "#fed7aa"
                Text { anchors.left: parent.left; anchors.leftMargin: 10; anchors.top: parent.top; anchors.topMargin: 6; text: "上传"; font.pixelSize: 10; color: "#b45309" }
                Text { anchors.left: parent.left; anchors.leftMargin: 10; anchors.bottom: parent.bottom; anchors.bottomMargin: 7; text: chartModelRef ? chartModelRef.ulCurrent : "0 B/s"; font.pixelSize: 12; color: "#0f172a"; font.bold: true }
            }
            Rectangle {
                width: (metricRow.width - 16) / 3
                height: 42
                radius: 6
                color: "#f5f3ff"
                border.color: "#ddd6fe"
                Text { anchors.left: parent.left; anchors.leftMargin: 10; anchors.top: parent.top; anchors.topMargin: 6; text: "峰值刻度"; font.pixelSize: 10; color: "#6d28d9" }
                Text { anchors.left: parent.left; anchors.leftMargin: 10; anchors.bottom: parent.bottom; anchors.bottomMargin: 7; text: chartModelRef ? chartModelRef.formatRate(chartModelRef.maxRate) : "0 B/s"; font.pixelSize: 12; color: "#0f172a"; font.bold: true }
            }
        }

        Row {
            id: controlsRow
            anchors.right: parent.right
            anchors.top: metricRow.bottom
            anchors.rightMargin: 8
            anchors.topMargin: 4
            spacing: 8

            Rectangle {
                width: 96
                height: 24
                radius: 4
                color: "#ffffff"
                border.color: "#cbd5e1"
                Text {
                    anchors.centerIn: parent
                    text: "周期: " + root.periodOptions[root.periodIndex]
                    color: "#334155"
                    font.pixelSize: 11
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.periodIndex = (root.periodIndex + 1) % root.periodOptions.length;
                        canvas.requestPaint();
                    }
                }
            }
        }

        Item {
            id: seriesChecksPanel
            anchors.right: parent.right
            anchors.top: controlsRow.bottom
            anchors.rightMargin: 8
            anchors.topMargin: 4
            width: 132
            height: 10 * 20 + 9 * 2 + 8
            z: 5

            Rectangle {
                anchors.fill: parent
                color: "#ffffffee"
                border.color: "#e2e8f0"
                radius: 6
            }
            Column {
                id: seriesChecks
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 4
                spacing: 2

            Item {
                id: cTotalDl
                property bool checked: true
                width: 128; height: 20
                Row { anchors.fill: parent; spacing: 6
                    Rectangle { width: 14; height: 14; radius: 2; color: cTotalDl.checked ? "#38bdf8" : "#ffffff"; border.color: "#94a3b8"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "总下载"; color: "#334155"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
                }
                MouseArea { anchors.fill: parent; onClicked: { cTotalDl.checked = !cTotalDl.checked; canvas.requestPaint(); } }
            }
            Item {
                id: cTotalUl
                property bool checked: true
                width: 128; height: 20
                Row { anchors.fill: parent; spacing: 6
                    Rectangle { width: 14; height: 14; radius: 2; color: cTotalUl.checked ? "#fb923c" : "#ffffff"; border.color: "#94a3b8"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "总上传"; color: "#334155"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
                }
                MouseArea { anchors.fill: parent; onClicked: { cTotalUl.checked = !cTotalUl.checked; canvas.requestPaint(); } }
            }
            Item {
                id: cPayloadDl
                property bool checked: true
                width: 128; height: 20
                Row { anchors.fill: parent; spacing: 6
                    Rectangle { width: 14; height: 14; radius: 2; color: cPayloadDl.checked ? "#38bdf8" : "#ffffff"; border.color: "#94a3b8"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "有效负荷下载"; color: "#334155"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
                }
                MouseArea { anchors.fill: parent; onClicked: { cPayloadDl.checked = !cPayloadDl.checked; canvas.requestPaint(); } }
            }
            Item {
                id: cPayloadUl
                property bool checked: true
                width: 128; height: 20
                Row { anchors.fill: parent; spacing: 6
                    Rectangle { width: 14; height: 14; radius: 2; color: cPayloadUl.checked ? "#fb923c" : "#ffffff"; border.color: "#94a3b8"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "有效负荷上传"; color: "#334155"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
                }
                MouseArea { anchors.fill: parent; onClicked: { cPayloadUl.checked = !cPayloadUl.checked; canvas.requestPaint(); } }
            }
            Item { id: cOverDl; property bool checked: false; width: 128; height: 20
                Row { anchors.fill: parent; spacing: 6
                    Rectangle { width: 14; height: 14; radius: 2; color: cOverDl.checked ? "#38bdf8" : "#ffffff"; border.color: "#94a3b8"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "下载开销"; color: "#334155"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter } }
                MouseArea { anchors.fill: parent; onClicked: { cOverDl.checked = !cOverDl.checked; canvas.requestPaint(); } } }
            Item { id: cOverUl; property bool checked: false; width: 128; height: 20
                Row { anchors.fill: parent; spacing: 6
                    Rectangle { width: 14; height: 14; radius: 2; color: cOverUl.checked ? "#fb923c" : "#ffffff"; border.color: "#94a3b8"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "上传开销"; color: "#334155"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter } }
                MouseArea { anchors.fill: parent; onClicked: { cOverUl.checked = !cOverUl.checked; canvas.requestPaint(); } } }
            Item { id: cDhtDl; property bool checked: false; width: 128; height: 20
                Row { anchors.fill: parent; spacing: 6
                    Rectangle { width: 14; height: 14; radius: 2; color: cDhtDl.checked ? "#38bdf8" : "#ffffff"; border.color: "#94a3b8"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "DHT下载"; color: "#334155"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter } }
                MouseArea { anchors.fill: parent; onClicked: { cDhtDl.checked = !cDhtDl.checked; canvas.requestPaint(); } } }
            Item { id: cDhtUl; property bool checked: false; width: 128; height: 20
                Row { anchors.fill: parent; spacing: 6
                    Rectangle { width: 14; height: 14; radius: 2; color: cDhtUl.checked ? "#fb923c" : "#ffffff"; border.color: "#94a3b8"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "DHT上传"; color: "#334155"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter } }
                MouseArea { anchors.fill: parent; onClicked: { cDhtUl.checked = !cDhtUl.checked; canvas.requestPaint(); } } }
            Item { id: cTrDl; property bool checked: false; width: 128; height: 20
                Row { anchors.fill: parent; spacing: 6
                    Rectangle { width: 14; height: 14; radius: 2; color: cTrDl.checked ? "#38bdf8" : "#ffffff"; border.color: "#94a3b8"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "Tracker下载"; color: "#334155"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter } }
                MouseArea { anchors.fill: parent; onClicked: { cTrDl.checked = !cTrDl.checked; canvas.requestPaint(); } } }
            Item { id: cTrUl; property bool checked: false; width: 128; height: 20
                Row { anchors.fill: parent; spacing: 6
                    Rectangle { width: 14; height: 14; radius: 2; color: cTrUl.checked ? "#fb923c" : "#ffffff"; border.color: "#94a3b8"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "Tracker上传"; color: "#334155"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter } }
                MouseArea { anchors.fill: parent; onClicked: { cTrUl.checked = !cTrUl.checked; canvas.requestPaint(); } } }
            }
        }

        Canvas {
            id: canvas
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: controlsRow.bottom
            anchors.bottom: parent.bottom
            anchors.margins: 6

            property real leftMargin: 64
            property real topMargin: 8
            property real rightMargin: 16
            property real bottomMargin: 30

            function niceStep(rawStep) {
                if (rawStep <= 0) return 1;
                var p = Math.pow(10, Math.floor(Math.log(rawStep) / Math.log(10)));
                var n = rawStep / p;
                if (n <= 1) return 1 * p;
                if (n <= 2) return 2 * p;
                if (n <= 5) return 5 * p;
                return 10 * p;
            }

            function niceMaxFrom(maxVal, ticks) {
                var safe = Math.max(1, maxVal);
                var step = niceStep(safe / ticks);
                return step * ticks;
            }

            function yOf(v, axisMax, chartTop, chartHeight) {
                var ratio = v / axisMax;
                var y = chartTop + chartHeight - ratio * chartHeight;
                if (y > chartTop + chartHeight - 1) y = chartTop + chartHeight - 1;
                if (y < chartTop) y = chartTop;
                return y;
            }

            function periodMinutes() {
                if (root.periodIndex === 0) return 1;
                if (root.periodIndex === 1) return 5;
                if (root.periodIndex === 2) return 30;
                if (root.periodIndex === 3) return 60;
                if (root.periodIndex === 4) return 180;
                if (root.periodIndex === 5) return 360;
                if (root.periodIndex === 6) return 720;
                return 1440;
            }

            function selectedSeries() {
                var out = [];
                if (cTotalDl.checked) out.push({ key: "dl", name: "总下载", up: false, style: "solid" });
                if (cTotalUl.checked) out.push({ key: "ul", name: "总上传", up: true, style: "solid" });
                if (cPayloadDl.checked) out.push({ key: "payload_dl", name: "有效负荷下载", up: false, style: "solid" });
                if (cPayloadUl.checked) out.push({ key: "payload_ul", name: "有效负荷上传", up: true, style: "solid" });
                if (cOverDl.checked) out.push({ key: "overhead_dl", name: "下载开销", up: false, style: "dash" });
                if (cOverUl.checked) out.push({ key: "overhead_ul", name: "上传开销", up: true, style: "dash" });
                if (cDhtDl.checked) out.push({ key: "dht_dl", name: "DHT下载", up: false, style: "dot" });
                if (cDhtUl.checked) out.push({ key: "dht_ul", name: "DHT上传", up: true, style: "dot" });
                if (cTrDl.checked) out.push({ key: "tracker_dl", name: "Tracker下载", up: false, style: "step" });
                if (cTrUl.checked) out.push({ key: "tracker_ul", name: "Tracker上传", up: true, style: "step" });
                return out;
            }

            function shapeFor(s) {
                // 固定线形：同类上传/下载保持同线形，仅颜色区分
                if (s.key === "payload_dl" || s.key === "payload_ul") return "solid";
                if (s.key === "overhead_dl" || s.key === "overhead_ul") return "dash";
                if (s.key === "dht_dl" || s.key === "dht_ul") return "dot";
                if (s.key === "tracker_dl" || s.key === "tracker_ul") return "step";
                return s.style;
            }

            function shapeLabel(shape) {
                if (shape === "solid") return "实线";
                if (shape === "dash") return "虚线";
                if (shape === "dot") return "点线";
                if (shape === "step") return "阶梯";
                return shape;
            }

            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);

                var cw = width - leftMargin - rightMargin;
                var ch = height - topMargin - bottomMargin;
                if (cw < 80 || ch < 40) return;

                if (!chartModelRef) return;

                var selected = selectedSeries();
                if (selected.length === 0) return;
                var mins = periodMinutes();
                var keys = [];
                for (var kk = 0; kk < selected.length; kk++) keys.push(selected[kk].key);
                var maxVal = chartModelRef.maxRateForSeries(keys, mins);
                var count = 2;

                // dynamic axis: 0 baseline + nice ticks
                var tickCount = 5;
                var axisMax = niceMaxFrom(maxVal, tickCount);
                var tickStep = axisMax / tickCount;

                // grid lines (aligned with ticks)
                ctx.strokeStyle = "#e2e8f0";
                ctx.lineWidth = 1;
                ctx.setLineDash([4, 3]);
                for (var g = 0; g <= tickCount; g++) {
                    var gridVal = g * tickStep;
                    var gy = yOf(gridVal, axisMax, topMargin, ch);
                    ctx.beginPath();
                    ctx.moveTo(leftMargin, gy);
                    ctx.lineTo(leftMargin + cw, gy);
                    ctx.stroke();
                }
                ctx.setLineDash([]);

                // left axis and ticks
                ctx.strokeStyle = "#94a3b8";
                ctx.lineWidth = 1;
                ctx.beginPath();
                ctx.moveTo(leftMargin, topMargin);
                ctx.lineTo(leftMargin, topMargin + ch);
                ctx.stroke();
                // x-axis baseline (0 B/s) - draw thicker for visibility
                var baseY = yOf(0, axisMax, topMargin, ch);
                ctx.strokeStyle = "#64748b";
                ctx.lineWidth = 1.5;
                ctx.beginPath();
                ctx.moveTo(leftMargin, baseY);
                ctx.lineTo(leftMargin + cw, baseY);
                ctx.stroke();

                ctx.fillStyle = "#64748b";
                ctx.font = "10px sans-serif";
                ctx.textAlign = "right";
                for (var yi = 0; yi <= tickCount; yi++) {
                    var val = yi * tickStep;
                    var yy = yOf(val, axisMax, topMargin, ch);
                    ctx.beginPath();
                    ctx.moveTo(leftMargin - 4, yy);
                    ctx.lineTo(leftMargin, yy);
                    ctx.stroke();
                    ctx.fillText(chartModelRef.formatRate(val), leftMargin - 8, yy + 4);
                }

                // legend top-left
                var legendX = leftMargin + 8;
                var legendY = topMargin + 12;
                for (var li = 0; li < selected.length; li++) {
                    var s = selected[li];
                    var color = s.up ? "#fb923c" : "#38bdf8";
                    var shape = shapeFor(s);
                    ctx.strokeStyle = color;
                    ctx.lineWidth = 2;
                    if (shape === "dash") ctx.setLineDash([6, 3]);
                    else if (shape === "dot") ctx.setLineDash([2, 3]);
                    else ctx.setLineDash([]);
                    ctx.beginPath();
                    ctx.moveTo(legendX, legendY + li * 14);
                    ctx.lineTo(legendX + 16, legendY + li * 14);
                    ctx.stroke();
                    ctx.setLineDash([]);
                    ctx.fillStyle = "#334155";
                    ctx.font = "10px sans-serif";
                    ctx.textAlign = "left";
                    ctx.fillText(s.name + "（" + shapeLabel(shape) + "）", legendX + 20, legendY + 4 + li * 14);
                }

                // draw each selected series
                for (var si = 0; si < selected.length; si++) {
                    var ss = selected[si];
                    var data = chartModelRef.seriesSamples(ss.key, mins);
                    count = data.length;
                    var dx = cw / (count - 1);
                    var color = ss.up ? "#fb923c" : "#38bdf8";
                    var shape = shapeFor(ss);

                    // area fill for primary payload only
                    if (ss.key === "payload_dl" || ss.key === "payload_ul") {
                        ctx.beginPath();
                        ctx.moveTo(leftMargin, topMargin + ch);
                        for (var ai = 0; ai < count; ai++) {
                            var ax = leftMargin + dx * ai;
                            var ay = yOf(data[ai], axisMax, topMargin, ch);
                            ctx.lineTo(ax, ay);
                        }
                        ctx.lineTo(leftMargin + dx * (count - 1), topMargin + ch);
                        ctx.closePath();
                        ctx.fillStyle = ss.up ? "rgba(251, 146, 60, 0.10)" : "rgba(56, 189, 248, 0.14)";
                        ctx.fill();
                    }

                    ctx.strokeStyle = color;
                    ctx.lineWidth = 2.2;
                    if (shape === "dash") ctx.setLineDash([7, 4]);
                    else if (shape === "dot") ctx.setLineDash([2, 3]);
                    else ctx.setLineDash([]);
                    ctx.beginPath();

                    var x0 = leftMargin;
                    var y0 = yOf(data[0], axisMax, topMargin, ch);
                    ctx.moveTo(x0, y0);
                    for (var di = 1; di < count; di++) {
                        var x1 = leftMargin + dx * di;
                        var y1 = yOf(data[di], axisMax, topMargin, ch);
                        if (shape === "step") {
                            ctx.lineTo(x1, y0);
                            ctx.lineTo(x1, y1);
                        } else if (shape === "smooth") {
                            var cx = (x0 + x1) / 2;
                            ctx.quadraticCurveTo(cx, y0, x1, y1);
                        } else {
                            ctx.lineTo(x1, y1);
                        }
                        x0 = x1;
                        y0 = y1;
                    }
                    ctx.stroke();
                    ctx.setLineDash([]);
                }
            }

            Connections {
                target: chartModelRef
                function onSamplesChanged() { canvas.requestPaint(); }
            }
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            Component.onCompleted: requestPaint()

            // Some environments miss initial repaint notifications.
            // Keep a lightweight periodic repaint to guarantee visible updates.
            Timer {
                interval: 1000
                running: true
                repeat: true
                onTriggered: canvas.requestPaint()
            }
        }
    }
}
