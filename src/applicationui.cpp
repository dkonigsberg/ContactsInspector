#include "applicationui.hpp"

#include <QtCore/QList>

#include <bb/cascades/Application>
#include <bb/cascades/QmlDocument>
#include <bb/cascades/NavigationPane>
#include <bb/cascades/LocaleHandler>
#include <bb/cascades/ActionItem>
#include <bb/cascades/Menu>
#include <bb/cascades/Page>
#include <bb/cascades/Sheet>
#include <bb/cascades/ListView>
#include <bb/cascades/GroupDataModel>
#include <bb/pim/contacts/ContactService>
#include <bb/pim/contacts/Contact>
#include <bb/pim/contacts/ContactListFilters>
#include <bb/system/InvokeManager>
#include <bb/system/InvokeRequest>
#include <bb/system/SystemPrompt>
#include <bb/ApplicationInfo>

#include "contactpage.hpp"

using namespace bb::cascades;

ApplicationUI::ApplicationUI(bb::cascades::Application *app) : QObject(app), loadThread_(NULL)
{
    qRegisterMetaType<QList<bb::pim::contacts::Contact> >("QList<bb::pim::contacts::Contact>");

    translator_ = new QTranslator(this);
    localeHandler_ = new LocaleHandler(this);

    connect(localeHandler_, SIGNAL(systemLanguageChanged()), this, SLOT(onSystemLanguageChanged()));
    onSystemLanguageChanged();

    QmlDocument *qml = QmlDocument::create("asset:///main.qml").parent(this);

    navPane_ = qml->createRootObject<NavigationPane>();
    connect(navPane_, SIGNAL(popTransitionEnded(bb::cascades::Page*)),
        this, SLOT(onPopTransitionEnded(bb::cascades::Page*)));

    page_ = navPane_->findChild<Page *>("rootPage");
    listView_ = page_->findChild<ListView *>("listView");
    connect(page_, SIGNAL(refreshList()), this, SLOT(onRefreshContactsList()));
    connect(page_, SIGNAL(search()), this, SLOT(onSearch()));
    connect(page_, SIGNAL(openContact(int)), this, SLOT(onOpenContact(int)));

    dataModel_ = page_->findChild<GroupDataModel*>("dataModel");

    ActionItem *aboutItem = ActionItem::create()
        .title(tr("About"))
        .imageSource(QUrl("asset:///images/ic_info.png"))
        .onTriggered(this, SLOT(onAboutActionTriggered()));

    app->setScene(navPane_);

    app->setMenu(Menu::create().addAction(aboutItem));
    app->setMenuEnabled(true);

    page_->setProperty("activityRunning", true);
    QMetaObject::invokeMethod(this, "onRefreshContactsList", Qt::QueuedConnection);
}

void ApplicationUI::onSystemLanguageChanged()
{
    QCoreApplication::instance()->removeTranslator(translator_);
    QString locale_string = QLocale().name();
    QString file_name = QString("ContactsInspector_%1").arg(locale_string);
    if (translator_->load(file_name, "app/native/qm")) {
        QCoreApplication::instance()->installTranslator(translator_);
    }
}

void ApplicationUI::onPopTransitionEnded(bb::cascades::Page *page)
{
    if(page) {
        page->deleteLater();
    }
}

void ApplicationUI::onAboutActionTriggered()
{
    QmlDocument *qml = QmlDocument::create("asset:///AboutPage.qml").parent(this);
    if(qml->hasErrors()) { return; }

    Page *aboutPage = qml->createRootObject<Page>();
    qml->setParent(aboutPage);

    bb::ApplicationInfo appInfo;
    aboutPage->setProperty("appName", appInfo.title());
    aboutPage->setProperty("versionNumber", appInfo.version());

    Sheet *sheet = Sheet::create().content(aboutPage);
    connect(aboutPage, SIGNAL(close()), this, SLOT(onSheetPageClosed()));
    connect(aboutPage, SIGNAL(openUrl(QString)), this, SLOT(onOpenUrlInBrowser(QString)));
    sheet->open();
    Application::instance()->setMenuEnabled(false);
}

void ApplicationUI::onSheetPageClosed()
{
    Page *page = qobject_cast<Page*>(sender());
    Sheet *sheet = qobject_cast<Sheet*>(page->parent());
    sheet->close();
    sheet->deleteLater();
    Application::instance()->setMenuEnabled(true);
}

