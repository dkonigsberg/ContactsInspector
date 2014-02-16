#include "contactpage.hpp"

#include <QtCore/QUrl>
#include <QtCore/QFile>

#include <bb/cascades/QmlDocument>
#include <bb/cascades/Page>
#include <bb/cascades/NavigationPane>
#include <bb/cascades/ListView>
#include <bb/cascades/GroupDataModel>
#include <bb/cascades/ActionItem>
#include <bb/cascades/InvokeActionItem>
#include <bb/cascades/InvokeQuery>
#include <bb/cascades/pickers/FilePicker>
#include <bb/system/SystemToast>
#include <bb/pim/contacts/ContactService>
#include <bb/pim/contacts/Contact>
#include <bb/pim/contacts/ContactAttribute>
#include <bb/pim/contacts/ContactPostalAddress>
#include <bb/pim/account/AccountService>
#include <bb/pim/account/Account>
#include <bb/pim/account/Provider>
#include <bb/data/JsonDataAccess>

using namespace bb::cascades;

ContactPage::ContactPage(int contactId, QObject *parent)
    : QObject(parent), contactId_(contactId), navPane_(NULL)
{
    QmlDocument *qml = QmlDocument::create("asset:///ContactPage.qml").parent(this);
    qml->setContextProperty("cs", this);
    page_ = qml->createRootObject<Page>();
    connect(page_, SIGNAL(propertiesSelected()), this, SLOT(onPropertiesSelected()));
    connect(page_, SIGNAL(attributesSelected()), this, SLOT(onAttributesSelected()));
    connect(page_, SIGNAL(destroyed()), this, SLOT(deleteLater()));
    page_->setProperty("contactId", contactId_);

    listView_ = page_->findChild<ListView *>("listView");

    propertiesModel_ = new GroupDataModel(this);
    propertiesModel_->setGrouping(ItemGrouping::ByFullValue);
    propertiesModel_->setSortingKeys(QStringList() << "property" << "title" << "description");

    attributesModel_ = new GroupDataModel(this);
    attributesModel_->setGrouping(ItemGrouping::ByFullValue);
    attributesModel_->setSortingKeys(QStringList() << "kind" << "title" << "description");

    InvokeActionItem *openAction = InvokeActionItem::create(InvokeQuery::create()
        .invokeTargetId("sys.pim.contacts.app")
        .mimeType("application/vnd.blackberry.contact.id")
        .data(QByteArray::number(contactId_))).parent(page_);
    page_->addAction(openAction, ActionBarPlacement::OnBar);

    ActionItem *saveAction = ActionItem::create()
        .title(tr("Save Data"))
        .imageSource(QUrl("asset:///images/ic_save.png"))
        .onTriggered(this, SLOT(onSaveData()))
        .parent(this);
    page_->addAction(saveAction, ActionBarPlacement::OnBar);
}

ContactPage::~ContactPage()
{
}

void ContactPage::push(bb::cascades::NavigationPane *navPane)
{
    if(navPane_) {
        qCritical() << "Cannot push page that has already been pushed";
        return;
    }

    navPane->push(page_);
    navPane_ = navPane;
    populateContactFields();
}

void ContactPage::populateContactFields()
{
    bb::pim::contacts::ContactService contactService;
    const bb::pim::contacts::Contact contact = contactService.contactDetails(contactId_);

    if(!contact.smallPhotoFilepath().isEmpty()) {
        page_->setProperty("photoImageSource", QLatin1String("file://") + contact.smallPhotoFilepath());
    }
    page_->setProperty("displayName", contact.displayName());
    page_->setProperty("displayCompanyName", contact.displayCompanyName());

    populateContactProperties(contact);
    populateContactAttributes(contact);
    populateExportData(contact);

    onPropertiesSelected();
}

