import QtQuick 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1 as Platform
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

Item {
    id: root
    implicitWidth: PlasmaCore.Units.gridUnit * 14
    implicitHeight: PlasmaCore.Units.gridUnit * 10

    property string summaryPath: Platform.StandardPaths.writableLocation(Platform.StandardPaths.GenericDataLocation) + "/rtcwake-gui/next-wake.json"
    property string nextWakeText: i18n("No wake scheduled yet")
    property string nextActionText: i18n("Action: not set")

    signal refreshRequested()

    function refreshSummary() {
        if (!summaryPath || summaryPath.length === 0)
            return;
        summaryReader.connectSource("/usr/bin/env cat \"" + summaryPath + "\"");
    }

    PlasmaCore.DataSource {
        id: summaryReader
        engine: "executable"
        connectedSources: []
        onNewData: function(source, data) {
            var text = data["stdout"] || "";
            if (text.length > 0) {
                try {
                    var parsed = JSON.parse(text);
                    root.nextWakeText = parsed.friendly || root.nextWakeText;
                    root.nextActionText = parsed.action ? i18n("Action: %1", parsed.action) : root.nextActionText;
                } catch (e) {
                    root.nextWakeText = i18n("Unable to parse status");
                }
            }
            disconnectSource(source);
        }
    }

    PlasmaCore.DataSource {
        id: launcher
        engine: "executable"
        connectedSources: []
        onNewData: function(source) {
            disconnectSource(source);
        }
    }

    Timer {
        interval: 60000
        running: true
        repeat: true
        onTriggered: root.refreshSummary()
    }

    Component.onCompleted: root.refreshSummary()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: PlasmaCore.Units.smallSpacing
        spacing: PlasmaCore.Units.smallSpacing

        PlasmaComponents3.Label {
            text: i18n("rtcwake Planner")
            font.bold: true
        }

        PlasmaComponents3.Label {
            text: root.nextWakeText
            wrapMode: Text.WordWrap
        }

        PlasmaComponents3.Label {
            text: root.nextActionText
            wrapMode: Text.WordWrap
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: PlasmaCore.Units.smallSpacing

            PlasmaComponents3.Button {
                text: i18n("Refresh")
                onClicked: root.refreshSummary()
            }

            PlasmaComponents3.Button {
                text: i18n("Open planner")
                Layout.fillWidth: true
                onClicked: launcher.connectSource("rtcwake-gui")
            }
        }
    }
}
