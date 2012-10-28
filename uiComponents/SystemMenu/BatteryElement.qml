import Qt 4.7

MenuListEntry {
    id: thisElement
    property int ident: 0
    property string batteryText;
    property real uiScale;
    property real textScale;

    Connections {
        target: NativeSystemMenuHandler
        onBatteryLevelUpdated: batteryText = batteryLevel
    }

    selectable: false
    content:
        Row {
            x: ident;
            width: parent.width
            Text {
                text: runtime.getLocalizedString("Battery: ");
                color: "#AAA";
                font.pixelSize: 18 * textScale;
                font.family: "Prelude"
            }

            Text {
                text: batteryText;
                color: "#AAA";
                font.pixelSize: 18 * textScale;
                font.family: "Prelude"
            }
        }
}