void ContactPage::populateContactProperties(const bb::pim::contacts::Contact &contact)
{
    bb::pim::account::AccountService accountService;

    foreach(const bb::pim::contacts::AccountId accountId, contact.sourceAccountIds()) {
        bb::pim::account::Account account = accountService.account(accountId);
        bb::pim::account::Provider provider = account.provider();

        QVariantMap map;
        map["property"] = tr("Source account");
        map["title"] = account.displayName();
        map["description"] = provider.name();
        map["status"] = accountId;
        propertiesModel_->insert(map);
    }

    if(!contact.firstName().isEmpty()) {
        QVariantMap map;
        map["property"] = tr("First name");
        map["title"] = contact.firstName();
        propertiesModel_->insert(map);
    }

    if(!contact.lastName().isEmpty()) {
        QVariantMap map;
        map["property"] = tr("Last name");
        map["title"] = contact.lastName();
        propertiesModel_->insert(map);
    }

    foreach(const bb::pim::contacts::ContactAttribute &attribute, contact.emails()) {
        QVariantMap map;
        map["property"] = tr("Email");
        map["title"] = attribute.value();
        map["status"] = attribute.attributeDisplayLabel();
        propertiesModel_->insert(map);
    }

    foreach(const bb::pim::contacts::ContactAttribute &attribute, contact.phoneNumbers()) {
        QVariantMap map;
        map["property"] = tr("Phone");
        map["title"] = attribute.value();
        map["status"] = attribute.attributeDisplayLabel();
        propertiesModel_->insert(map);
    }

    foreach(const bb::pim::contacts::ContactPhoto &photo, contact.photos()) {
        QVariantMap map;
        map["property"] = tr("Photo");
        map["title"] = tr("ID: %1").arg(photo.id());
        map["description"] = tr("Account: %1").arg(photo.sourceAccountId());
        if(photo.id() == contact.primaryPhoto().id()) {
            map["status"] = tr("Primary");
        }
        map["imageSource"] = photo.smallPhoto();
        propertiesModel_->insert(map);
    }

    foreach(const bb::pim::contacts::ContactPostalAddress &address, contact.postalAddresses()) {
        QStringList fields;
        if(!address.line1().isEmpty()) {
            fields.append(address.line1());
        }
        if(!address.line2().isEmpty()) {
            fields.append(address.line2());
        }
        if(!address.city().isEmpty()) {
            fields.append(address.city());
        }
        if(!address.region().isEmpty()) {
            fields.append(address.region());
        }
        if(!address.country().isEmpty()) {
            fields.append(address.country());
        }
        QVariantMap map;
        map["property"] = tr("Address");
        map["title"] = fields.join("; ");
        map["description"] = address.label();
        propertiesModel_->insert(map);
    }
}

void ContactPage::populateContactAttributes(const bb::pim::contacts::Contact &contact)
{
    foreach(const bb::pim::contacts::ContactAttribute &attribute, contact.attributes()) {
        QVariantMap map;
        map["kind"] = attributeKindName(attribute.kind());
        map["title"] = attributeSubKindName(attribute.subKind());
        map["description"] = attribute.value();
        map["status"] = attribute.id();
        attributesModel_->insert(map);
    }
}

void ContactPage::populateExportData(const bb::pim::contacts::Contact &contact)
{
    QVariantMap data;

    QVariantMap header;
    header["accountId"] = contact.accountId();
    header["contactId"] = contact.id();
    header["displayName"] = contact.displayName();
    header["displayCompanyName"] = contact.displayCompanyName();
    data["header"] = header;

    bb::pim::account::AccountService accountService;

    QVariantList sourceAccounts;
    foreach(const bb::pim::contacts::AccountId accountId, contact.sourceAccountIds()) {
        bb::pim::account::Account account = accountService.account(accountId);
        bb::pim::account::Provider provider = account.provider();

        QVariantMap map;
        map["id"] = accountId;
        map["displayName"] = account.displayName();
        map["providerId"] = provider.id();
        map["providerName"] = provider.name();
        sourceAccounts.append(map);
    }
    data["sourceAccounts"] = sourceAccounts;

    QVariantList photoProperties;
    foreach(const bb::pim::contacts::ContactPhoto &photo, contact.photos()) {
        QVariantMap map;
        map["id"] = photo.id();
        map["sourceAccountId"] = photo.sourceAccountId();
        map["isPrimary"] = (photo.id() == contact.primaryPhoto().id());
        photoProperties.append(map);
    }
    data["photos"] = photoProperties;

    QVariantList attributes;
    foreach(const bb::pim::contacts::ContactAttribute &attribute, contact.attributes()) {
        QVariantMap map;
        map["id"] = attribute.id();

        QVariantList sourcesList;
        foreach(int source, attribute.sources()) {
            sourcesList.append(qVariantFromValue(source));
        }
        map["sources"] = sourcesList;

        map["kind"] = attributeKindName(attribute.kind());
        map["subKind"] = attributeSubKindName(attribute.subKind());
        map["value"] = attribute.value();
        attributes.append(map);
    }
    data["attributes"] = attributes;

    bb::data::JsonDataAccess json;
    json.saveToBuffer(data, &exportData_);
    if(json.hasError()) {
        qWarning() << "Error preparing export data:" << json.error().errorMessage();
    }
}

