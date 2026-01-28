import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AirgapFormatter

Popup {
    id: shareDialog
    modal: true
    anchors.centerIn: parent
    width: Math.min(400, parent.width - 40)
    padding: 20
    closePolicy: Popup.CloseOnEscape

    signal shareConfirmed(string passphrase)
    signal cancelled()

    property string errorMessage: ""
    property bool isBusy: false

    // Reset state when opening
    onOpened: {
        passphraseField.text = "";
        errorMessage = "";
        isBusy = false;
        passphraseField.forceActiveFocus();
    }

    // Handle escape key
    onClosed: {
        if (!isBusy) {
            cancelled();
        }
    }

    background: Rectangle {
        color: Theme.backgroundSecondary
        border.color: Theme.border
        border.width: 1
        radius: 8
    }

    ColumnLayout {
        spacing: 16
        width: parent.width

        // Title
        Text {
            text: "Share JSON"
            font.pixelSize: 18
            font.weight: Font.DemiBold
            color: Theme.textPrimary
        }

        // Security info text
        Text {
            text: "Creates a time-bounded encrypted link valid for 5 minutes."
            font.pixelSize: 13
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // Passphrase field
        TextField {
            id: passphraseField
            Layout.fillWidth: true
            placeholderText: "Optional passphrase for extra security"
            echoMode: TextInput.Password
            enabled: !shareDialog.isBusy

            background: Rectangle {
                implicitHeight: 40
                color: Theme.background
                border.color: passphraseField.activeFocus ? Theme.focusRing : Theme.border
                border.width: passphraseField.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }

            color: Theme.textPrimary
            placeholderTextColor: Theme.textSecondary

            // Submit on Enter
            Keys.onReturnPressed: {
                if (!shareDialog.isBusy) {
                    shareDialog.shareConfirmed(passphraseField.text);
                }
            }
        }

        // Passphrase hint
        Text {
            text: "Leave empty for quick share, or set passphrase for two-factor security."
            font.pixelSize: 11
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            opacity: 0.8
        }

        // Error message area
        Rectangle {
            visible: shareDialog.errorMessage !== ""
            Layout.fillWidth: true
            height: errorText.height + 16
            color: Theme.badgeErrorBg
            border.color: Theme.badgeErrorBorder
            border.width: 1
            radius: 4

            Text {
                id: errorText
                anchors.centerIn: parent
                width: parent.width - 16
                text: shareDialog.errorMessage
                color: Theme.textError
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }

        // Button row
        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 8

            // Cancel button
            Button {
                id: cancelButton
                text: "Cancel"
                enabled: !shareDialog.isBusy

                contentItem: Text {
                    text: cancelButton.text
                    color: Theme.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                    opacity: cancelButton.enabled ? 1.0 : 0.5
                }

                background: Rectangle {
                    implicitWidth: 80
                    implicitHeight: 36
                    color: cancelButton.hovered && cancelButton.enabled ? Theme.backgroundTertiary : "transparent"
                    border.color: Theme.border
                    border.width: 1
                    radius: 4
                }

                onClicked: {
                    shareDialog.close();
                    shareDialog.cancelled();
                }
            }

            // Share button (primary)
            Button {
                id: shareButton
                text: shareDialog.isBusy ? "..." : "Share"
                enabled: !shareDialog.isBusy

                contentItem: Text {
                    text: shareButton.text
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    opacity: shareButton.enabled ? 1.0 : 0.7
                }

                background: Rectangle {
                    implicitWidth: 80
                    implicitHeight: 36
                    color: !shareButton.enabled ? Qt.darker(Theme.accent, 1.3) :
                           shareButton.pressed ? Qt.darker(Theme.accent, 1.2) :
                           shareButton.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent
                    border.color: shareButton.activeFocus ? Theme.focusRing : "transparent"
                    border.width: shareButton.activeFocus ? Theme.focusRingWidth : 0
                    radius: 4
                }

                onClicked: {
                    shareDialog.shareConfirmed(passphraseField.text);
                }
            }
        }
    }
}