void ApplicationUI::onOpenUrlInBrowser(const QString &url) {
    bb::system::InvokeManager invokeManager;
    bb::system::InvokeRequest request;
    request.setTarget("sys.browser");
    request.setAction("bb.action.OPEN");
    request.setMimeType("text/html");
    request.setUri(url);
    invokeManager.invoke(request);
}

void ApplicationUI::onRefreshContactsList()
{
    if(loadThread_) { return; }

    dataModel_->clear();

    loadThread_ = new QThread(this);
    ContactsLoader *loader = new ContactsLoader();
    connect(loader, SIGNAL(pageLoaded(QList<bb::pim::contacts::Contact>)),
        this, SLOT(onContactsPageLoaded(QList<bb::pim::contacts::Contact>)));
    connect(loader, SIGNAL(finished()), this, SLOT(onContactsLoadFinished()));
    connect(loadThread_, SIGNAL(started()), loader, SLOT(start()));
    connect(loadThread_, SIGNAL(finished()), loadThread_, SLOT(deleteLater()));
    loader->moveToThread(loadThread_);
    loadThread_->start();
}

void ApplicationUI::onContactsPageLoaded(const QList<bb::pim::contacts::Contact> &contactsPage)
{
    foreach(const bb::pim::contacts::Contact &contact, contactsPage) {
        if(!contact.isValid()) { continue; }
        QVariantMap map;
        map["displayName"] = contact.displayName();
        map["displayCompanyName"] = contact.displayCompanyName();
        map["contactId"] = contact.id();
        if(!contact.smallPhotoFilepath().isEmpty()) {
            map["photo"] = QLatin1String("file://") + contact.smallPhotoFilepath();
        }
        dataModel_->insert(map);
    }
}

void ApplicationUI::onContactsLoadFinished()
{
    page_->setProperty("activityRunning", false);
    loadThread_->quit();
    loadThread_ = NULL;
}

void ApplicationUI::onSearch()
{
    bb::system::SystemPrompt *prompt = new bb::system::SystemPrompt(this);
    connect(prompt, SIGNAL(finished(bb::system::SystemUiResult::Type)),
        this, SLOT(onSearchPromptFinished(bb::system::SystemUiResult::Type)));
    prompt->setTitle(tr("Search"));
    prompt->show();
}

void ApplicationUI::onSearchPromptFinished(bb::system::SystemUiResult::Type result)
{
    bb::system::SystemPrompt *prompt = qobject_cast<bb::system::SystemPrompt *>(sender());
    prompt->deleteLater();
    if(result == bb::system::SystemUiResult::ConfirmButtonSelection) {
        QVariantList indexPath;
        const QString text = prompt->inputFieldTextEntry().trimmed();
        if(!text.isEmpty()) {
            foreach(const QVariantMap& map, dataModel_->toListOfMaps()) {
                if(map["displayName"].toString().contains(text, Qt::CaseInsensitive)
                    || map["displayCompanyName"].toString().contains(text, Qt::CaseInsensitive)
                    || map["contactId"].toString() == text) {
                    indexPath = dataModel_->findExact(map);
                    break;
                }
            }
        }

        if(!indexPath.isEmpty()) {
            listView_->scrollToItem(indexPath);
        }
    }
}

void ApplicationUI::onOpenContact(int contactId)
{
    ContactPage *contactPage = new ContactPage(contactId, this);
    contactPage->push(navPane_);
}

ContactsLoader::ContactsLoader(QObject *parent) : QObject(parent)
{
}

void ContactsLoader::start()
{
    bb::pim::contacts::ContactService contactService;

    const int maxLimit = 200;
    bb::pim::contacts::ContactListFilters options;
    options.setLimit(maxLimit);
    options.setSortBy(bb::pim::contacts::SortColumn::FirstName, bb::pim::contacts::SortOrder::Ascending);

    do {
        QList<bb::pim::contacts::Contact> contactsPage = contactService.contacts(options);
        emit pageLoaded(contactsPage);
        if (contactsPage.size() == maxLimit) {
            options.setAnchorId(contactsPage[maxLimit - 1].id());
        }
        else {
            break;
        }
    } while (true);

    emit finished();
}
