import Qt 4.7

MenuListEntry {
    id: rotationElement
    property int    ident:         0
    property alias  modeText:      rotationLock.text
    property bool   locked:        false
    property bool   delayUpdate:   false
    property string newText:       ""
    property bool   newLockStatus: false
    property real uiScale;
    property real textScale;

    property int iconSpacing : 4 * uiScale
    property int rightMarging: 8 * uiScale

    content:
        Item {
        width: rotationElement.width

            Text {
            id: rotationLock
                x: ident;
                anchors.verticalCenter: parent.verticalCenter
                text: runtime.getLocalizedString("Turn on Rotation Lock");
                color: "#FFF";
                font.bold: false;
                font.pixelSize: 18 * textScale;
                font.family: "Prelude"
            }

            Image {
                id: lockIndicatorOn
                visible: !locked
                x: parent.width - width - iconSpacing - rightMarging
                anchors.verticalCenter: parent.verticalCenter
                scale: uiScale

                source: "/usr/palm/sysmgr/images/statusBar/icon-rotation-lock.png"
             }

            Image {
                id: lockIndicatorOff
                visible: locked
                x: parent.width - width - iconSpacing - rightMarging
                anchors.verticalCenter: parent.verticalCenter
                scale: uiScale

                source: "/usr/palm/sysmgr/images/statusBar/icon-rotation-lock-off.png"
             }
        }
}
