import bb.cascades 1.0

Page {
    id: about
    signal close()
    signal openUrl(string urlString)
    property string appName: "Contacts Inspector"
    property string versionNumber: "X.Y.Z"
    property string appCopyright: "Copyright \u00A9 2014, Derek Konigsberg\nAll Rights Reserved"
    property bool darkTheme: (Application.themeSupport.theme.colorTheme.style == VisualStyle.Dark)
    titleBar: TitleBar {
        title: qsTr("About") + Retranslate.onLanguageChanged
        dismissAction: ActionItem {
            title: qsTr("Close") + Retranslate.onLanguageChanged
            onTriggered: {
                about.close();
            }
        }
    }
    content: Container {
        layout: DockLayout {
        }
        ScrollView {
            scrollViewProperties {
                scrollMode: ScrollMode.Vertical
            }
            verticalAlignment: VerticalAlignment.Fill
            horizontalAlignment: HorizontalAlignment.Fill
            
            Container {
                topPadding: 50
                bottomPadding: 50
                leftPadding: 50
                rightPadding: 50
                layout: StackLayout {
                    orientation: LayoutOrientation.TopToBottom
                }
                horizontalAlignment: HorizontalAlignment.Center
                Label {
                    text: appName
                    topMargin: 25
                    bottomMargin: 25
                    horizontalAlignment: HorizontalAlignment.Center
                    textStyle {
                        base: SystemDefaults.TextStyles.BigText
                    }
                }
                Label {
                    text: qsTr("Version %1").arg(versionNumber) + Retranslate.onLanguageChanged
                    topMargin: 25
                    bottomMargin: 25
                    horizontalAlignment: HorizontalAlignment.Center
                    textStyle {
                        base: SystemDefaults.TextStyles.TitleText
                    }
                }
                Label {
                    text: appCopyright
                    topMargin: 25
                    bottomMargin: 25
                    multiline: true
                    horizontalAlignment: HorizontalAlignment.Center
                    textStyle {
                        base: SystemDefaults.TextStyles.SubtitleText
                        textAlign: TextAlign.Center
                    }
                }
                Divider {
                    topMargin: 50
                    bottomMargin: 50
                }
                Label {
                    topMargin: 50
                    bottomMargin: 50
                    multiline: true
                    text: qsTr("In-app icons from the BlackBerry Developer site") + Retranslate.onLanguageChanged
                    horizontalAlignment: HorizontalAlignment.Center
                    textStyle {
                        base: SystemDefaults.TextStyles.SubtitleText
                        color: darkTheme ? Color.Blue : Color.DarkBlue
                    }
                    onTouch: {
                        if (event.isUp()) {
                            about.openUrl("https://developer.blackberry.com/design/bb10/");
                        }
                    }
                }
                Label {
                    topMargin: 50
                    bottomMargin: 50
                    multiline: true
                    text: qsTr("Application icon based on icons from the Tango Desktop Project") + Retranslate.onLanguageChanged
                    horizontalAlignment: HorizontalAlignment.Center
                    textStyle {
                        base: SystemDefaults.TextStyles.SubtitleText
                        color: darkTheme ? Color.Blue : Color.DarkBlue
                    }
                    onTouch: {
                        if (event.isUp()) {
                            about.openUrl("http://tango.freedesktop.org/");
                        }
                    }
                }
            }
        }
        ImageView {
            imageSource: "asset:///images/github-forkme.png"
            verticalAlignment: VerticalAlignment.Top
            horizontalAlignment: HorizontalAlignment.Right
            onTouch: {
                if (event.isUp()) {
                    about.openUrl("https://github.com/dkonigsberg/ContactsInspector");
                }
            }
        }
    }
}
