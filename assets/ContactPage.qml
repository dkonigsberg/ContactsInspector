import bb.cascades 1.0

Page {
    id: page
    property int contactId
    property alias photoImageSource: photoView.imageSource
    property alias displayName: displayNameLabel.text
    property alias displayCompanyName: displayCompanyLabel.text
    
    signal propertiesSelected()
    signal attributesSelected()
    
    titleBar: TitleBar {
        title: qsTr("Contact Details") + Retranslate.onLanguageChanged
    }
    
    content: Container {
        horizontalAlignment: HorizontalAlignment.Fill
        layout: DockLayout {}
        
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Top
            
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: 20
                bottomPadding: 20
                leftPadding: 20
                rightPadding: 20
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }
                ImageView {
                    id: photoView
                    verticalAlignment: VerticalAlignment.Center
                }
                Container {
                    leftMargin: 20
                    layout: StackLayout {
                        orientation: LayoutOrientation.TopToBottom
                    }
                    Label {
                        id: displayNameLabel
                        horizontalAlignment: HorizontalAlignment.Fill
                        topMargin: 0
                        bottomMargin: 0
                        multiline: true
                        textStyle {
                            base: SystemDefaults.TextStyles.TitleText
                        }
                        textFormat: TextFormat.Plain
                        content.flags: TextContentFlag.ActiveTextOff
                    }
                    Label {
                        id: displayCompanyLabel
                        horizontalAlignment: HorizontalAlignment.Fill
                        topMargin: 0
                        bottomMargin: 0
                        multiline: true
                        textStyle {
                            base: SystemDefaults.TextStyles.TitleText
                        }
                        textFormat: TextFormat.Plain
                        content.flags: TextContentFlag.ActiveTextOff
                    }
                    Label {
                        id: contactIdLabel
                        horizontalAlignment: HorizontalAlignment.Fill
                        topMargin: 0
                        bottomMargin: 0
                        multiline: true
                        text: qsTr("ID: %1").arg(contactId) + Retranslate.onLanguageChanged
                        textStyle {
                            base: SystemDefaults.TextStyles.TitleText
                        }
                        textFormat: TextFormat.Plain
                        content.flags: TextContentFlag.ActiveTextOff
                    }
                }
            }
            
            SegmentedControl {
                horizontalAlignment: HorizontalAlignment.Center
                Option {
                    text: qsTr("Properties") + Retranslate.onLanguageChanged
                    selected: true
                }
                Option {
                    text: qsTr("Attributes") + Retranslate.onLanguageChanged
                }
                onSelectedIndexChanged: {
                    if(selectedIndex == 0) {
                        page.propertiesSelected()
                    } else if(selectedIndex == 1) {
                        page.attributesSelected()
                    }
                }
            }

            ListView {
                objectName: "listView"
                listItemComponents: [
                    ListItemComponent {
                        type: "item"
                        StandardListItem {
                            title: ListItemData.title
                            description: ListItemData.description
                            status: ListItemData.status
                            imageSource: ListItemData.imageSource
                        }
                    }
                ]
            }
        }
    }
}