void ContactPage::onPropertiesSelected()
{
    listView_->setDataModel(propertiesModel_);
}

void ContactPage::onAttributesSelected()
{
    listView_->setDataModel(attributesModel_);
}

void ContactPage::onSaveData()
{
    pickers::FilePicker* filePicker = new pickers::FilePicker(this);
    filePicker->setType(pickers::FileType::Other);
    filePicker->setMode(pickers::FilePickerMode::Saver);
    filePicker->setDefaultSaveFileNames(QStringList() << QString("contact-%1.txt").arg(contactId_));
    connect(filePicker, SIGNAL(fileSelected(QStringList)), this, SLOT(onPickerFileSelected(QStringList)), Qt::QueuedConnection);
    connect(filePicker, SIGNAL(canceled()), this, SLOT(onPickerCanceled()));
    filePicker->open();
}

void ContactPage::onPickerFileSelected(const QStringList& selectedFiles)
{
    pickers::FilePicker *picker = qobject_cast<pickers::FilePicker*>(sender());
    picker->deleteLater();
    if(selectedFiles.length() < 1 || selectedFiles[0].isEmpty()) { return; }
    const QString filename = selectedFiles[0];

    QFile file(filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(exportData_);
        file.close();
        file.setPermissions(
            QFile::ReadOwner | QFile::WriteOwner |
            QFile::ReadGroup | QFile::WriteGroup |
            QFile::ReadOther | QFile::WriteOther);

        bb::system::SystemToast *toast = new bb::system::SystemToast(this);
        connect(toast, SIGNAL(finished(bb::system::SystemUiResult::Type)), toast, SLOT(deleteLater()));
        toast->setBody(tr("Contact data saved to file"));
        toast->show();
    }
    else {
        qWarning() << "Unable to open file for writing:" << file.errorString();

        bb::system::SystemToast *toast = new bb::system::SystemToast(this);
        connect(toast, SIGNAL(finished(bb::system::SystemUiResult::Type)), toast, SLOT(deleteLater()));
        toast->setBody(tr("Unable to save contact data"));
        toast->show();
    }
}

void ContactPage::onPickerCanceled()
{
    pickers::FilePicker *picker = qobject_cast<pickers::FilePicker*>(sender());
    picker->deleteLater();
}

