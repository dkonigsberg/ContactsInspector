#ifndef CONTACTPAGE_HPP
#define CONTACTPAGE_HPP

#include <QtCore/QObject>

#include <bb/pim/contacts/Contact>
#include <bb/pim/contacts/ContactAttribute>

namespace bb { namespace cascades {
class Page;
class NavigationPane;
class ListView;
class GroupDataModel;
}}

class ContactPage : public QObject
{
    Q_OBJECT
public:
    ContactPage(int contactId, QObject *parent=0);
    virtual ~ContactPage();
    void push(bb::cascades::NavigationPane *navPane);
private slots:
    void onPropertiesSelected();
    void onAttributesSelected();
    void onSaveData();
    void onPickerFileSelected(const QStringList& selectedFiles);
    void onPickerCanceled();
private:
    void populateContactFields();
    void populateContactProperties(const bb::pim::contacts::Contact &contact);
    void populateContactAttributes(const bb::pim::contacts::Contact &contact);
    void populateExportData(const bb::pim::contacts::Contact &contact);
    static QString attributeKindName(bb::pim::contacts::AttributeKind::Type kind);
    static QString attributeSubKindName(bb::pim::contacts::AttributeSubKind::Type subKind);
    int contactId_;
    bb::cascades::Page *page_;
    bb::cascades::NavigationPane *navPane_;
    bb::cascades::ListView *listView_;
    bb::cascades::GroupDataModel *propertiesModel_;
    bb::cascades::GroupDataModel *attributesModel_;
    QByteArray exportData_;
};

#endif // CONTACTPAGE_HPP
