import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AirgapFormatter

Popup {
    id: passphrasePrompt
    modal: true
    anchors.centerIn: parent
    width: Math.min(400, parent.width - 40)
    padding: 20
    closePolicy: Popup.CloseOnEscape

    signal passphraseEntered(string passphrase)
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

    // Function to show error and allow retry
    function showError(message) {
        errorMessage = message;
        isBusy = false;
        passphraseField.forceActiveFocus();
        passphraseField.selectAll();
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
            text: "Enter Passphrase"
            font.pixelSize: 18
            font.weight: Font.DemiBold
            color: Theme.textPrimary
        }

        // Info text
        Text {
            text: "This shared link is passphrase-protected."
            font.pixelSize: 13
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // Passphrase field
        TextField {
            id: passphraseField
            Layout.fillWidth: true
            placeholderText: "Enter the passphrase"
            echoMode: TextInput.Password
            enabled: !passphrasePrompt.isBusy

            background: Rectangle {
                implicitHeight: 40
                color: Theme.background
                border.color: passphrasePrompt.errorMessage !== "" ? Theme.textError :
                              passphraseField.activeFocus ? Theme.focusRing : Theme.border
                border.width: passphraseField.activeFocus ? Theme.focusRingWidth : 1
                radius: 4
            }

            color: Theme.textPrimary
            placeholderTextColor: Theme.textSecondary

            // Submit on Enter
            Keys.onReturnPressed: {
                if (!passphrasePrompt.isBusy && passphraseField.text.trim() !== "") {
                    passphrasePrompt.isBusy = true;
                    passphrasePrompt.errorMessage = "";
                    passphrasePrompt.passphraseEntered(passphraseField.text);
                }
            }
        }

        // Error message area (for wrong passphrase)
        Rectangle {
            visible: passphrasePrompt.errorMessage !== ""
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
                text: passphrasePrompt.errorMessage
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
                enabled: !passphrasePrompt.isBusy

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
                    passphrasePrompt.close();
                    passphrasePrompt.cancelled();
                }
            }

            // Decrypt button (primary)
            Button {
                id: decryptButton
                text: passphrasePrompt.isBusy ? "..." : "Decrypt"
                enabled: !passphrasePrompt.isBusy && passphraseField.text.trim() !== ""

                contentItem: Text {
                    text: decryptButton.text
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    opacity: decryptButton.enabled ? 1.0 : 0.7
                }

                background: Rectangle {
                    implicitWidth: 80
                    implicitHeight: 36
                    color: !decryptButton.enabled ? Qt.darker(Theme.accent, 1.3) :
                           decryptButton.pressed ? Qt.darker(Theme.accent, 1.2) :
                           decryptButton.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent
                    border.color: decryptButton.activeFocus ? Theme.focusRing : "transparent"
                    border.width: decryptButton.activeFocus ? Theme.focusRingWidth : 0
                    radius: 4
                }

                onClicked: {
                    passphrasePrompt.isBusy = true;
                    passphrasePrompt.errorMessage = "";
                    passphrasePrompt.passphraseEntered(passphraseField.text);
                }
            }
        }
    }
}