QString ContactPage::attributeKindName(bb::pim::contacts::AttributeKind::Type kind)
{
    switch(kind) {
    case bb::pim::contacts::AttributeKind::Invalid:
        return "Invalid";
    case bb::pim::contacts::AttributeKind::Phone:
        return "Phone";
    case bb::pim::contacts::AttributeKind::Fax:
        return "Fax";
    case bb::pim::contacts::AttributeKind::Pager:
        return "Pager";
    case bb::pim::contacts::AttributeKind::Email:
        return "Email";
    case bb::pim::contacts::AttributeKind::Website:
        return "Website";
    case bb::pim::contacts::AttributeKind::Feed:
        return "Feed";
    case bb::pim::contacts::AttributeKind::Profile:
        return "Profile";
    case bb::pim::contacts::AttributeKind::Family:
        return "Family";
    case bb::pim::contacts::AttributeKind::Person:
        return "Person";
    case bb::pim::contacts::AttributeKind::Date:
        return "Date";
    case bb::pim::contacts::AttributeKind::Group:
        return "Group";
    case bb::pim::contacts::AttributeKind::Name:
        return "Name";
    case bb::pim::contacts::AttributeKind::StockSymbol:
        return "StockSymbol";
    case bb::pim::contacts::AttributeKind::Ranking:
        return "Ranking";
    case bb::pim::contacts::AttributeKind::OrganizationAffiliation:
        return "OrganizationAffiliation";
    case bb::pim::contacts::AttributeKind::Education:
        return "Education";
    case bb::pim::contacts::AttributeKind::Note:
        return "Note";
    case bb::pim::contacts::AttributeKind::InstantMessaging:
        return "InstantMessaging";
    case bb::pim::contacts::AttributeKind::VideoChat:
        return "VideoChat";
    case bb::pim::contacts::AttributeKind::ConnectionCount:
        return "ConnectionCount";
    case bb::pim::contacts::AttributeKind::Hidden:
        return "Hidden";
    case bb::pim::contacts::AttributeKind::Biography:
        return "Biography";
    case bb::pim::contacts::AttributeKind::Sound:
        return "Sound";
    case bb::pim::contacts::AttributeKind::Notification:
        return "Notification";
    case bb::pim::contacts::AttributeKind::MessageSound:
        return "MessageSound";
    case bb::pim::contacts::AttributeKind::MessageNotification:
        return "MessageNotification";
    default:
        return QString("Kind (%1)").arg(kind);
    }
}

