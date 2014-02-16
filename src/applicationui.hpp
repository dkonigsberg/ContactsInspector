#ifndef APPLICATIONUI_HPP
#define APPLICATIONUI_HPP

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QList>

#include <bb/pim/contacts/Contact>
#include <bb/system/SystemUiResult>

namespace bb { namespace cascades {
class Application;
class LocaleHandler;
class NavigationPane;
class Page;
class ListView;
class GroupDataModel;
}}

class QTranslator;

class ApplicationUI : public QObject
{
    Q_OBJECT
public:
    ApplicationUI(bb::cascades::Application *app);
    virtual ~ApplicationUI() { }
private slots:
    void onSystemLanguageChanged();
    void onPopTransitionEnded(bb::cascades::Page *page);
    void onAboutActionTriggered();
    void onSheetPageClosed();
    void onOpenUrlInBrowser(const QString &url);
    void onRefreshContactsList();
    void onContactsPageLoaded(const QList<bb::pim::contacts::Contact> &contactsPage);
    void onContactsLoadFinished();
    void onSearch();
    void onSearchPromptFinished(bb::system::SystemUiResult::Type result);
    void onOpenContact(int contactId);
private:
    QTranslator *translator_;
    bb::cascades::LocaleHandler *localeHandler_;
    bb::cascades::NavigationPane *navPane_;
    bb::cascades::Page *page_;
    bb::cascades::ListView *listView_;
    bb::cascades::GroupDataModel *dataModel_;
    QThread *loadThread_;
};

class ContactsLoader : public QObject
{
    Q_OBJECT
public:
    ContactsLoader(QObject *parent=0);
    virtual ~ContactsLoader() { }
public slots:
    void start();
signals:
    void pageLoaded(const QList<bb::pim::contacts::Contact> &contactsPage);
    void finished();
};

#endif // APPLICATIONUI_HPP
