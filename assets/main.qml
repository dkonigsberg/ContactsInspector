import bb.cascades 1.0

NavigationPane {
    id: nav
    Page {
        id: page
        objectName: "rootPage"
        signal refreshList()
        signal search()
        signal openContact(int contactId)
        property alias activityRunning: activityIndicator.running
        content: Container {
            layout: DockLayout { }
            ListView {
                objectName: "listView"
                verticalAlignment: VerticalAlignment.Fill
                horizontalAlignment: HorizontalAlignment.Fill
                dataModel: GroupDataModel {
                    objectName: "dataModel"
                    grouping: ItemGrouping.ByFirstChar
                    sortingKeys: ["displayName"]
                }
                listItemComponents: [
                    ListItemComponent {
                        type: "item"
                        StandardListItem {
                            title: ListItemData.displayName
                            description: ListItemData.displayCompanyName
                            status: ListItemData.contactId
                            imageSource: ListItemData.photo
                            imageSpaceReserved: true
                        }
                    }
                ]
                onTriggered: {
                    if (indexPath.length > 1) {
                        var chosenItem = dataModel.data(indexPath);
                        page.openContact(chosenItem.contactId);
                    }
                }
            }
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                layout: DockLayout { }
                background: Color.create(0.0, 0.0, 0.0, 0.25)
                visible: activityIndicator.running
                overlapTouchPolicy: activityIndicator.running ? OverlapTouchPolicy.Deny : OverlapTouchPolicy.Allow
                touchPropagationMode: activityIndicator.running ? TouchPropagationMode.Full : TouchPropagationMode.None
                ActivityIndicator {
                    id: activityIndicator
                    verticalAlignment: VerticalAlignment.Center
                    horizontalAlignment: HorizontalAlignment.Center
                    preferredHeight: 400
                    preferredWidth: 400
                }
            }
        }
        actions: [
            ActionItem {
                enabled: !page.activityRunning
                title: qsTr("Refresh") + Retranslate.onLanguageChanged
                imageSource: "asset:///images/ic_reload.png"
                ActionBar.placement: ActionBarPlacement.OnBar
                onTriggered: {
                    page.refreshList()
                }
            },
            ActionItem {
                enabled: !page.activityRunning
                title: qsTr("Search") + Retranslate.onLanguageChanged
                imageSource: "asset:///images/ic_search.png"
                ActionBar.placement: ActionBarPlacement.OnBar
                onTriggered: {
                    page.search()
                }
            }
        ]
    }
}