QString ContactPage::attributeSubKindName(bb::pim::contacts::AttributeSubKind::Type subKind)
{
    switch(subKind) {
    case bb::pim::contacts::AttributeSubKind::Invalid:
        return "Invalid";
    case bb::pim::contacts::AttributeSubKind::Other:
        return "Other";
    case bb::pim::contacts::AttributeSubKind::Home:
        return "Home";
    case bb::pim::contacts::AttributeSubKind::Work:
        return "Work";
    case bb::pim::contacts::AttributeSubKind::PhoneMobile:
        return "PhoneMobile";
    case bb::pim::contacts::AttributeSubKind::FaxDirect:
        return "FaxDirect";
    case bb::pim::contacts::AttributeSubKind::Blog:
        return "Blog";
    case bb::pim::contacts::AttributeSubKind::WebsiteResume:
        return "WebsiteResume";
    case bb::pim::contacts::AttributeSubKind::WebsitePortfolio:
        return "WebsitePortfolio";
    case bb::pim::contacts::AttributeSubKind::WebsitePersonal:
        return "WebsitePersonal";
    case bb::pim::contacts::AttributeSubKind::WebsiteCompany:
        return "WebsiteCompany";
    case bb::pim::contacts::AttributeSubKind::ProfileFacebook:
        return "ProfileFacebook";
    case bb::pim::contacts::AttributeSubKind::ProfileTwitter:
        return "ProfileTwitter";
    case bb::pim::contacts::AttributeSubKind::ProfileLinkedIn:
        return "ProfileLinkedIn";
    case bb::pim::contacts::AttributeSubKind::ProfileGist:
        return "ProfileGist";
    case bb::pim::contacts::AttributeSubKind::ProfileTungle:
        return "ProfileTungle";
    case bb::pim::contacts::AttributeSubKind::FamilySpouse:
        return "FamilySpouse";
    case bb::pim::contacts::AttributeSubKind::FamilyChild:
        return "FamilyChild";
    case bb::pim::contacts::AttributeSubKind::FamilyParent:
        return "FamilyParent";
    case bb::pim::contacts::AttributeSubKind::PersonManager:
        return "PersonManager";
    case bb::pim::contacts::AttributeSubKind::PersonAssistant:
        return "PersonAssistant";
    case bb::pim::contacts::AttributeSubKind::DateBirthday:
        return "DateBirthday";
    case bb::pim::contacts::AttributeSubKind::DateAnniversary:
        return "DateAnniversary";
    case bb::pim::contacts::AttributeSubKind::GroupDepartment:
        return "GroupDepartment";
    case bb::pim::contacts::AttributeSubKind::NameGiven:
        return "NameGiven";
    case bb::pim::contacts::AttributeSubKind::NameSurname:
        return "NameSurname";
    case bb::pim::contacts::AttributeSubKind::Title:
        return "Title";
    case bb::pim::contacts::AttributeSubKind::NameSuffix:
        return "NameSuffix";
    case bb::pim::contacts::AttributeSubKind::NameMiddle:
        return "NameMiddle";
    case bb::pim::contacts::AttributeSubKind::NameNickname:
        return "NameNickname";
    case bb::pim::contacts::AttributeSubKind::NameAlias:
        return "NameAlias";
    case bb::pim::contacts::AttributeSubKind::NameDisplayName:
        return "NameDisplayName";
    case bb::pim::contacts::AttributeSubKind::NamePhoneticGiven:
        return "NamePhoneticGiven";
    case bb::pim::contacts::AttributeSubKind::NamePhoneticSurname:
        return "NamePhoneticSurname";
    case bb::pim::contacts::AttributeSubKind::StockSymbolNyse:
        return "StockSymbolNyse";
    case bb::pim::contacts::AttributeSubKind::StockSymbolNasdaq:
        return "StockSymbolNasdaq";
    case bb::pim::contacts::AttributeSubKind::StockSymbolTse:
        return "StockSymbolTse";
    case bb::pim::contacts::AttributeSubKind::StockSymbolLse:
        return "StockSymbolLse";
    case bb::pim::contacts::AttributeSubKind::StockSymbolTsx:
        return "StockSymbolTsx";
    case bb::pim::contacts::AttributeSubKind::RankingKlout:
        return "RankingKlout";
    case bb::pim::contacts::AttributeSubKind::RankingTrstRank:
        return "RankingTrstRank";
    case bb::pim::contacts::AttributeSubKind::OrganizationAffiliationName:
        return "OrganizationAffiliationName";
    case bb::pim::contacts::AttributeSubKind::OrganizationAffiliationPhoneticName:
        return "OrganizationAffiliationPhoneticName";
    case bb::pim::contacts::AttributeSubKind::StartDate:
        return "StartDate";
    case bb::pim::contacts::AttributeSubKind::EndDate:
        return "EndDate";
    case bb::pim::contacts::AttributeSubKind::OrganizationAffiliationDetails:
        return "OrganizationAffiliationDetails";
    case bb::pim::contacts::AttributeSubKind::EducationInstitutionName:
        return "EducationInstitutionName";
    case bb::pim::contacts::AttributeSubKind::EducationDegree:
        return "EducationDegree";
    case bb::pim::contacts::AttributeSubKind::EducationConcentration:
        return "EducationConcentration";
    case bb::pim::contacts::AttributeSubKind::EducationActivities:
        return "EducationActivities";
    case bb::pim::contacts::AttributeSubKind::EducationNotes:
        return "EducationNotes";
    case bb::pim::contacts::AttributeSubKind::InstantMessagingBbmPin:
        return "InstantMessagingBbmPin";
    case bb::pim::contacts::AttributeSubKind::InstantMessagingAim:
        return "InstantMessagingAim";
    case bb::pim::contacts::AttributeSubKind::InstantMessagingAliwangwang:
        return "InstantMessagingAliwangwang";
    case bb::pim::contacts::AttributeSubKind::InstantMessagingGoogleTalk:
        return "InstantMessagingGoogleTalk";
    case bb::pim::contacts::AttributeSubKind::InstantMessagingSametime:
        return "InstantMessagingSametime";
    case bb::pim::contacts::AttributeSubKind::InstantMessagingIcq:
        return "InstantMessagingIcq";
    case bb::pim::contacts::AttributeSubKind::InstantMessagingIrc:
        return "InstantMessagingIrc";
    case bb::pim::contacts::AttributeSubKind::InstantMessagingJabber:
        return "InstantMessagingJabber";
    case bb::pim::contacts::AttributeSubKind::InstantMessagingMsLcs:
        return "InstantMessagingMsLcs";
    case bb::pim::contacts::AttributeSubKind::InstantMessagingMsn:
        return "InstantMessagingMsn";
    case bb::pim::contacts::AttributeSubKind::InstantMessagingQq:
        return "InstantMessagingQq";
    case bb::pim::contacts::AttributeSubKind::InstantMessagingSkype:
        return "InstantMessagingSkype";
    case bb::pim::contacts::AttributeSubKind::InstantMessagingYahooMessenger:
        return "InstantMessagingYahooMessenger";
    case bb::pim::contacts::AttributeSubKind::InstantMessagingYahooMessengerJapan:
        return "InstantMessagingYahooMessengerJapan";
    case bb::pim::contacts::AttributeSubKind::VideoChatBbPlaybook:
        return "VideoChatBbPlaybook";
    case bb::pim::contacts::AttributeSubKind::HiddenLinkedIn:
        return "HiddenLinkedIn";
    case bb::pim::contacts::AttributeSubKind::HiddenFacebook:
        return "HiddenFacebook";
    case bb::pim::contacts::AttributeSubKind::HiddenTwitter:
        return "HiddenTwitter";
    case bb::pim::contacts::AttributeSubKind::ConnectionCountLinkedIn:
        return "ConnectionCountLinkedIn";
    case bb::pim::contacts::AttributeSubKind::ConnectionCountFacebook:
        return "ConnectionCountFacebook";
    case bb::pim::contacts::AttributeSubKind::ConnectionCountTwitter:
        return "ConnectionCountTwitter";
    case bb::pim::contacts::AttributeSubKind::HiddenChecksum:
        return "HiddenChecksum";
    case bb::pim::contacts::AttributeSubKind::HiddenSpeedDial:
        return "HiddenSpeedDial";
    case bb::pim::contacts::AttributeSubKind::BiographyFacebook:
        return "BiographyFacebook";
    case bb::pim::contacts::AttributeSubKind::BiographyTwitter:
        return "BiographyTwitter";
    case bb::pim::contacts::AttributeSubKind::BiographyLinkedIn:
        return "BiographyLinkedIn";
    case bb::pim::contacts::AttributeSubKind::SoundRingtone:
        return "SoundRingtone";
    case bb::pim::contacts::AttributeSubKind::SimContactType:
        return "SimContactType";
    case bb::pim::contacts::AttributeSubKind::EcoID:
        return "EcoID";
    case bb::pim::contacts::AttributeSubKind::Personal:
        return "Personal";
    case bb::pim::contacts::AttributeSubKind::StockSymbolAll:
        return "StockSymbolAll";
    case bb::pim::contacts::AttributeSubKind::NotificationVibration:
        return "NotificationVibration";
    case bb::pim::contacts::AttributeSubKind::NotificationLED:
        return "NotificationLED";
    case bb::pim::contacts::AttributeSubKind::MessageNotificationVibration:
        return "MessageNotificationVibration";
    case bb::pim::contacts::AttributeSubKind::MessageNotificationLED:
        return "MessageNotificationLED";
    case bb::pim::contacts::AttributeSubKind::MessageNotificationDuringCall:
        return "MessageNotificationDuringCall";
    case bb::pim::contacts::AttributeSubKind::VideoChatPin:
        return "VideoChatPin";
    case bb::pim::contacts::AttributeSubKind::NamePrefix:
        return "NamePrefix";
    case bb::pim::contacts::AttributeSubKind::Business:
        return "Business";
    case bb::pim::contacts::AttributeSubKind::ProfileSinaWeibo:
        return "ProfileSinaWeibo";
    case bb::pim::contacts::AttributeSubKind::HiddenSinaWeibo:
        return "HiddenSinaWeibo";
    case bb::pim::contacts::AttributeSubKind::ConnectionCountSinaWeibo:
        return "ConnectionCountSinaWeibo";
    case bb::pim::contacts::AttributeSubKind::BiographySinaWeibo:
        return "BiographySinaWeibo";
    case bb::pim::contacts::AttributeSubKind::DeviceInfo:
        return "DeviceInfo";
    default:
        return QString("SubKind (%1)").arg(subKind);
    }
}
