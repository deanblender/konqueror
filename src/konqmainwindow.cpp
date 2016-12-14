/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>
   Copyright (C) 2000-2005 David Faure <faure@kde.org>
   Copyright (C) 2007 Eduardo Robles Elvira <edulix@gmail.com>
   Copyright (C) 2007 Daniel García Moreno <danigm@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqmainwindow.h"
#include "konqmouseeventfilter.h"
#include "konqclosedwindowsmanager.h"
#include "konqsessionmanager.h"
#include "konqsessiondlg.h"
#include "konqdraggablelabel.h"
#include "konqcloseditem.h"
#include "konqapplication.h"
#include "konqguiclients.h"
#include "konqmainwindowfactory.h"
#include "KonqMainWindowAdaptor.h"
#include "KonquerorAdaptor.h"
#include "konqview.h"
#include "konqrun.h"
#include "konqmisc.h"
#include "konqviewmanager.h"
#include "konqframestatusbar.h"
#include "konqtabs.h"
#include "konqactions.h"
#include "konqsettingsxt.h"
#include "konqextensionmanager.h"
#include "konqueror_interface.h"
#include "delayedinitializer.h"
#include "konqextendedbookmarkowner.h"
#include "konqframevisitor.h"
#include "konqbookmarkbar.h"
#include "konqundomanager.h"
#include "konqhistorydialog.h"
#include <config-konqueror.h>
#include <kstringhandler.h>

#include <konq_events.h>
#include <konqpixmapprovider.h>
#include <kbookmarkmanager.h>
#include <kinputdialog.h>
#include <kcomponentdata.h>
#include <klineedit.h>
#include <kzip.h>
#include <pwd.h>
// we define STRICT_ANSI to get rid of some warnings in glibc
#ifndef __STRICT_ANSI__
#define __STRICT_ANSI__
#define _WE_DEFINED_IT_
#endif
#include <netdb.h>
#ifdef _WE_DEFINED_IT_
#undef __STRICT_ANSI__
#undef _WE_DEFINED_IT_
#endif
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <kde_file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QDesktopServices>
#include <QFile>
#include <QClipboard>
#include <QStackedWidget>
#include <QFileInfo>
#if KONQ_HAVE_X11
#include <QX11Info>
#endif
#include <QtCore/QEvent>
#include <QKeyEvent>
#include <QtCore/QByteRef>
#include <QtCore/QList>
#include <QPixmap>
#include <QLineEdit>

#include <kaboutdata.h>
#include <ktoolbar.h>
#include <konqbookmarkmenu.h>
#include <kcmultidialog.h>
#include <QDebug>
#include <kdesktopfile.h>
#include <kedittoolbar.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kmessagebox_queued.h>
#include <knewfilemenu.h>
#include <konq_popupmenu.h>
#include "konqsettings.h"
#include "konqanimatedlogo_p.h"
#include <kprotocolinfo.h>
#include <kprotocolmanager.h>
#include <kstandardshortcut.h>
#include <kstandardaction.h>
#include <kstandarddirs.h>
#include <ksycoca.h>
#include <QTemporaryFile>
#include <ktogglefullscreenaction.h>
#include <ktoolbarpopupaction.h>
#include <kurlcompletion.h>
#include <kurlrequesterdialog.h>
#include <kurlrequester.h>
#include <kmimetypetrader.h>
#include <kwindowsystem.h>
#include <KJobWidgets>
#include <kfiledialog.h>
#include <KLocalizedString>
#include <QIcon>
#include <kiconloader.h>
#include <QMenu>
#include <kprocess.h>
#include <kio/scheduler.h>
#include <kio/netaccess.h>
#include <KIO/JobUiDelegate>
#include <KIO/CopyJob>
#include <KIO/Job>
#include <KIO/FileUndoManager>
#include <kparts/browseropenorsavequestion.h>
#include <KParts/OpenUrlEvent>
#include <KParts/BrowserHostExtension>
#include <KCompletionMatches>
#include <kacceleratormanager.h>
#include <kuser.h>
#include <kxmlguifactory.h>
#include <netwm.h>
#include <sonnet/configdialog.h>

#include <kauthorized.h>
#include <QtDBus/QtDBus>
#include <kconfiggroup.h>
#include <kglobalsettings.h>
#include <kurlauthorized.h>
#include <QFontDatabase>
#include <QMenuBar>
#include <QStandardPaths>
#include <KSharedConfig>

template class QList<QPixmap *>;
template class QList<KToggleAction *>;

static KBookmarkManager *s_bookmarkManager = 0;
QList<KonqMainWindow *> *KonqMainWindow::s_lstViews = 0;
KConfig *KonqMainWindow::s_comboConfig = 0;
KCompletion *KonqMainWindow::s_pCompletion = 0;

KonqOpenURLRequest KonqOpenURLRequest::null;

static const unsigned short int s_closedItemsListLength = 10;

static void raiseWindow(KonqMainWindow *window)
{
    if (!window) {
        return;
    }

    if (window->isMinimized()) {
        KWindowSystem::unminimizeWindow(window->winId());
    }
    window->activateWindow();
    window->raise();
}

KonqExtendedBookmarkOwner::KonqExtendedBookmarkOwner(KonqMainWindow *w)
{
    m_pKonqMainWindow = w;
}

KonqMainWindow::KonqMainWindow(const QUrl &initialURL)
    : KParts::MainWindow()
    , m_paClosedItems(0)
    , m_fullyConstructed(false)
    , m_bLocationBarConnected(false)
    , m_bURLEnterLock(false)
    , m_urlCompletionStarted(false)
    , m_prevMenuBarVisible(true)
    , m_goBuffer(0)
    , m_pBookmarkMenu(0)
    , m_configureDialog(0)
    , m_pURLCompletion(0)
    , m_isPopupWithProxyWindow(false)
{
    if (!s_lstViews) {
        s_lstViews = new QList<KonqMainWindow *>;
    }

    s_lstViews->append(this);

    KonqMouseEventFilter::self(); // create it

    m_pChildFrame = 0;
    m_pActiveChild = 0;
    m_workingTab = 0;
    (void) new KonqMainWindowAdaptor(this);
    m_paBookmarkBar = 0;

    m_viewModesGroup = new QActionGroup(this);
    m_viewModesGroup->setExclusive(true);
    connect(m_viewModesGroup, SIGNAL(triggered(QAction*)),
            this, SLOT(slotViewModeTriggered(QAction*)),
            Qt::QueuedConnection); // Queued so that we don't delete the action from the code that triggered it.

    // This has to be called before any action is created for this mainwindow
    setComponentData(KComponentData::mainComponent(), false /*don't load plugins yet*/);

    m_pViewManager = new KonqViewManager(this);

    m_viewModeMenu = 0;
    m_openWithMenu = 0;
    m_paCopyFiles = 0;
    m_paMoveFiles = 0;
    m_bookmarkBarInitialized = false;

    m_toggleViewGUIClient = new ToggleViewGUIClient(this);

    m_pBookmarksOwner = new KonqExtendedBookmarkOwner(this);

    // init history-manager, load history, get completion object
    if (!s_pCompletion) {
        s_bookmarkManager = KBookmarkManager::userBookmarksManager();

        // let the KBookmarkManager know that we are a browser, equals to "keditbookmarks --browser"
        s_bookmarkManager->setEditorOptions(QStringLiteral("konqueror"), true);

        KonqHistoryManager *mgr = new KonqHistoryManager(s_bookmarkManager);
        s_pCompletion = mgr->completionObject();

        // setup the completion object before createGUI(), so that the combo
        // picks up the correct mode from the HistoryManager (in slotComboPlugged)
        int mode = KonqSettings::settingsCompletionMode();
        s_pCompletion->setCompletionMode(static_cast<KCompletion::CompletionMode>(mode));
    }
    connect(KParts::HistoryProvider::self(), &KParts::HistoryProvider::cleared, this, &KonqMainWindow::slotClearComboHistory);

    KonqPixmapProvider *prov = KonqPixmapProvider::self();
    if (!s_comboConfig) {
        s_comboConfig = new KConfig(QStringLiteral("konq_history"), KConfig::NoGlobals);
        KonqCombo::setConfig(s_comboConfig);
        KConfigGroup locationBarGroup(s_comboConfig, "Location Bar");
        prov->load(locationBarGroup, QStringLiteral("ComboIconCache"));
    }

    connect(prov, SIGNAL(changed()), SLOT(slotIconsChanged()));

    m_pUndoManager = new KonqUndoManager(this);
    connect(m_pUndoManager, SIGNAL(undoAvailable(bool)),
            this, SLOT(slotUndoAvailable(bool)));

    initCombo();
    initActions();

    connect(KGlobalSettings::self(), &KGlobalSettings::kdisplayFontChanged, this, &KonqMainWindow::slotReconfigure);

    setXMLFile(QStringLiteral("konqueror.rc"));

    setStandardToolBarMenuEnabled(true);

    createGUI(Q_NULLPTR);

    m_combo->setParent(toolBar(QStringLiteral("locationToolBar")));
    m_combo->setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
    m_combo->show();

    checkDisableClearButton();

    connect(toolBarMenuAction(), SIGNAL(triggered()), this, SLOT(slotForceSaveMainWindowSettings()));

    if (!m_toggleViewGUIClient->empty()) {
        plugActionList(QStringLiteral("toggleview"), m_toggleViewGUIClient->actions());
    } else {
        delete m_toggleViewGUIClient;
        m_toggleViewGUIClient = 0;
    }

    m_bNeedApplyKonqMainWindowSettings = true;

    if (!initialURL.isEmpty()) {
        openFilteredUrl(initialURL.url());
    } else {
        // silent
        m_bNeedApplyKonqMainWindowSettings = false;
    }

    resize(700, 480);

    //qDebug() << this << "created";

    KonqSessionManager::self();
    m_fullyConstructed = true;
}

KonqMainWindow::~KonqMainWindow()
{
    //qDebug() << this;

    delete m_pViewManager;
    m_pViewManager = 0;

    if (s_lstViews) {
        s_lstViews->removeAll(this);
        if (s_lstViews->isEmpty()) {
            delete s_lstViews;
            s_lstViews = 0;
        }
    }

    qDeleteAll(m_openWithActions);
    m_openWithActions.clear();

    delete m_pBookmarkMenu;
    delete m_paBookmarkBar;
    delete m_pBookmarksOwner;
    delete m_pURLCompletion;
    delete m_paClosedItems;

    if (s_lstViews == 0) {
        delete s_comboConfig;
        s_comboConfig = 0;
    }

    delete m_configureDialog;
    m_configureDialog = 0;
    delete m_combo;
    m_combo = 0;
    delete m_locationLabel;
    m_locationLabel = 0;
    m_pUndoManager->disconnect();
    delete m_pUndoManager;

    //qDebug() << this << "deleted";
}

QWidget *KonqMainWindow::createContainer(QWidget *parent, int index, const QDomElement &element, QAction *&containerAction)
{
    QWidget *res = KParts::MainWindow::createContainer(parent, index, element, containerAction);

    static QString nameBookmarkBar = QStringLiteral("bookmarkToolBar");
    static QString tagToolBar = QStringLiteral("ToolBar");
    if (res && (element.tagName() == tagToolBar) && (element.attribute(QStringLiteral("name")) == nameBookmarkBar)) {
        Q_ASSERT(::qobject_cast<KToolBar *>(res));
        if (!KAuthorized::authorizeAction(QStringLiteral("bookmarks"))) {
            delete res;
            return 0;
        }

        if (!m_bookmarkBarInitialized) {
            // The actual menu needs a different action collection, so that the bookmarks
            // don't appear in kedittoolbar
            m_bookmarkBarInitialized = true;
            DelayedInitializer *initializer = new DelayedInitializer(QEvent::Show, res);
            connect(initializer, &DelayedInitializer::initialize, this, &KonqMainWindow::initBookmarkBar);
        }
    }

    if (res && element.tagName() == QLatin1String("Menu")) {
        const QString &menuName = element.attribute(QStringLiteral("name"));
        if (menuName == QLatin1String("edit") || menuName == QLatin1String("tools")) {
            Q_ASSERT(qobject_cast<QMenu *>(res));
            KAcceleratorManager::manage(static_cast<QMenu *>(res));
        }
    }

    return res;
}

void KonqMainWindow::initBookmarkBar()
{
    KToolBar *bar = qFindChild<KToolBar *>(this, QStringLiteral("bookmarkToolBar"));

    if (!bar) {
        return;
    }

    const bool wasVisible = bar->isVisible();

    delete m_paBookmarkBar;
    m_paBookmarkBar = new KBookmarkBar(s_bookmarkManager, m_pBookmarksOwner, bar, this);

    // hide if empty
    if (bar->actions().count() == 0 || !wasVisible) {
        bar->hide();
    }
}

void KonqMainWindow::removeContainer(QWidget *container, QWidget *parent, QDomElement &element, QAction *containerAction)
{
    static QString nameBookmarkBar = QStringLiteral("bookmarkToolBar");
    static QString tagToolBar = QStringLiteral("ToolBar");

    if (element.tagName() == tagToolBar && element.attribute(QStringLiteral("name")) == nameBookmarkBar) {
        Q_ASSERT(::qobject_cast<KToolBar *>(container));
        if (m_paBookmarkBar) {
            m_paBookmarkBar->clear();
        }
    }

    KParts::MainWindow::removeContainer(container, parent, element, containerAction);
}

// Detect a name filter (e.g. *.txt) in the url.
// Note that KShortURIFilter does the same, but we have no way of getting it from there
//
// Note: this removes the filter from the URL.
QString KonqMainWindow::detectNameFilter(QUrl &url)
{
    if (!KProtocolManager::supportsListing(url)) {
        return QString();
    }

    // Look for wildcard selection
    QString nameFilter;
    QString path = url.path();
    int lastSlash = path.lastIndexOf('/');
    if (lastSlash > -1) {
        if (!url.query().isEmpty() && lastSlash == path.length() - 1) {  //  In /tmp/?foo, foo isn't a query
            path += url.query(); // includes the '?'
        }
        QString fileName = path.mid(lastSlash + 1);
        if (fileName.indexOf('*') != -1 || fileName.indexOf('[') != -1 || fileName.indexOf('?') != -1) {
            // Check that a file or dir with all the special chars in the filename doesn't exist
            // (NetAccess::exists has a fast path for local files)
            if (!KIO::NetAccess::exists(url, KIO::NetAccess::DestinationSide, this)) {
                nameFilter = fileName;
                url = url.adjusted(QUrl::RemoveFilename | QUrl::RemoveQuery);
                qDebug() << "Found wildcard. nameFilter=" << nameFilter << "  New url=" << url;
            }
        }
    }

    return nameFilter;
}

void KonqMainWindow::openFilteredUrl(const QString &url, const KonqOpenURLRequest &req)
{
    // Filter URL to build a correct one
    if (m_currentDir.isEmpty() && m_currentView) {
        m_currentDir = m_currentView->url();
    }

    QUrl filteredURL(KonqMisc::konqFilteredURL(this, url, m_currentDir));
    qDebug() << "url" << url << "filtered into" << filteredURL;

    if (filteredURL.isEmpty()) { // initially empty, or error (e.g. ~unknown_user)
        return;
    }

    m_currentDir.clear();

    openUrl(0, filteredURL, QString(), req);

    // #4070: Give focus to view after URL was entered manually
    // Note: we do it here if the view mode (i.e. part) wasn't changed
    // If it is changed, then it's done in KonqViewManager::doSetActivePart
    if (m_currentView) {
        m_currentView->setFocus();
    }
}

void KonqMainWindow::openFilteredUrl(const QString &_url, bool inNewTab, bool tempFile)
{
    KonqOpenURLRequest req(_url);
    req.browserArgs.setNewTab(inNewTab);
    req.newTabInFront = true;
    req.tempFile = tempFile;

    openFilteredUrl(_url, req);
}

void KonqMainWindow::openFilteredUrl(const QString &_url,  const QString &_mimeType, bool inNewTab, bool tempFile)
{
    KonqOpenURLRequest req(_url);
    req.browserArgs.setNewTab(inNewTab);
    req.newTabInFront = true;
    req.tempFile = tempFile;
    req.args.setMimeType(_mimeType);

    openFilteredUrl(_url, req);
}

void KonqMainWindow::openUrl(KonqView *_view, const QUrl &_url,
                             const QString &_mimeType, const KonqOpenURLRequest &_req,
                             bool trustedSource)
{
#ifndef NDEBUG // needed for req.debug()
    qDebug() << "url=" << _url << "mimeType=" << _mimeType
             << "_req=" << _req.debug() << "view=" << _view;
#endif

    // We like modifying args in this method :)
    QUrl url(_url);
    QString mimeType(_mimeType);
    KonqOpenURLRequest req(_req);

    if (mimeType.isEmpty()) {
        mimeType = req.args.mimeType();
    }

    if (url.scheme() != QLatin1String("error") && url.scheme() != QLatin1String("about")) {
        if (!url.isValid()) {
            // I think we can't really get here anymore; I tried and didn't succeed.
            // URL filtering catches this case before hand, and in cases without filtering
            // (e.g. HTML link), the url is empty here, not invalid.
            // But just to be safe, let's keep this code path
            url = KParts::BrowserRun::makeErrorUrl(KIO::ERR_MALFORMED_URL, url.url(), url);
        } else if (!KProtocolInfo::isKnownProtocol(url)) {
            url = KParts::BrowserRun::makeErrorUrl(KIO::ERR_UNSUPPORTED_PROTOCOL, url.scheme(), url);
        }
    }

    if (url.url() == QLatin1String("about:blank") || url.scheme() == QLatin1String("error")) {
        mimeType = QStringLiteral("text/html");
    }

    const QString nameFilter = detectNameFilter(url);
    if (!nameFilter.isEmpty()) {
        req.nameFilter = nameFilter;
        url = url.adjusted(QUrl::RemoveFilename);
    }

    QLineEdit *edit = comboEdit();
    if (edit) {
        edit->setModified(false);
    }

    KonqView *view = _view;

    // When clicking a 'follow active' view (e.g. view is the sidebar),
    // open the URL in the active view
    if (view && view->isFollowActive()) {
        view = m_currentView;
    }

    if (!view && !req.browserArgs.newTab()) {
        view = m_currentView;    /* Note, this can be 0, e.g. on startup */
    } else if (!view && req.browserArgs.newTab()) {

        // The URL should be opened in a new tab. Let's create the tab right away,
        // it gives faster user feedback (#163628). For a short while (kde-4.1-beta1)
        // I removed this entire block so that we wouldn't end up with a useless tab when
        // launching an external application for this mimetype. But user feedback
        // in all cases is more important than empty tabs in some cases.
        view = m_pViewManager->addTab(QStringLiteral("text/html"),
                                      QString(),
                                      false,
                                      req.openAfterCurrentPage);
        if (view) {
            view->setCaption(i18nc("@title:tab", "Loading..."));
            view->setLocationBarURL(_url);
            if (!req.browserArgs.frameName.isEmpty()) {
                view->setViewName(req.browserArgs.frameName);    // #44961
            }

            if (req.newTabInFront) {
                m_pViewManager->showTab(view);
            }

            updateViewActions(); //A new tab created -- we may need to enable the "remove tab" button (#56318)
        } else {
            req.browserArgs.setNewTab(false);
        }
    }

    const QString oldLocationBarURL = locationBarURL();
    if (view) {
        if (view == m_currentView) {
            //will do all the stuff below plus GUI stuff
            abortLoading();
        } else {
            view->stop();
            // Don't change location bar if not current view
        }
    }

    // Fast mode for local files: do the stat ourselves instead of letting KRun do it.
    if (mimeType.isEmpty() && url.isLocalFile()) {
        QMimeDatabase db;
        mimeType = db.mimeTypeForFile(url.toLocalFile()).name();
    }

    if (url.isLocalFile()) {
        // Generic mechanism for redirecting to tar:/<path>/ when clicking on a tar file,
        // zip:/<path>/ when clicking on a zip file, etc.
        // The .protocol file specifies the mimetype that the kioslave handles.
        // Note that we don't use mimetype inheritance since we don't want to
        // open OpenDocument files as zip folders...
        // Also note that we do this here and not in openView anymore,
        // because in the case of foo.bz2 we don't know the final mimetype, we need a konqrun...
        const QString protocol = KProtocolManager::protocolForArchiveMimetype(mimeType);
        if (!protocol.isEmpty() && KonqFMSettings::settings()->shouldEmbed(mimeType)) {
            url.setScheme(protocol);
            if (mimeType == QLatin1String("application/x-webarchive")) {
                url.setPath(url.path() + "/index.html");
                mimeType = QStringLiteral("text/html");
            } else {
                if (KProtocolManager::outputType(url) == KProtocolInfo::T_FILESYSTEM) {
                    if (!url.path().endsWith('/')) {
                        url.setPath(url.path() + '/');
                    }
                    mimeType = QStringLiteral("inode/directory");
                } else {
                    mimeType.clear();
                }
            }
        }

        // Redirect to the url in Type=Link desktop files
        if (mimeType == QLatin1String("application/x-desktop")) {
            KDesktopFile df(url.toLocalFile());
            if (df.hasLinkType()) {
                url = QUrl(df.readUrl());
                mimeType.clear(); // to be determined again
            }
        }
    }

    const bool hasMimeType = (!mimeType.isEmpty() && mimeType != QLatin1String("application/octet-stream"));
    KService::Ptr offer;
    bool associatedAppIsKonqueror = false;
    if (hasMimeType) {
        offer = KMimeTypeTrader::self()->preferredService(mimeType, QStringLiteral("Application"));
        associatedAppIsKonqueror = isMimeTypeAssociatedWithSelf(mimeType, offer);
        // If the associated app is konqueror itself, then make sure we try to embed before bailing out.
        if (associatedAppIsKonqueror) {
            req.forceAutoEmbed = true;
        }
    }

    //qDebug() << "trying openView for" << url << "( mimeType" << mimeType << ")";
    if (hasMimeType || url.url() == QLatin1String("about:") || url.url().startsWith(QLatin1String("about:konqueror")) || url.url() == QLatin1String("about:plugins")) {

        // Built-in view ?
        if (!openView(mimeType, url, view /* can be 0 */, req)) {
            //qDebug() << "openView returned false";
            // Are we following another view ? Then forget about this URL. Otherwise fire app.
            if (!req.followMode) {
                //qDebug() << "we were not following. Fire app.";
                // The logic below is similar to BrowserRun::handleNonEmbeddable(),
                // but we don't have a BrowserRun instance here, and since it uses
                // some virtual methods [like save, for KHTMLRun], we can't just
                // move all the logic to static methods... catch 22...

                if (!url.isLocalFile() && !trustedSource && KonqRun::isTextExecutable(mimeType)) {
                    mimeType = QStringLiteral("text/plain");    // view, don't execute
                }
                // Remote URL: save or open ?
                QString protClass = KProtocolInfo::protocolClass(url.scheme());
                bool open = url.isLocalFile() || protClass == QLatin1String(":local") || KProtocolInfo::isHelperProtocol(url);
                if (!open) {
                    KParts::BrowserOpenOrSaveQuestion dlg(this, url, mimeType);
                    dlg.setFeatures(KParts::BrowserOpenOrSaveQuestion::ServiceSelection);
                    const KParts::BrowserOpenOrSaveQuestion::Result res = dlg.askOpenOrSave();
                    if (res == KParts::BrowserOpenOrSaveQuestion::Save) {
                        KParts::BrowserRun::saveUrl(url, QString(), this, req.args);
                    }
                    open = (res == KParts::BrowserOpenOrSaveQuestion::Open);
                    if (open) {
                        offer = dlg.selectedService();
                    }
                }
                if (open) {
                    if (associatedAppIsKonqueror && refuseExecutingKonqueror(mimeType)) {
                        return;
                    }
                    QList<QUrl> lst;
                    lst.append(url);
                    //qDebug() << "Got offer" << (offer ? offer->name() : QString("0"));
                    const bool allowExecution = trustedSource || KParts::BrowserRun::allowExecution(mimeType, url);
                    if (allowExecution) {
                        const bool isExecutable = KonqRun::isExecutable(mimeType);
                        // Open with no offer means the user clicked on "Open With..." button.
                        if (!offer && !isExecutable) {
                            (void) KRun::displayOpenWithDialog(lst, this);
                        } else if (isExecutable || !KRun::runApplication(*offer, lst, this)) {
                            setLocationBarURL(oldLocationBarURL);   // Revert to previous locationbar URL
                            (void)new KRun(url, this);
                        }
                    }
                }
            }
        }
    } else { // no known mimeType, use KonqRun
        bool earlySetLocationBarURL = false;
        if (!view && !m_currentView) { // no view yet, e.g. starting with url as argument
            earlySetLocationBarURL = true;
        } else if (view == m_currentView && view->url().isEmpty()) { // opening in current view
            earlySetLocationBarURL = true;
        }
        if (req.browserArgs.newTab()) { // it's going into a new tab anyway
            earlySetLocationBarURL = false;
        }
        if (earlySetLocationBarURL) {
            // Show it for now in the location bar, but we'll need to store it in the view
            // later on (can't do it yet since either view == 0 or updateHistoryEntry will be called).
            qDebug() << "url=" << url;
            setLocationBarURL(url);
        }

        qDebug() << "Creating new konqrun for" << url << "req.typedUrl=" << req.typedUrl;

        KonqRun *run = new KonqRun(this, view /* can be 0 */, url, req, trustedSource);

        // Never start in external browser
        run->setEnableExternalBrowser(false);

        if (view) {
            view->setRun(run);
        }

        if (view == m_currentView) {
            startAnimation();
        }

        connect(run, &KonqRun::finished, this, &KonqMainWindow::slotRunFinished);
    }
}

// When opening a new view, for @p mimeType, prefer the part used in @p currentView, if possible.
// Testcase: window.open after manually switching to another web engine, and with "open popups in tabs" off.
static QString preferredService(KonqView *currentView, const QString &mimeType)
{
    if (currentView && !mimeType.isEmpty() && currentView->supportsMimeType(mimeType)) {
        return currentView->service()->desktopEntryName();
    }
    return QString();
}

bool KonqMainWindow::openView(QString mimeType, const QUrl &_url, KonqView *childView, const KonqOpenURLRequest &req)
{
    // Second argument is referring URL
    if (!KUrlAuthorized::authorizeUrlAction(QStringLiteral("open"), childView ? childView->url() : QUrl(), _url)) {
        QString msg = KIO::buildErrorString(KIO::ERR_ACCESS_DENIED, _url.toDisplayString());
        QMessageBox::warning(this, i18n("Access denied"), msg);
        return true; // Nothing else to do.
    }

    if (KonqRun::isExecutable(mimeType)) {
        return false;    // execute, don't open
    }
    // Contract: the caller of this method should ensure the view is stopped first.

#ifndef NDEBUG
    qDebug() << mimeType << _url << "childView=" << childView << "req:" << req.debug();
#endif
    bool bOthersFollowed = false;

    if (childView) {
        // If we're not already following another view (and if we are not reloading)
        if (!req.followMode && !req.args.reload() && !m_pViewManager->isLoadingProfile()) {
            // When clicking a 'follow active' view (e.g. childView is the sidebar),
            // open the URL in the active view
            // (it won't do anything itself, since it's locked to its location)
            if (childView->isFollowActive() && childView != m_currentView) {
                abortLoading();
                setLocationBarURL(_url);
                KonqOpenURLRequest newreq;
                newreq.forceAutoEmbed = true;
                newreq.followMode = true;
                newreq.args = req.args;
                newreq.browserArgs = req.browserArgs;
                bOthersFollowed = openView(mimeType, _url, m_currentView, newreq);
            }
            // "link views" feature, and "sidebar follows active view" feature
            bOthersFollowed = makeViewsFollow(_url, req.args, req.browserArgs, mimeType, childView) || bOthersFollowed;
        }
        if (childView->isLockedLocation() && !req.args.reload() /* allow to reload a locked view*/) {
            return bOthersFollowed;
        }
    }

    QUrl url(_url);

    // In case we open an index.html, we want the location bar
    // to still display the original URL (so that 'up' uses that URL,
    // and since that's what the user entered).
    // changePart will take care of setting and storing that url.
    QString originalURL = url.toDisplayString(QUrl::PreferLocalFile);
    if (!req.nameFilter.isEmpty()) { // keep filter in location bar
        if (!originalURL.endsWith('/')) {
            originalURL += '/';
        }
        originalURL += req.nameFilter;
    }

    QString serviceName = req.serviceName; // default: none provided
    const QString urlStr = url.url();
    if (urlStr == QLatin1String("about:") || urlStr.startsWith(QLatin1String("about:konqueror")) || urlStr == QLatin1String("about:plugins")) {
        mimeType = QStringLiteral("KonqAboutPage"); // not KParts/ReadOnlyPart, it fills the Location menu ! :)
        serviceName = QStringLiteral("konq_aboutpage");
        originalURL = req.typedUrl.isEmpty() ? QString() : req.typedUrl;
    } else if (urlStr == QLatin1String("about:blank") && req.typedUrl.isEmpty()) {
        originalURL.clear();
    }

    bool forceAutoEmbed = req.forceAutoEmbed || req.userRequestedReload;
    if (!req.typedUrl.isEmpty()) { // the user _typed_ the URL, he wants it in Konq.
        forceAutoEmbed = true;
    }
    if (url.scheme() == QLatin1String("about") || url.scheme() == QLatin1String("error")) {
        forceAutoEmbed = true;
    }
    // Related to KonqFactory::createView
    if (!forceAutoEmbed && !KonqFMSettings::settings()->shouldEmbed(mimeType)) {
        qDebug() << "KonqFMSettings says: don't embed this servicetype";
        return false;
    }
    // Do we even have a part to embed? Otherwise don't ask, since we'd ask twice.
    if (!forceAutoEmbed) {
        KService::List partServiceOffers;
        KonqFactory::getOffers(mimeType, &partServiceOffers);
        if (partServiceOffers.isEmpty()) {
            qDebug() << "No part available for" << mimeType;
            return false;
        }
    }

    // If the protocol doesn't support writing (e.g. HTTP) then we might want to save instead of just embedding.
    // So (if embedding would succeed, hence the checks above) we ask the user
    // Otherwise the user will get asked 'open or save' in openUrl anyway.
    if (!forceAutoEmbed && !KProtocolManager::supportsWriting(url)) {
        QString suggestedFileName;
        KonqRun *run = childView ? childView->run() : 0;
        int attachment = 0;
        if (run) {
            suggestedFileName = run->suggestedFileName();
            attachment = (run->serverSuggestsSave()) ? KParts::BrowserRun::AttachmentDisposition : KParts::BrowserRun::InlineDisposition;
        }

        KParts::BrowserOpenOrSaveQuestion dlg(this, url, mimeType);
        dlg.setSuggestedFileName(suggestedFileName);
        const KParts::BrowserOpenOrSaveQuestion::Result res = dlg.askEmbedOrSave(attachment);
        if (res == KParts::BrowserOpenOrSaveQuestion::Embed) {
            forceAutoEmbed = true;
        } else if (res == KParts::BrowserOpenOrSaveQuestion::Cancel) {
            return true;    // handled, don't do anything else
        } else { // Save
            KParts::BrowserRun::saveUrl(url, suggestedFileName, this, req.args);
            return true; // handled
        }
    }

    bool ok = true;
    if (!childView) {
        if (req.browserArgs.newTab()) {
            KonqFrameTabs *tabContainer = m_pViewManager->tabContainer();
            int index = tabContainer->currentIndex();
            childView = m_pViewManager->addTab(mimeType, serviceName, false, req.openAfterCurrentPage);

            if (req.newTabInFront && childView) {
                if (req.openAfterCurrentPage) {
                    tabContainer->setCurrentIndex(index + 1);
                } else {
                    tabContainer->setCurrentIndex(tabContainer->count() - 1);
                }
            }
        }

        else {
            // Create a new view
            // createFirstView always uses force auto-embed even if user setting is "separate viewer",
            // since this window has no view yet - we don't want to keep an empty mainwindow.
            // This can happen with e.g. application/pdf from a target="_blank" link, or window.open.
            childView = m_pViewManager->createFirstView(mimeType, serviceName);

            if (childView) {
                enableAllActions(true);
                m_currentView = childView;
            }
        }

        if (!childView) {
            return false;    // It didn't work out.
        }

        childView->setViewName(m_initialFrameName.isEmpty() ? req.browserArgs.frameName : m_initialFrameName);
        m_initialFrameName.clear();
    } else { // We know the child view
        if (!childView->isLockedViewMode()) {
            if (ok) {

                // When typing a new URL, the current context doesn't matter anymore
                // -> select the preferred part for a given mimetype (even if the current part can handle this mimetype).
                // This fixes the "get katepart and then type a website URL -> loaded into katepart" problem
                // (first fixed in r168902 from 2002!, see also unittest KonqHtmlTest::textThenHtml())

                if (!req.typedUrl.isEmpty() || !serviceName.isEmpty()) {
                    if (childView->isLoading()) { // Stop the view first, #282641.
                        childView->stop();
                    }
                    ok = childView->changePart(mimeType, serviceName, forceAutoEmbed);
                } else {
                    ok = childView->ensureViewSupports(mimeType, forceAutoEmbed);
                }
            }
        }
    }

    if (ok) {
        //qDebug() << "req.nameFilter= " << req.nameFilter;
        //qDebug() << "req.typedUrl= " << req.typedUrl;
        //qDebug() << "Browser extension? " << (childView->browserExtension() ? "YES" : "NO");
        //qDebug() << "Referrer: " << req.args.metaData()["referrer"];
        childView->setTypedURL(req.typedUrl);
        if (childView->part()) {
            childView->part()->setArguments(req.args);
        }
        if (childView->browserExtension()) {
            childView->browserExtension()->setBrowserArguments(req.browserArgs);
        }

        // see dolphinpart
        childView->part()->setProperty("filesToSelect", QVariant::fromValue(req.filesToSelect));

        if (!url.isEmpty()) {
            childView->openUrl(url, originalURL, req.nameFilter, req.tempFile);
        }
    }
    //qDebug() << "ok=" << ok << "bOthersFollowed=" << bOthersFollowed
    //             << "returning" << (ok || bOthersFollowed);
    return ok || bOthersFollowed;
}

static KonqView *findChildView(KParts::ReadOnlyPart *callingPart, const QString &name, KonqMainWindow *&mainWindow, KParts::BrowserHostExtension *&hostExtension, KParts::ReadOnlyPart **part)
{
    if (!KonqMainWindow::mainWindowList()) {
        return 0;
    }

    foreach (KonqMainWindow *window, *KonqMainWindow::mainWindowList()) {
        KonqView *res = window->childView(callingPart, name, hostExtension, part);
        if (res) {
            mainWindow = window;
            return res;
        }
    }

    return 0;
}

void KonqMainWindow::slotOpenURLRequest(const QUrl &url, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &browserArgs)
{
    //qDebug() << "frameName=" << browserArgs.frameName;

    KParts::ReadOnlyPart *callingPart = static_cast<KParts::ReadOnlyPart *>(sender()->parent());
    QString frameName = browserArgs.frameName;

    if (!frameName.isEmpty()) {
        static QString _top = QStringLiteral("_top");
        static QString _self = QStringLiteral("_self");
        static QString _parent = QStringLiteral("_parent");
        static QString _blank = QStringLiteral("_blank");

        if (frameName.toLower() == _blank) {
            KonqMainWindow *mainWindow = (m_popupProxyWindow ? m_popupProxyWindow.data() : this);
            mainWindow->slotCreateNewWindow(url, args, browserArgs);
            if (m_isPopupWithProxyWindow) {
                raiseWindow(mainWindow);
            }
            return;
        }

        if (frameName.toLower() != _top &&
                frameName.toLower() != _self &&
                frameName.toLower() != _parent) {
            KParts::BrowserHostExtension *hostExtension = 0;
            KonqView *view = childView(callingPart, frameName, hostExtension, 0);
            if (!view) {
                KonqMainWindow *mainWindow = 0;
                view = findChildView(callingPart, frameName, mainWindow, hostExtension, 0);

                if (!view || !mainWindow) {
                    slotCreateNewWindow(url, args, browserArgs);
                    return;
                }

                if (hostExtension) {
                    hostExtension->openUrlInFrame(url, args, browserArgs);
                } else {
                    mainWindow->openUrlRequestHelper(view, url, args, browserArgs);
                }
                return;
            }

            if (hostExtension) {
                hostExtension->openUrlInFrame(url, args, browserArgs);
            } else {
                openUrlRequestHelper(view, url, args, browserArgs);
            }
            return;
        }
    }

    KonqView *view = browserArgs.newTab() ? 0 : childView(callingPart);
    openUrlRequestHelper(view, url, args, browserArgs);
}

//Called by slotOpenURLRequest
void KonqMainWindow::openUrlRequestHelper(KonqView *childView, const QUrl &url, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &browserArgs)
{
    //qDebug() << "url=" << url;
    KonqOpenURLRequest req;
    req.args = args;
    req.browserArgs = browserArgs;
    openUrl(childView, url, args.mimeType(), req, browserArgs.trustedSource);
}

QObject *KonqMainWindow::lastFrame(KonqView *view)
{
    QObject *nextFrame, *viewFrame;
    nextFrame = view->frame();
    viewFrame = 0;
    while (nextFrame != 0 && !::qobject_cast<QStackedWidget *>(nextFrame)) {
        viewFrame = nextFrame;
        nextFrame = nextFrame->parent();
    }
    return nextFrame ? viewFrame : 0;
}

// Linked-views feature, plus "sidebar follows URL opened in the active view" feature
bool KonqMainWindow::makeViewsFollow(const QUrl &url,
                                     const KParts::OpenUrlArguments &args,
                                     const KParts::BrowserArguments &browserArgs,
                                     const QString &serviceType, KonqView *senderView)
{
    if (!senderView->isLinkedView() && senderView != m_currentView) {
        return false;    // none of those features apply -> return
    }

    bool res = false;
    //qDebug() << senderView->metaObject()->className() << "url=" << url << "serviceType=" << serviceType;
    KonqOpenURLRequest req;
    req.forceAutoEmbed = true;
    req.followMode = true;
    req.args = args;
    req.browserArgs = browserArgs;
    // We can't iterate over the map here, and openUrl for each, because the map can get modified
    // (e.g. by part changes). Better copy the views into a list.
    const QList<KonqView *> listViews = m_mapViews.values();

    QObject *senderFrame = lastFrame(senderView);

    foreach (KonqView *view, listViews) {
        if (view == senderView) {
            continue;
        }
        bool followed = false;
        // Views that should follow this URL as both views are linked
        if (view->isLinkedView() && senderView->isLinkedView()) {
            QObject *viewFrame = lastFrame(view);

            // Only views in the same tab of the sender will follow
            if (senderFrame && viewFrame && viewFrame != senderFrame) {
                continue;
            }

            qDebug() << "sending openUrl to view" << view->part()->metaObject()->className() << "url=" << url;

            // XXX duplicate code from ::openUrl
            if (view == m_currentView) {
                abortLoading();
                setLocationBarURL(url);
            } else {
                view->stop();
            }

            followed = openView(serviceType, url, view, req);
        } else {
            // Make the sidebar follow the URLs opened in the active view
            if (view->isFollowActive() && senderView == m_currentView) {
                followed = openView(serviceType, url, view, req);
            }
        }

        // Ignore return value if the view followed but doesn't really
        // show the file contents. We still want to see that
        // file, e.g. in a separate viewer.
        // This happens in views locked to a directory mode,
        // like sidebar and konsolepart (#52161).
        const bool ignore = view->isLockedViewMode() && view->showsDirectory();
        //qDebug() << "View " << view->service()->name()
        //              << " supports dirs: " << view->showsDirectory()
        //              << " is locked-view-mode:" << view->isLockedViewMode()
        //              << " ignore=" << ignore;
        if (!ignore) {
            res = followed || res;
        }
    }

    return res;
}

void KonqMainWindow::abortLoading()
{
    if (m_currentView) {
        m_currentView->stop(); // will take care of the statusbar
        stopAnimation();
    }
}

// Are there any indications that this window has a strong popup
// nature and should therefore not be embedded into a tab?
static bool isPopupWindow(const KParts::WindowArgs &windowArgs)
{
    // ### other settings to respect?
    return windowArgs.x() != -1 || windowArgs.y() != -1 ||
           windowArgs.width() != -1 || windowArgs.height() != -1 ||
           !windowArgs.isMenuBarVisible() ||
           !windowArgs.toolBarsVisible() ||
           !windowArgs.isStatusBarVisible();
}

// This is called for the javascript window.open call.
// Also called for MMB on link, target="_blank" link, MMB on folder, etc.
void KonqMainWindow::slotCreateNewWindow(const QUrl &url,
        const KParts::OpenUrlArguments &args,
        const KParts::BrowserArguments &browserArgs,
        const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart **part)
{
    // NOTE: 'part' may be null

    qDebug() << "url=" << url << "args.mimeType()=" << args.mimeType()
             << "browserArgs.frameName=" << browserArgs.frameName;

    // If we are a popup window, forward the request the proxy window.
    if (m_isPopupWithProxyWindow && m_popupProxyWindow) {
        m_popupProxyWindow->slotCreateNewWindow(url, args, browserArgs, windowArgs, part);
        raiseWindow(m_popupProxyWindow);
        return;
    }

    if (part) {
        *part = 0;    // Make sure to be initialized in case of failure...
    }

    KonqMainWindow *mainWindow = 0;
    if (!browserArgs.frameName.isEmpty() && browserArgs.frameName.toLower() != QLatin1String("_blank")) {
        KParts::BrowserHostExtension *hostExtension = 0;
        KParts::ReadOnlyPart *ro_part = 0;
        KParts::BrowserExtension *be = ::qobject_cast<KParts::BrowserExtension *>(sender());
        if (be) {
            ro_part = ::qobject_cast<KParts::ReadOnlyPart *>(be->parent());
        }
        if (findChildView(ro_part, browserArgs.frameName, mainWindow, hostExtension, part)) {
            // Found a view. If url isn't empty, we should open it - but this never happens currently
            // findChildView put the resulting part in 'part', so we can just return now
            //qDebug() << "frame=" << browserArgs.frameName << "-> found part=" << part << part->name();
            return;
        }
    }

    bool createTab = browserArgs.newTab();
    if (!createTab && !browserArgs.forcesNewWindow() /* explicit "Open in New Window" action, e.g. on frame or history item */) {
        if (args.actionRequestedByUser()) { // MMB or some RMB popupmenu action
            createTab = KonqSettings::mmbOpensTab();
        } else { // Javascript popup
            createTab = KonqSettings::popupsWithinTabs() &&
                        !isPopupWindow(windowArgs);
        }
    }
    qDebug() << "createTab=" << createTab << "part=" << part;

    if (createTab && !m_isPopupWithProxyWindow) {

        bool newtabsinfront = !windowArgs.lowerWindow();
        if (KonqSettings::newTabsInFront()) {
            newtabsinfront = !newtabsinfront;
        }
        const bool aftercurrentpage = KonqSettings::openAfterCurrentPage();

        KonqOpenURLRequest req;
        req.args = args;
        req.browserArgs = browserArgs;
        // Can we use the standard way (openUrl), or do we need the part pointer immediately?
        if (!part) {
            req.browserArgs.setNewTab(true);
            req.forceAutoEmbed = true; // testcase: MMB on link-to-PDF, when pdf setting is "show file in external browser".
            req.newTabInFront = newtabsinfront;
            req.openAfterCurrentPage = aftercurrentpage;
            openUrl(0, url, args.mimeType(), req);
        } else {
            KonqView *newView = m_pViewManager->addTab(QStringLiteral("text/html"), QString(), false, aftercurrentpage);
            if (newView == 0) {
                return;
            }

            if (newtabsinfront) {
                m_pViewManager->showTab(newView);
            }

            openUrl(newView, url.isEmpty() ? QUrl(QStringLiteral("about:blank")) : url, QString(), req);
            newView->setViewName(browserArgs.frameName);

            *part = newView->part();
        }

        // Raise the current window if the request to create the tab came from a popup
        // window, e.g. clicking on links with target = "_blank" in popup windows.
        KParts::BrowserExtension *be = qobject_cast<KParts::BrowserExtension *>(sender());
        KonqView *view = (be ? childView(qobject_cast<KParts::ReadOnlyPart *>(be->parent())) : 0);
        KonqMainWindow *window = view ? view->mainWindow() : 0;
        if (window && window->m_isPopupWithProxyWindow && !m_isPopupWithProxyWindow) {
            raiseWindow(this);
        }

        return;
    }

    KonqOpenURLRequest req;
    req.args = args;
    req.browserArgs = browserArgs;
    req.browserArgs.setNewTab(false); // we got a new window, no need for a new tab in that window
    req.forceAutoEmbed = true;
    req.serviceName = preferredService(m_currentView, args.mimeType());

    mainWindow = KonqMainWindowFactory::createEmptyWindow();
    mainWindow->resetAutoSaveSettings(); // Don't autosave

    // Do we know the mimetype? If not, go to generic openUrl which will use a KonqRun.
    if (args.mimeType().isEmpty()) {
        mainWindow->openUrl(0, url, QString(), req);
    } else {
        if (!mainWindow->openView(args.mimeType(), url, 0, req)) {
            // we have problems. abort.
            delete mainWindow;

            if (part) {
                *part = 0;
            }
            return;
        }
    }

    qDebug() << "newWindow" << mainWindow << "currentView" << mainWindow->currentView() << "views" << mainWindow->viewMap().count();

    KonqView *view = 0;
    // cannot use activePart/currentView, because the activation through the partmanager
    // is delayed by a singleshot timer (see KonqViewManager::setActivePart)
    // ### TODO: not true anymore
    if (mainWindow->viewMap().count()) {
        MapViews::ConstIterator it = mainWindow->viewMap().begin();
        view = it.value();

        if (part) {
            *part = it.key();
        }
    }

    // activate the view now in order to make the menuBar() hide call work
    if (part && *part) {
        mainWindow->viewManager()->setActivePart(*part);
    }

#if KONQ_HAVE_X11
    // WORKAROUND: Clear the window state information set by KMainWindow::restoreWindowSize
    // so that the size and location settings we set below always take effect.
    KWindowSystem::clearState(mainWindow->winId(), NET::Max);
#endif

    // process the window args
    const int xPos = ((windowArgs.x() == -1) ?  mainWindow->x() : windowArgs.x());
    const int yPos = ((windowArgs.y() == -1) ?  mainWindow->y() : windowArgs.y());
    const int width = ((windowArgs.width() == -1) ?  mainWindow->width() : windowArgs.width());
    const int height = ((windowArgs.height() == -1) ?  mainWindow->height() : windowArgs.height());

    mainWindow->move(xPos, yPos);
    mainWindow->resize(width, height);

    // Make the window open properties configurable. This is equivalent to
    // Firefox's "dom.disable_window_open_feature.*" properties. For now
    // only LocationBar visiblity is configurable.
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cfg(config, "DisableWindowOpenFeatures");

    if (!windowArgs.isMenuBarVisible()) {
        mainWindow->menuBar()->hide();
        mainWindow->m_paShowMenuBar->setChecked(false);
    }

    if (!windowArgs.toolBarsVisible()) {
        // For security reasons the address bar is NOT hidden by default. The
        // user can override the default setup by adding a config option
        // "LocationBar=false" to the [DisableWindowOpenFeatures] section of
        // konquerorrc.
        const bool showLocationBar = cfg.readEntry("LocationBar", true);
        KToolBar *locationToolBar = mainWindow->toolBar(QStringLiteral("locationToolBar"));

        Q_FOREACH (KToolBar *bar, mainWindow->findChildren<KToolBar *>()) {
            if (bar != locationToolBar || !showLocationBar) {
                bar->hide();
            }
        }

        if (locationToolBar && showLocationBar && isPopupWindow(windowArgs)) {
            // Hide all the actions of the popup window
            KActionCollection *collection = mainWindow->actionCollection();
            for (int i = 0, count = collection->count(); i < count; ++i) {
                collection->action(i)->setVisible(false);
            }

            // Show only those actions that are allowed in a popup window
            static const char *const s_allowedActions[] = {
                "go_back", "go_forward", "go_up", "reload", "hard_reload",
                "stop", "cut", "copy", "paste", "print", "fullscreen",
                "add_bookmark", "new_window", 0
            };
            for (int i = 0; s_allowedActions[i]; ++i) {
                if (QAction *action = collection->action(QLatin1String(s_allowedActions[i]))) {
                    action->setVisible(true);
                }
            }

            // Make only the address widget available in the location toolbar
            locationToolBar->clear();
            QAction *action = locationToolBar->addWidget(mainWindow->m_combo);
            action->setVisible(true);

            // Make the combo box non editable and clear it of previous history
            QLineEdit *edit = (mainWindow->m_combo ? mainWindow->m_combo->lineEdit() : 0);
            if (edit) {
                mainWindow->m_combo->clear();
                mainWindow->m_combo->setCompletionMode(KCompletion::CompletionNone);
                edit->setReadOnly(true);
            }

            // Store the originating window as the popup's proxy window so that
            // new tab requests in the popup window are forwarded to it.
            mainWindow->m_popupProxyWindow = this;
            mainWindow->m_isPopupWithProxyWindow = true;
        }
    }

    if (view) {
        if (!windowArgs.scrollBarsVisible()) {
            view->disableScrolling();
        }
        if (!windowArgs.isStatusBarVisible()) {
            view->frame()->statusbar()->hide();
            mainWindow->m_paShowStatusBar->setChecked(false);
        } else {
            mainWindow->m_paShowStatusBar->setChecked(true);
        }
    }

    if (!windowArgs.isResizable())
        // ### this doesn't seem to work :-(
    {
        mainWindow->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    }

// Trying to show the window initially behind the current window is a bit tricky,
// as this involves the window manager, which may see things differently.
// Many WMs raise and activate new windows, which means without WM support this won't work very
// well. If the WM has support for _NET_WM_USER_TIME, it will be just set to 0 (=don't focus on show),
// and the WM should take care of it itself.
    bool wm_usertime_support = false;

#if KONQ_HAVE_X11
    if (KWindowSystem::platform() == KWindowSystem::Platform::X11) {
        auto saved_last_input_time = QX11Info::appUserTime();
        if (windowArgs.lowerWindow()) {
            NETRootInfo wm_info(QX11Info::connection(), NET::Supported);
            wm_usertime_support = wm_info.isSupported(NET::WM2UserTime);
            if (wm_usertime_support) {
                // *sigh*, and I thought nobody would need QWidget::dontFocusOnShow().
                // Avoid Qt's support for user time by setting it to 0, and
                // set the property ourselves.
                QX11Info::setAppUserTime(0);
                KWindowSystem::setUserTime(mainWindow->winId(), 0);
            }
            // Put below the current window before showing, in case that actually works with the WM.
            // First do complete lower(), then stackUnder(), because the latter may not work with many WMs.
            mainWindow->lower();
            mainWindow->stackUnder(this);
        }

        mainWindow->show();

        if (windowArgs.lowerWindow()) {
            QX11Info::setAppUserTime(saved_last_input_time);
            if (!wm_usertime_support) {
                // No WM support. Let's try ugly tricks.
                mainWindow->lower();
                mainWindow->stackUnder(this);
                if (this->isActiveWindow()) {
                    this->activateWindow();
                }
            }
        }
    }
#else // KONQ_HAVE_X11
    mainWindow->show();
#endif

    if (windowArgs.isFullScreen()) {
        mainWindow->action("fullscreen")->trigger();
    }
}

void KonqMainWindow::slotNewWindow()
{
    KonqMainWindow *mainWin = KonqMainWindowFactory::createNewWindow();
    mainWin->show();
}

void KonqMainWindow::slotDuplicateWindow()
{
    m_pViewManager->duplicateWindow()->show();
}

void KonqMainWindow::slotSendURL()
{
    const QList<QUrl> lst = currentURLs();
    QString body;
    QString fileNameList;
    for (QList<QUrl>::ConstIterator it = lst.constBegin(); it != lst.constEnd(); ++it) {
        if (!body.isEmpty()) {
            body += '\n';
        }
        body += (*it).toDisplayString();
        if (!fileNameList.isEmpty()) {
            fileNameList += QLatin1String(", ");
        }
        fileNameList += (*it).fileName();
    }
    QString subject;
    if (m_currentView && !m_currentView->showsDirectory()) {
        subject = m_currentView->caption();
    } else { // directory view
        subject = fileNameList;
    }
    QUrl mailtoUrl;
    mailtoUrl.setScheme(QStringLiteral("mailto"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("subject"), subject);
    query.addQueryItem(QStringLiteral("body"), body);
    mailtoUrl.setQuery(query);
    QDesktopServices::openUrl(mailtoUrl);
}

void KonqMainWindow::slotSendFile()
{
    const QList<QUrl> lst = currentURLs();
    QStringList urls;
    QString fileNameList;
    for (QList<QUrl>::ConstIterator it = lst.constBegin(); it != lst.constEnd(); ++it) {
        if (!fileNameList.isEmpty()) {
            fileNameList += QLatin1String(", ");
        }
        if ((*it).isLocalFile() && QFileInfo((*it).toLocalFile()).isDir()) {
            // Create a temp dir, so that we can put the ZIP file in it with a proper name
            // Problem: when to delete it?
            QTemporaryDir tempDir;
            tempDir.setAutoRemove(false);
            if (!tempDir.isValid()) {
                qWarning() << "Could not create temporary dir";
                continue;
            }
            const QString zipFileName = tempDir.path() + '/' + (*it).fileName() + ".zip";
            KZip zip(zipFileName);
            if (!zip.open(QIODevice::WriteOnly)) {
                qWarning() << "Could not open" << zipFileName << "for writing";
                continue;
            }
            zip.addLocalDirectory((*it).path(), QString());
            zip.close();
            fileNameList += (*it).fileName() + ".zip";
            urls.append(QUrl::fromLocalFile(zipFileName).url());
        } else {
            fileNameList += (*it).fileName();
            urls.append((*it).url());
        }
    }
    QString subject;
    if (m_currentView && !m_currentView->showsDirectory()) {
        subject = m_currentView->caption();
    } else {
        subject = fileNameList;
    }
    QUrl mailtoUrl;
    mailtoUrl.setScheme(QStringLiteral("mailto"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("subject"), subject);
    for (const QString& url : urls) {
        query.addQueryItem(QStringLiteral("attachment"), url);
    }
    mailtoUrl.setQuery(query);
    QDesktopServices::openUrl(mailtoUrl);
}

void KonqMainWindow::slotOpenLocation()
{
    focusLocationBar();
    QLineEdit *edit = comboEdit();
    if (edit) {
        edit->selectAll();
    }
}

void KonqMainWindow::slotOpenFile()
{
    QUrl currentUrl;
    if (m_currentView && m_currentView->url().isLocalFile()) {
        currentUrl = m_currentView->url();
    } else {
        currentUrl = QUrl::fromLocalFile(QDir::homePath());
    }

    QUrl url = KFileDialog::getOpenUrl(currentUrl, QString(), this);
    if (!url.isEmpty()) {
        openFilteredUrl(url.url().trimmed());
    }
}

void KonqMainWindow::slotIconsChanged()
{
    qDebug();
    if (m_combo) {
        m_combo->updatePixmaps();
    }
    m_pViewManager->updatePixmaps();
    updateWindowIcon();
}

void KonqMainWindow::slotOpenWith()
{
    if (!m_currentView) {
        return;
    }

    const QList<QUrl> lst{ m_currentView->url() };
    const QString serviceName = sender()->objectName();
    const KService::List offers = m_currentView->appServiceOffers();
    KService::List::ConstIterator it = offers.begin();
    const KService::List::ConstIterator end = offers.end();
    for (; it != end; ++it) {
        if ((*it)->desktopEntryName() == serviceName) {
            KRun::runApplication(**it, lst, this);
            return;
        }
    }
}

void KonqMainWindow::slotViewModeTriggered(QAction *action)
{
    if (!m_currentView) {
        return;
    }

    // Gather data from the action, since the action will be deleted by changePart
    QString modeName = action->objectName();
    Q_ASSERT(modeName.endsWith("-viewmode"));
    modeName.chop(9);
    const QString internalViewMode = action->data().toString();

    if (m_currentView->service()->desktopEntryName() != modeName) {
        m_currentView->stop();
        m_currentView->lockHistory();

        // Save those, because changePart will lose them
        const QUrl url = m_currentView->url();
        const QString locationBarURL = m_currentView->locationBarURL();
#if 0
        // Problem: dolphinpart doesn't currently implement it. But we don't need it that much
        // now that it's the main filemanagement part for all standard modes.
        QList<QUrl> filesToSelect = childView->part()->property("filesToSelect").value<QList<QUrl>>();
#endif

        m_currentView->changePart(m_currentView->serviceType(), modeName);
        m_currentView->openUrl(url, locationBarURL);
    }

    if (!internalViewMode.isEmpty() && internalViewMode != m_currentView->internalViewMode()) {
        m_currentView->setInternalViewMode(internalViewMode);
    }
}

void KonqMainWindow::slotLockView()
{
    if (!m_currentView) {
        return;
    }

    m_currentView->setLockedLocation(m_paLockView->isChecked());
}

void KonqMainWindow::slotStop()
{
    abortLoading();
    if (m_currentView) {
        m_currentView->frame()->statusbar()->message(i18n("Canceled."));
    }
}

void KonqMainWindow::slotLinkView()
{
    if (!m_currentView) {
        return;
    }

    // Can't access this action in passive mode anyway
    Q_ASSERT(!m_currentView->isPassiveMode());
    const bool mode = !m_currentView->isLinkedView();

    const QList<KonqView *> linkableViews = KonqLinkableViewsCollector::collect(this);
    if (linkableViews.count() == 2) {
        // Exactly two linkable views : link both
        linkableViews.at(0)->setLinkedView(mode);
        linkableViews.at(1)->setLinkedView(mode);
    } else { // Normal case : just this view
        m_currentView->setLinkedView(mode);
    }
}

void KonqMainWindow::slotReload(KonqView *reloadView, bool softReload)
{
    if (!reloadView) {
        reloadView = m_currentView;
    }

    if (!reloadView || (reloadView->url().isEmpty() && reloadView->locationBarURL().isEmpty())) {
        return;
    }

    if (reloadView->isModified()) {
        if (KMessageBox::warningContinueCancel(this,
                                               i18n("This page contains changes that have not been submitted.\nReloading the page will discard these changes."),
                                               i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"), QStringLiteral("view-refresh")), KStandardGuiItem::cancel(), QStringLiteral("discardchangesreload")) != KMessageBox::Continue) {
            return;
        }
    }

    KonqOpenURLRequest req(reloadView->typedUrl());
    req.userRequestedReload = true;
    if (reloadView->prepareReload(req.args, req.browserArgs, softReload)) {
        reloadView->lockHistory();
        // Reuse current servicetype for local files, but not for remote files (it could have changed, e.g. over HTTP)
        QString serviceType = reloadView->url().isLocalFile() ? reloadView->serviceType() : QString();
        // By using locationBarURL instead of url, we preserve name filters (#54687)
        QUrl reloadUrl = QUrl::fromUserInput(reloadView->locationBarURL(), QString(), QUrl::AssumeLocalFile);
        if (reloadUrl.isEmpty()) { // e.g. initial screen
            reloadUrl = reloadView->url();
        }
        openUrl(reloadView, reloadUrl, serviceType, req);
    }
}

void KonqMainWindow::slotForceReload()
{
    // A forced reload is simply a "hard" (i.e. - not soft!) reload.
    slotReload(0L /* Current view */, false /* Not softReload*/);
}

void KonqMainWindow::slotReloadPopup()
{
    KonqFrameBase *tab = m_pViewManager->tabContainer()->tabAt(m_workingTab);
    if (tab) {
        slotReload(tab->activeChildView());
    }
}

void KonqMainWindow::slotHome()
{
    const QString homeURL = m_paHomePopup->data().toString();

    KonqOpenURLRequest req;
    req.browserArgs.setNewTab(true);
    req.newTabInFront = KonqSettings::newTabsInFront();

    Qt::MouseButtons buttons = QApplication::mouseButtons();
    Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();

    if (modifiers & Qt::ShiftModifier) {
        req.newTabInFront = !req.newTabInFront;
    }

    if (modifiers & Qt::ControlModifier) { // Ctrl Left/MMB
        openFilteredUrl(homeURL, req);
    } else if (buttons & Qt::MiddleButton) {
        if (KonqSettings::mmbOpensTab()) {
            openFilteredUrl(homeURL, req);
        } else {
            const QUrl finalURL = KonqMisc::konqFilteredURL(this, homeURL);
            KonqMainWindow *mw = KonqMainWindowFactory::createNewWindow(finalURL);
            mw->show();
        }
    } else {
        openFilteredUrl(homeURL, false);
    }
}

void KonqMainWindow::slotHomePopupActivated(QAction *action)
{
    openUrl(0, QUrl(action->data().toString()));
}

void KonqMainWindow::slotGoHistory()
{
    if (!m_historyDialog) {
        m_historyDialog = new KonqHistoryDialog(this);
        m_historyDialog->setAttribute(Qt::WA_DeleteOnClose);
        m_historyDialog->setModal(false);
    }
    m_historyDialog->show();
}

void KonqMainWindow::slotConfigureExtensions()
{
    KonqExtensionManager extensionManager(this, this, m_currentView ? m_currentView->part() : 0);
    extensionManager.exec();
}

void KonqMainWindow::slotConfigure()
{
    if (!m_configureDialog) {
        m_configureDialog = new KCMultiDialog(this);
        m_configureDialog->setObjectName(QStringLiteral("configureDialog"));
        connect(m_configureDialog, &KCMultiDialog::finished, this, &KonqMainWindow::slotConfigureDone);

        //BEGIN SYNC with initActions()
        const char *const toplevelModules[] = {
            "khtml_general",
#ifndef Q_OS_WIN
            "kcmkonqyperformance",
#endif
            "bookmarks"
        };
        for (uint i = 0; i < sizeof(toplevelModules) / sizeof(char *); ++i)
            if (KAuthorized::authorizeControlModule(toplevelModules[i])) {
                m_configureDialog->addModule(KCModuleInfo(QString(toplevelModules[i]) + ".desktop"));
            }

        if (KAuthorized::authorizeControlModule(QStringLiteral("filebehavior"))) {
            KPageWidgetItem *fileManagementGroup = m_configureDialog->addModule(QStringLiteral("filebehavior"));
            if (fileManagementGroup) {
                fileManagementGroup->setName(i18n("File Management"));
                const char *const fmModules[] = {
                    "kcmdolphinviewmodes",
                    "kcmdolphinnavigation",
                    "kcmdolphinservices",
                    "kcmdolphingeneral",
                    "filetypes",
                    "kcmtrash"
                };
                for (uint i = 0; i < sizeof(fmModules) / sizeof(char *); ++i)
                    if (KAuthorized::authorizeControlModule(fmModules[i])) {
                        m_configureDialog->addModule(KCModuleInfo(QString(fmModules[i]) + ".desktop"), fileManagementGroup);
                    }
            } else {
                qWarning() << "Unable to load the \"File Management\" configuration module";
            }
        }

        if (KAuthorized::authorizeControlModule(QStringLiteral("khtml_behavior"))) {
            KPageWidgetItem *webGroup = m_configureDialog->addModule(QStringLiteral("khtml_behavior"));
            if (webGroup) {
                webGroup->setName(i18n("Web Browsing"));

                const char *const webModules[] = {
                    "khtml_appearance",
                    "khtml_filter",
                    "ebrowsing",
                    "cache",
                    "proxy",
                    "kcmhistory",
                    "cookies",
                    "crypto",
                    "useragent",
                    "khtml_java_js",
                    "khtml_plugins"
                };
                for (uint i = 0; i < sizeof(webModules) / sizeof(char *); ++i)
                    if (KAuthorized::authorizeControlModule(webModules[i])) {
                        m_configureDialog->addModule(KCModuleInfo(QString(webModules[i]) + ".desktop"), webGroup);
                    }
            } else {
                qWarning() << "Unable to load the \"Web Browsing\" configuration module";
            }

        }
        //END SYNC with initActions()

    }

    m_configureDialog->show();

}

void KonqMainWindow::slotConfigureDone()
{
    // Cleanup the dialog so other instances can use it..
    if (m_configureDialog) {
        m_configureDialog->deleteLater();
        m_configureDialog = 0;
    }
}

void KonqMainWindow::slotConfigureSpellChecking()
{
#pragma message("TODO KF5: Port Sonnet::ConfigDialog usage somehow")
#if 0 // KF5 TODO
    Sonnet::ConfigDialog dialog(KSharedConfig::openConfig().data(), this);
    dialog.setWindowIcon(QIcon::fromTheme("konqueror"));
    dialog.exec();
#endif
}

void KonqMainWindow::slotConfigureToolbars()
{
    slotForceSaveMainWindowSettings();
    KEditToolBar dlg(factory(), this);
    connect(&dlg, &KEditToolBar::newToolBarConfig, this, &KonqMainWindow::slotNewToolbarConfig);
    connect(&dlg, &KEditToolBar::newToolBarConfig, this, &KonqMainWindow::initBookmarkBar);
    dlg.exec();
    checkDisableClearButton();
}

void KonqMainWindow::slotNewToolbarConfig() // This is called when OK or Apply is clicked
{
    if (m_toggleViewGUIClient) {
        plugActionList(QStringLiteral("toggleview"), m_toggleViewGUIClient->actions());
    }
    if (m_currentView && m_currentView->appServiceOffers().count() > 0) {
        plugActionList(QStringLiteral("openwith"), m_openWithActions);
    }

    plugViewModeActions();

    KConfigGroup cg = KSharedConfig::openConfig()->group("KonqMainWindow");
    applyMainWindowSettings(cg);
}

void KonqMainWindow::slotUndoAvailable(bool avail)
{
    m_paUndo->setEnabled(avail);
}

void KonqMainWindow::slotPartChanged(KonqView *childView, KParts::ReadOnlyPart *oldPart, KParts::ReadOnlyPart *newPart)
{
    m_mapViews.remove(oldPart);
    m_mapViews.insert(newPart, childView);

    // Remove the old part, and add the new part to the manager
    const bool wasActive = m_pViewManager->activePart() == oldPart;

    m_pViewManager->replacePart(oldPart, newPart, false);

    // Set active immediately - but only if the old part was the active one (#67956)
    if (wasActive) {
        // Note: this makes the new part active... so it calls slotPartActivated
        m_pViewManager->setActivePart(newPart);
    }

    viewsChanged();
}

void KonqMainWindow::slotRunFinished()
{
    const KonqRun *run = static_cast<const KonqRun *>(sender());

    if (!run->mailtoURL().isEmpty()) {
        QDesktopServices::openUrl(run->mailtoURL());
    }

    if (run->hasError()) {   // we had an error
        QDBusMessage message = QDBusMessage::createSignal(KONQ_MAIN_PATH, QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("removeFromCombo"));
        message << run->url().toDisplayString();
        QDBusConnection::sessionBus().send(message);
    }

    KonqView *childView = run->childView();

    // Check if we found a mimetype _and_ we got no error (example: cancel in openwith dialog)
    if (run->wasMimeTypeFound() && !run->hasError()) {

        // We do this here and not in the constructor, because
        // we are waiting for the first view to be set up before doing this...
        // Note: this is only used when konqueror is started from command line.....
        if (m_bNeedApplyKonqMainWindowSettings) {
            m_bNeedApplyKonqMainWindowSettings = false; // only once
            applyKonqMainWindowSettings();
        }

        return;
    }

    // An error happened in KonqRun - stop wheel etc.

    if (childView) {
        childView->setLoading(false);

        if (childView == m_currentView) {
            stopAnimation();

            // Revert to working URL - unless the URL was typed manually
            if (run->typedUrl().isEmpty() && childView->currentHistoryEntry()) { // not typed
                childView->setLocationBarURL(childView->currentHistoryEntry()->locationBarURL);
            }
        }
    } else { // No view, e.g. starting up empty
        stopAnimation();
    }
}

void KonqMainWindow::applyKonqMainWindowSettings()
{
    const QStringList toggableViewsShown = KonqSettings::toggableViewsShown();
    QStringList::ConstIterator togIt = toggableViewsShown.begin();
    QStringList::ConstIterator togEnd = toggableViewsShown.end();
    for (; togIt != togEnd; ++togIt) {
        // Find the action by name
        //    QAction * act = m_toggleViewGUIClient->actionCollection()->action( (*togIt).toLatin1() );
        QAction *act = m_toggleViewGUIClient->action(*togIt);
        if (act) {
            act->trigger();
        } else {
            qWarning() << "Unknown toggable view in ToggableViewsShown " << *togIt;
        }
    }
}

void KonqMainWindow::slotSetStatusBarText(const QString &)
{
    // Reimplemented to disable KParts::MainWindow default behaviour
    // Does nothing here, see KonqFrame
}

void KonqMainWindow::slotViewCompleted(KonqView *view)
{
    Q_ASSERT(view);

    // Need to update the current working directory
    // of the completion object every time the user
    // changes the directory!! (DA)
    if (m_pURLCompletion) {
        m_pURLCompletion->setDir(QUrl::fromUserInput(view->locationBarURL()));
    }
}

void KonqMainWindow::slotPartActivated(KParts::Part *part)
{
    qDebug() << part << (part ? part->componentData().componentName() : QLatin1String(""));

    KonqView *newView = 0;
    KonqView *oldView = m_currentView;

    if (part) {
        newView = m_mapViews.value(static_cast<KParts::ReadOnlyPart *>(part));
        Q_ASSERT(newView);
        if (newView->isPassiveMode()) {
            // Passive view. Don't connect anything, don't change m_currentView
            // Another view will become the current view very soon
            //qDebug() << "Passive mode - return";
            return;
        }
    }

    KParts::BrowserExtension *ext = 0;

    if (oldView) {
        ext = oldView->browserExtension();
        if (ext) {
            //qDebug() << "Disconnecting extension for view" << oldView;
            disconnectExtension(ext);
        }
    }

    qDebug() << "New current view" << newView;
    m_currentView = newView;
    if (newView) {
        m_paShowStatusBar->setChecked(newView->frame()->statusbar()->isVisible());
    }

    if (!part) {
        //qDebug() << "No part activated - returning";
        unplugViewModeActions();
        createGUI(0);
        KParts::MainWindow::setCaption(QString());
        return;
    }

    ext = m_currentView->browserExtension();

    if (ext) {
        connectExtension(ext);
    } else {
        qDebug() << "No Browser Extension for the new part";
        // Disable all browser-extension actions

        KParts::BrowserExtension::ActionSlotMap *actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();
        KParts::BrowserExtension::ActionSlotMap::ConstIterator it = actionSlotMap->constBegin();
        const KParts::BrowserExtension::ActionSlotMap::ConstIterator itEnd = actionSlotMap->constEnd();
        for (; it != itEnd; ++it) {
            QAction *act = actionCollection()->action(QString::fromLatin1(it.key()));
            Q_ASSERT(act);
            if (act) {
                act->setEnabled(false);
            }
        }

        if (m_paCopyFiles) {
            m_paCopyFiles->setEnabled(false);
        }
        if (m_paMoveFiles) {
            m_paMoveFiles->setEnabled(false);
        }
    }

    createGUI(part);

    // View-dependent GUI

    KParts::MainWindow::setCaption(KStringHandler::csqueeze(m_currentView->caption(), 128));
    // This line causes #170470 when removing the current tab, because QTabBar
    // emits currentChanged before calling tabRemoved, so KTabWidget gets confused.
    // I don't see a need for it anyway...
    //m_currentView->frame()->setTitle(m_currentView->caption(), 0);

    updateOpenWithActions();
    updateViewActions(); // undo, lock, link and other view-dependent actions
    updateViewModeActions();

    bool viewShowsDir = m_currentView->showsDirectory();
    bool buttonShowsFolder = m_paHomePopup->text() == i18n("Home Folder");
    if (m_paHomePopup->text() == i18n("Home") || viewShowsDir != buttonShowsFolder) {
        QAction *actHomeFolder = new QAction(this);
        QAction *actHomePage = new QAction(this);

        actHomeFolder->setIcon(QIcon::fromTheme(QStringLiteral("user-home")));
        actHomeFolder->setText(i18n("Home Folder"));
        actHomeFolder->setStatusTip(i18n("Navigate to your 'Home Folder'"));
        actHomeFolder->setWhatsThis(i18n("Navigate to your local 'Home Folder'"));
        actHomeFolder->setData(QDir::homePath());
        actHomePage->setIcon(QIcon::fromTheme(QStringLiteral("go-home")));
        actHomePage->setText(i18n("Home Page"));

        actHomePage->setStatusTip(i18n("Navigate to your 'Home Page'"));
        actHomePage->setWhatsThis(i18n("<html>Navigate to your 'Home Page'<br /><br />"
                                       "You can configure the location where this button takes you "
                                       "under <b>Settings -> Configure Konqueror -> General</b>.</html>"));
        actHomePage->setData(KonqSettings::homeURL());

        m_paHome->setIcon(viewShowsDir ? actHomeFolder->icon() : actHomePage->icon());
        m_paHome->setText(viewShowsDir ? actHomeFolder->text() : actHomePage->text());
        m_paHome->setStatusTip(viewShowsDir ? actHomeFolder->statusTip() : actHomePage->statusTip());
        m_paHome->setWhatsThis(viewShowsDir ? actHomeFolder->whatsThis() : actHomePage->whatsThis());
        m_paHomePopup->setIcon(viewShowsDir ? actHomeFolder->icon() : actHomePage->icon());
        m_paHomePopup->setText(viewShowsDir ? actHomeFolder->text() : actHomePage->text());
        m_paHomePopup->setStatusTip(viewShowsDir ? actHomeFolder->statusTip() : actHomePage->statusTip());
        m_paHomePopup->setWhatsThis(viewShowsDir ? actHomeFolder->whatsThis() : actHomePage->whatsThis());
        m_paHomePopup->setData(viewShowsDir ? actHomeFolder->data() : actHomePage->data());
        m_paHomePopup->menu()->clear();
        if (viewShowsDir) {
            m_paHomePopup->menu()->addAction(actHomePage);
            delete actHomeFolder;
        } else {
            m_paHomePopup->menu()->addAction(actHomeFolder);
            delete actHomePage;
        }
    }

    m_currentView->frame()->statusbar()->updateActiveStatus();

    if (oldView && oldView->frame()) {
        oldView->frame()->statusbar()->updateActiveStatus();
    }

    //qDebug() << "setting location bar url to"
    //         << m_currentView->locationBarURL() << "m_currentView=" << m_currentView;

    // Make sure the location bar gets updated when the view(tab) is changed.
    if (oldView != newView && m_combo) {
        m_combo->lineEdit()->setModified(false);
    }
    m_currentView->setLocationBarURL(m_currentView->locationBarURL());

    updateToolBarActions();

    m_currentView->setActiveComponent();
}

void KonqMainWindow::insertChildView(KonqView *childView)
{
    //qDebug() << childView;
    m_mapViews.insert(childView->part(), childView);

    connect(childView, SIGNAL(viewCompleted(KonqView*)),
            this, SLOT(slotViewCompleted(KonqView*)));

    emit viewAdded(childView);
}

// Called by KonqViewManager, internal
void KonqMainWindow::removeChildView(KonqView *childView)
{
    //qDebug() << childView;

    disconnect(childView, SIGNAL(viewCompleted(KonqView*)),
               this, SLOT(slotViewCompleted(KonqView*)));

#ifndef NDEBUG
    //dumpViewList();
#endif

    MapViews::Iterator it = m_mapViews.begin();
    const MapViews::Iterator end = m_mapViews.end();

    // find it in the map - can't use the key since childView->part() might be 0

    //qDebug() << "Searching map";

    while (it != end && it.value() != childView) {
        ++it;
    }

    //qDebug() << "Verifying search results";

    if (it == m_mapViews.end()) {
        qWarning() << "KonqMainWindow::removeChildView childView " << childView << " not in map !";
        return;
    }

    //qDebug() << "Removing view" << childView;

    m_mapViews.erase(it);

    emit viewRemoved(childView);

#ifndef NDEBUG
    //dumpViewList();
#endif

    // KonqViewManager takes care of m_currentView
}

void KonqMainWindow::linkableViewCountChanged()
{
    const QList<KonqView *> linkableViews = KonqLinkableViewsCollector::collect(this);
    const int lvc = linkableViews.count();
    m_paLinkView->setEnabled(lvc > 1);
    // Only one view -> unlink it
    if (lvc == 1) {
        linkableViews.at(0)->setLinkedView(false);
    }
    m_pViewManager->viewCountChanged();
}

void KonqMainWindow::viewCountChanged()
{
    // This is called (by the view manager) when the number of views changes.
    linkableViewCountChanged();
    viewsChanged();
}

void KonqMainWindow::viewsChanged()
{
    // This is called when the number of views changes OR when
    // the type of some view changes.

    updateViewActions(); // undo, lock, link and other view-dependent actions
}

KonqView *KonqMainWindow::childView(KParts::ReadOnlyPart *view)
{
    return m_mapViews.value(view);
}

KonqView *KonqMainWindow::childView(KParts::ReadOnlyPart *callingPart, const QString &name, KParts::BrowserHostExtension *&hostExtension, KParts::ReadOnlyPart **part)
{
    //qDebug() << "this=" << this << "looking for" << name;
    QList<KonqView *> views = m_mapViews.values();
    KonqView *callingView = m_mapViews.value(callingPart);
    if (callingView) {
        // Move the callingView in front of the list, in case of duplicate frame names (#133967)
        if (views.removeAll(callingView)) {
            views.prepend(callingView);
        }
    }

    Q_FOREACH (KonqView *view, views) {
        QString viewName = view->viewName();
        //qDebug() << "       - viewName=" << viewName
        //          << "frame names:" << view->frameNames();

        // First look for a hostextension containing this frame name
        KParts::BrowserHostExtension *ext = KParts::BrowserHostExtension::childObject(view->part());
        if (ext) {
            ext = ext->findFrameParent(callingPart, name);
            qDebug() << "BrowserHostExtension found part" << ext;
            if (!ext) {
                continue;    // Don't use this window
            }
        }

        if (!viewName.isEmpty() && viewName == name) {
            qDebug() << "found existing view by name:" << view;
            hostExtension = 0;
            if (part) {
                *part = view->part();
            }
            return view;
        }

//    KParts::BrowserHostExtension* ext = KonqView::hostExtension( view->part(), name );

        if (ext) {
            const QList<KParts::ReadOnlyPart *> frames = ext->frames();
            QListIterator<KParts::ReadOnlyPart *> frameIt(frames);
            while (frameIt.hasNext()) {
                KParts::ReadOnlyPart *item = frameIt.next();
                if (item->objectName() == name) {
                    qDebug() << "found a frame of name" << name << ":" << item;
                    hostExtension = ext;
                    if (part) {
                        *part = item;
                    }
                    return view;
                }
            }
        }
    }

    return 0;
}

int KonqMainWindow::activeViewsNotLockedCount() const
{
    int res = 0;
    MapViews::ConstIterator end = m_mapViews.constEnd();
    for (MapViews::ConstIterator it = m_mapViews.constBegin(); it != end; ++it) {
        if (!it.value()->isPassiveMode() && !it.value()->isLockedLocation()) {
            ++res;
        }
    }

    return res;
}

int KonqMainWindow::linkableViewsCount() const
{
    return KonqLinkableViewsCollector::collect(const_cast<KonqMainWindow *>(this)).count();
}

int KonqMainWindow::mainViewsCount() const
{
    int res = 0;
    MapViews::ConstIterator it = m_mapViews.constBegin();
    const MapViews::ConstIterator end = m_mapViews.constEnd();
    for (; it != end; ++it) {
        if (!it.value()->isPassiveMode() && !it.value()->isToggleView()) {
            //qDebug() << res << it.value() << it.value()->part()->widget();
            ++res;
        }
    }

    return res;
}

void KonqMainWindow::slotURLEntered(const QString &text, Qt::KeyboardModifiers modifiers)
{
    if (m_bURLEnterLock || text.isEmpty()) {
        return;
    }

    m_bURLEnterLock = true;

    if ((modifiers & Qt::ControlModifier) || (modifiers & Qt::AltModifier)) {
        m_combo->setURL(m_currentView ? m_currentView->url().toDisplayString() : QString());
        const bool inNewTab = !m_isPopupWithProxyWindow; // do not open a new tab in popup window.
        openFilteredUrl(text.trimmed(), inNewTab);
    } else {
        openFilteredUrl(text.trimmed());
    }

    m_bURLEnterLock = false;
}

void KonqMainWindow::slotSplitViewHorizontal()
{
    if (!m_currentView) {
        return;
    }
    KonqView *oldView = m_currentView;
    KonqView *newView = m_pViewManager->splitView(m_currentView, Qt::Horizontal);
    if (newView == 0) {
        return;
    }
    KonqOpenURLRequest req;
    req.forceAutoEmbed = true;
    openView(oldView->serviceType(), oldView->url(), newView, req);
}

void KonqMainWindow::slotSplitViewVertical()
{
    if (!m_currentView) {
        return;
    }
    KonqView *oldView = m_currentView;
    KonqView *newView = m_pViewManager->splitView(m_currentView, Qt::Vertical);
    if (newView == 0) {
        return;
    }
    KonqOpenURLRequest req;
    req.forceAutoEmbed = true;
    openView(oldView->serviceType(), oldView->url(), newView, req);
}

void KonqMainWindow::slotAddTab()
{
    // we can hardcode text/html because this is what about:blank will use anyway
    KonqView *newView = m_pViewManager->addTab(QStringLiteral("text/html"),
                        QString(),
                        false,
                        KonqSettings::openAfterCurrentPage());
    if (!newView) {
        return;
    }

    openUrl(newView, QUrl(QStringLiteral("about:blank")), QString());

    //HACK!! QTabBar likes to steal focus when changing widgets.  This can result
    //in a flicker since we don't want it to get focus we want the combo to get
    //or keep focus...
    // TODO: retest, and replace with the smaller hack from KTabWidget::moveTab
    QWidget *widget = newView->frame() && newView->frame()->part() ?
                      newView->frame()->part()->widget() : 0;
    QWidget *origFocusProxy = widget ? widget->focusProxy() : 0;
    if (widget) {
        widget->setFocusProxy(m_combo);
    }

    m_pViewManager->showTab(newView);

    if (widget) {
        widget->setFocusProxy(origFocusProxy);
    }

    m_workingTab = 0;
}

void KonqMainWindow::slotDuplicateTab()
{
    m_pViewManager->duplicateTab(m_pViewManager->tabContainer()->currentIndex(), KonqSettings::openAfterCurrentPage());
}

void KonqMainWindow::slotDuplicateTabPopup()
{
    m_pViewManager->duplicateTab(m_workingTab, KonqSettings::openAfterCurrentPage());
}

void KonqMainWindow::slotBreakOffTab()
{
    breakOffTab(m_pViewManager->tabContainer()->currentIndex());
}

void KonqMainWindow::slotBreakOffTabPopup()
{
    // Delay the call since it might delete the tabbar
    QMetaObject::invokeMethod(this, "breakOffTab", Qt::QueuedConnection, Q_ARG(int, m_workingTab));
}

void KonqMainWindow::breakOffTab(int tabIndex)
{
    KonqFrameBase *tab = m_pViewManager->tabContainer()->tabAt(tabIndex);
    if (!tab) {
        return;
    }
    const int originalTabIndex = m_pViewManager->tabContainer()->currentIndex();
    // TODO: Why do we warn about breaking off a modified tab, since it seems to show the unsubmitted form data just fine?
    if (!KonqModifiedViewsCollector::collect(tab).isEmpty()) {
        m_pViewManager->showTab(tabIndex);
        if (KMessageBox::warningContinueCancel(
                    this,
                    i18n("This tab contains changes that have not been submitted.\nDetaching the tab will discard these changes."),
                    i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"), QStringLiteral("tab-detach")), KStandardGuiItem::cancel(), QStringLiteral("discardchangesdetach")) != KMessageBox::Continue) {
            m_pViewManager->showTab(originalTabIndex);
            return;
        }
    }
    m_pViewManager->showTab(originalTabIndex);
    m_pViewManager->breakOffTab(tabIndex, size());
    updateViewActions();
}

void KonqMainWindow::slotPopupNewWindow()
{
    KFileItemList::const_iterator it = m_popupItems.constBegin();
    const KFileItemList::const_iterator end = m_popupItems.constEnd();
    KonqOpenURLRequest req;
    req.args = m_popupUrlArgs;
    req.browserArgs = m_popupUrlBrowserArgs;
    for (; it != end; ++it) {
        KonqMainWindow *mw = KonqMainWindowFactory::createNewWindow((*it).targetUrl(), req);
        mw->show();
    }
}

void KonqMainWindow::slotPopupThisWindow()
{
    openUrl(0, m_popupItems.first().url());
}

void KonqMainWindow::slotPopupNewTab()
{
    if (m_isPopupWithProxyWindow && !m_popupProxyWindow) {
        slotPopupNewWindow();
        return;
    }
    bool openAfterCurrentPage = KonqSettings::openAfterCurrentPage();
    bool newTabsInFront = KonqSettings::newTabsInFront();

    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        newTabsInFront = !newTabsInFront;
    }

    popupNewTab(newTabsInFront, openAfterCurrentPage);
}

void KonqMainWindow::popupNewTab(bool infront, bool openAfterCurrentPage)
{
    KonqOpenURLRequest req;
    req.newTabInFront = false;
    req.forceAutoEmbed = true;
    req.openAfterCurrentPage = openAfterCurrentPage;
    req.args = m_popupUrlArgs;
    req.browserArgs = m_popupUrlBrowserArgs;
    req.browserArgs.setNewTab(true);

    KonqMainWindow *mainWindow = (m_popupProxyWindow ? m_popupProxyWindow.data() : this);

    for (int i = 0; i < m_popupItems.count(); ++i) {
        if (infront && i == m_popupItems.count() - 1) {
            req.newTabInFront = true;
        }
        mainWindow->openUrl(0, m_popupItems[i].targetUrl(), QString(), req);
    }

    // Raise this window if the request to create the tab came from a popup window.
    if (m_isPopupWithProxyWindow) {
        raiseWindow(mainWindow);
    }
}

void KonqMainWindow::openMultiURL(const QList<QUrl> &url)
{
    QList<QUrl>::ConstIterator it = url.constBegin();
    const QList<QUrl>::ConstIterator end = url.constEnd();
    for (; it != end; ++it) {
        KonqView *newView = m_pViewManager->addTab(QStringLiteral("text/html"));
        Q_ASSERT(newView);
        if (newView == 0) {
            continue;
        }
        openUrl(newView, *it, QString());
        m_pViewManager->showTab(newView);
    }
}

void KonqMainWindow::slotRemoveView()
{
    if (!m_currentView) {
        return;
    }

    if (m_currentView->isModified()) {
        if (KMessageBox::warningContinueCancel(this,
                                               i18n("This view contains changes that have not been submitted.\nClosing the view will discard these changes."),
                                               i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"), QStringLiteral("view-close")), KStandardGuiItem::cancel(), QStringLiteral("discardchangesclose")) != KMessageBox::Continue) {
            return;
        }
    }

    // takes care of choosing the new active view
    m_pViewManager->removeView(m_currentView);
}

void KonqMainWindow::slotRemoveTab()
{
    removeTab(m_pViewManager->tabContainer()->currentIndex());
}

void KonqMainWindow::slotRemoveTabPopup()
{
    // Can't do immediately - may kill the tabbar, and we're in an event path down from it
    QMetaObject::invokeMethod(this, "removeTab", Qt::QueuedConnection, Q_ARG(int, m_workingTab));
}

void KonqMainWindow::removeTab(int tabIndex)
{
    KonqFrameBase *tab = m_pViewManager->tabContainer()->tabAt(tabIndex);
    if (!tab) {
        return;
    }
    const int originalTabIndex = m_pViewManager->tabContainer()->currentIndex();
    if (!KonqModifiedViewsCollector::collect(tab).isEmpty()) {
        m_pViewManager->showTab(tabIndex);
        if (KMessageBox::warningContinueCancel(
                    this,
                    i18n("This tab contains changes that have not been submitted.\nClosing the tab will discard these changes."),
                    i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"), QStringLiteral("tab-close")), KStandardGuiItem::cancel(), QStringLiteral("discardchangesclose")) != KMessageBox::Continue) {
            m_pViewManager->showTab(originalTabIndex);
            return;
        }
    }
    m_pViewManager->showTab(originalTabIndex);
    m_pViewManager->removeTab(tab);
    updateViewActions();
}

void KonqMainWindow::slotRemoveOtherTabs()
{
    removeOtherTabs(m_pViewManager->tabContainer()->currentIndex());
}

void KonqMainWindow::slotRemoveOtherTabsPopup()
{
    // Can't do immediately - kills the tabbar, and we're in an event path down from it
    QMetaObject::invokeMethod(this, "removeOtherTabs", Qt::QueuedConnection, Q_ARG(int, m_workingTab));
}

void KonqMainWindow::removeOtherTabs(int tabToKeep)
{
    if (KMessageBox::warningContinueCancel(
                this,
                i18n("Do you really want to close all other tabs?"),
                i18nc("@title:window", "Close Other Tabs Confirmation"), KGuiItem(i18n("Close &Other Tabs"), QStringLiteral("tab-close-other")),
                KStandardGuiItem::cancel(), QStringLiteral("CloseOtherTabConfirm")) != KMessageBox::Continue) {
        return;
    }

    KonqFrameTabs *tabContainer = m_pViewManager->tabContainer();
    const int originalTabIndex = tabContainer->currentIndex();
    for (int tabIndex = 0; tabIndex < tabContainer->count(); ++tabIndex) {
        if (tabIndex == tabToKeep) {
            continue;
        }
        KonqFrameBase *tab = tabContainer->tabAt(tabIndex);
        if (!KonqModifiedViewsCollector::collect(tab).isEmpty()) {
            m_pViewManager->showTab(tabIndex);
            if (KMessageBox::warningContinueCancel(
                        this,
                        i18n("This tab contains changes that have not been submitted.\nClosing other tabs will discard these changes."),
                        i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"), QStringLiteral("tab-close")),
                        KStandardGuiItem::cancel(), QStringLiteral("discardchangescloseother")) != KMessageBox::Continue) {
                m_pViewManager->showTab(originalTabIndex);
                return;
            }
        }
    }
    m_pViewManager->showTab(originalTabIndex);
    m_pViewManager->removeOtherTabs(tabToKeep);
    updateViewActions();
}

void KonqMainWindow::slotReloadAllTabs()
{
    KonqFrameTabs *tabContainer = m_pViewManager->tabContainer();
    const int originalTabIndex = tabContainer->currentIndex();
    for (int tabIndex = 0; tabIndex < tabContainer->count(); ++tabIndex) {
        KonqFrameBase *tab = tabContainer->tabAt(tabIndex);
        if (!KonqModifiedViewsCollector::collect(tab).isEmpty()) {
            m_pViewManager->showTab(tabIndex);
            if (KMessageBox::warningContinueCancel(this,
                                                   i18n("This tab contains changes that have not been submitted.\nReloading all tabs will discard these changes."),
                                                   i18nc("@title:window", "Discard Changes?"),
                                                   KGuiItem(i18n("&Discard Changes"), QStringLiteral("view-refresh")),
                                                   KStandardGuiItem::cancel(), QStringLiteral("discardchangesreload")) != KMessageBox::Continue) {
                m_pViewManager->showTab(originalTabIndex);
                return;
            }
        }
    }
    m_pViewManager->showTab(originalTabIndex);
    m_pViewManager->reloadAllTabs();
    updateViewActions();
}

void KonqMainWindow::slotActivateNextTab()
{
    m_pViewManager->activateNextTab();
}

void KonqMainWindow::slotActivatePrevTab()
{
    m_pViewManager->activatePrevTab();
}

void KonqMainWindow::slotActivateTab()
{
    m_pViewManager->activateTab(sender()->objectName().rightRef(2).toInt() - 1);
}

void KonqMainWindow::slotDumpDebugInfo()
{
#ifndef NDEBUG
    dumpViewList();
    m_pViewManager->printFullHierarchy();
#endif
}

bool KonqMainWindow::askForTarget(const KLocalizedString &text, QUrl &url)
{
    const QUrl initialUrl = (viewCount() == 2) ? otherView(m_currentView)->url() : m_currentView->url();
    QString label = text.subs(m_currentView->url().toDisplayString(QUrl::PreferLocalFile)).toString();
    KUrlRequesterDialog dlg(initialUrl, label, this);
    dlg.setWindowTitle(i18nc("@title:window", "Enter Target"));
    dlg.urlRequester()->setMode(KFile::File | KFile::ExistingOnly | KFile::Directory);
    if (dlg.exec()) {
        url = dlg.selectedUrl();
        if (url.isValid()) {
            return true;
        } else {
            KMessageBox::error(this, i18n("<qt><b>%1</b> is not valid</qt>", url.url()));
            return false;
        }
    }
    return false;
}

void KonqMainWindow::slotCopyFiles()
{
    QUrl dest;
    if (!askForTarget(ki18n("Copy selected files from %1 to:"), dest)) {
        return;
    }

    KIO::CopyJob *job = KIO::copy(currentURLs(), dest);
    KIO::FileUndoManager::self()->recordCopyJob(job);
    KJobWidgets::setWindow(job, this);
    job->ui()->setAutoErrorHandlingEnabled(true);
}

void KonqMainWindow::slotMoveFiles()
{
    QUrl dest;
    if (!askForTarget(ki18n("Move selected files from %1 to:"), dest)) {
        return;
    }

    KIO::CopyJob *job = KIO::move(currentURLs(), dest);
    KIO::FileUndoManager::self()->recordCopyJob(job);
    KJobWidgets::setWindow(job, this);
    job->ui()->setAutoErrorHandlingEnabled(true);
}

QList<QUrl> KonqMainWindow::currentURLs() const
{
    QList<QUrl> urls;
    if (m_currentView) {
        urls.append(m_currentView->url());
        if (!m_currentView->selectedItems().isEmpty()) { // Return list of selected items only if we have a selection
            urls = m_currentView->selectedItems().urlList();
        }
    }
    return urls;
}

// Only valid if there are one or two views
KonqView *KonqMainWindow::otherView(KonqView *view) const
{
    Q_ASSERT(viewCount() <= 2);
    MapViews::ConstIterator it = m_mapViews.constBegin();
    if ((*it) == view) {
        ++it;
    }
    if (it != m_mapViews.constEnd()) {
        return (*it);
    }
    return 0;
}

void KonqMainWindow::slotUpAboutToShow()
{
    if (!m_currentView) {
        return;
    }

    QMenu *popup = m_paUp->menu();
    popup->clear();

    int i = 0;

    // Use the location bar URL, because in case we display a index.html
    // we want to go up from the dir, not from the index.html
    QUrl u(QUrl::fromUserInput(m_currentView->locationBarURL()));
    u = KIO::upUrl(u);
    while (!u.path().isEmpty()) {
        QAction *action = new QAction(QIcon::fromTheme(KonqPixmapProvider::self()->iconNameFor(u)),
                                      u.toDisplayString(QUrl::PreferLocalFile),
                                      popup);
        action->setData(u);
        popup->addAction(action);

        if (u.path() == QLatin1String("/") || ++i > 10) {
            break;
        }

        u = KIO::upUrl(u);
    }
}

void KonqMainWindow::slotUp()
{
    if (!m_currentView) {
        return;
    }

    Qt::MouseButtons goMouseState = QApplication::mouseButtons();
    Qt::KeyboardModifiers goKeyboardState = QApplication::keyboardModifiers();

    KonqOpenURLRequest req;
    req.browserArgs.setNewTab(true);
    req.forceAutoEmbed = true;

    req.openAfterCurrentPage = KonqSettings::openAfterCurrentPage();
    req.newTabInFront = KonqSettings::newTabsInFront();

    if (goKeyboardState & Qt::ShiftModifier) {
        req.newTabInFront = !req.newTabInFront;
    }

    const QUrl &url = m_currentView->upUrl();
    if (goKeyboardState & Qt::ControlModifier) {
        openFilteredUrl(url.url(), req);
    } else if (goMouseState & Qt::MidButton) {
        if (KonqSettings::mmbOpensTab()) {
            openFilteredUrl(url.url(), req);
        } else {
            KonqMainWindow *mw = KonqMainWindowFactory::createNewWindow(url);
            mw->show();
        }
    } else {
        openFilteredUrl(url.url(), false);
    }
}

void KonqMainWindow::slotUpActivated(QAction *action)
{
    openUrl(0, action->data().value<QUrl>());
}

void KonqMainWindow::slotGoHistoryActivated(int steps)
{
    if (!m_goBuffer) {
        // Only start 1 timer.
        m_goBuffer = steps;
        m_goMouseState = QApplication::mouseButtons();
        m_goKeyboardState = QApplication::keyboardModifiers();
        QTimer::singleShot(0, this, SLOT(slotGoHistoryDelayed()));
    }
}

void KonqMainWindow::slotGoHistoryDelayed()
{
    if (!m_currentView) {
        return;
    }

    bool openAfterCurrentPage = KonqSettings::openAfterCurrentPage();
    bool mmbOpensTab = KonqSettings::mmbOpensTab();
    bool inFront = KonqSettings::newTabsInFront();
    if (m_goKeyboardState & Qt::ShiftModifier) {
        inFront = !inFront;
    }

    if (m_goKeyboardState & Qt::ControlModifier) {
        KonqView *newView = m_pViewManager->addTabFromHistory(m_currentView, m_goBuffer, openAfterCurrentPage);
        if (newView && inFront) {
            m_pViewManager->showTab(newView);
        }
    } else if (m_goMouseState & Qt::MidButton) {
        if (mmbOpensTab) {
            KonqView *newView = m_pViewManager->addTabFromHistory(m_currentView, m_goBuffer, openAfterCurrentPage);
            if (newView && inFront) {
                m_pViewManager->showTab(newView);
            }
        } else {
            KonqMisc::newWindowFromHistory(this->currentView(), m_goBuffer);
        }
    } else {
        m_currentView->go(m_goBuffer);
        makeViewsFollow(m_currentView->url(),
                        KParts::OpenUrlArguments(),
                        KParts::BrowserArguments(),
                        m_currentView->serviceType(),
                        m_currentView);
    }

    m_goBuffer = 0;
    m_goMouseState = Qt::LeftButton;
    m_goKeyboardState = Qt::NoModifier;
}

void KonqMainWindow::slotBackAboutToShow()
{
    m_paBack->menu()->clear();
    if (m_currentView) {
        KonqActions::fillHistoryPopup(m_currentView->history(), m_currentView->historyIndex(), m_paBack->menu(), true, false);
    }
}

/**
 * Fill the closed tabs action menu before it's shown
 */
void KonqMainWindow::slotClosedItemsListAboutToShow()
{
    QMenu *popup = m_paClosedItems->menu();
    // Clear the menu and fill it with a maximum of s_closedItemsListLength number of urls
    popup->clear();
    QAction *clearAction = popup->addAction(i18nc("This menu entry empties the closed items history", "Empty Closed Items History"));
    connect(clearAction, &QAction::triggered, m_pUndoManager, &KonqUndoManager::clearClosedItemsList);
    popup->insertSeparator(static_cast<QAction *>(0));

    QList<KonqClosedItem *>::ConstIterator it = m_pUndoManager->closedItemsList().constBegin();
    const QList<KonqClosedItem *>::ConstIterator end = m_pUndoManager->closedItemsList().constEnd();
    for (int i = 0; it != end && i < s_closedItemsListLength; ++it, ++i) {
        const QString text = QString::number(i) + ' ' + (*it)->title();
        QAction *action = popup->addAction((*it)->icon(), text);
        action->setActionGroup(m_closedItemsGroup);
        action->setData(i);
    }
    KAcceleratorManager::manage(popup);
}

/**
 * Fill the sessions list action menu before it's shown
 */
void KonqMainWindow::slotSessionsListAboutToShow()
{
    QMenu *popup = m_paSessions->menu();
    // Clear the menu and fill it with a maximum of s_closedItemsListLength number of urls
    popup->clear();
    QAction *saveSessionAction = popup->addAction(QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save As..."));
    connect(saveSessionAction, &QAction::triggered, this, &KonqMainWindow::saveCurrentSession);
    QAction *manageSessionsAction = popup->addAction(QIcon::fromTheme(QStringLiteral("view-choose")), i18n("Manage..."));
    connect(manageSessionsAction, &QAction::triggered, this, &KonqMainWindow::manageSessions);
    popup->addSeparator();

    QString dir = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + "sessions/";
    QDirIterator it(dir, QDir::Readable | QDir::NoDotAndDotDot | QDir::Dirs);

    while (it.hasNext()) {
        QFileInfo fileInfo(it.next());

        QAction *action = popup->addAction(KIO::decodeFileName(fileInfo.baseName()));
        action->setActionGroup(m_sessionsGroup);
        action->setData(fileInfo.filePath());
    }
    KAcceleratorManager::manage(popup);
}

void KonqMainWindow::saveCurrentSession()
{
    KonqNewSessionDlg dlg(this);
    dlg.exec();
}

void KonqMainWindow::manageSessions()
{
    KonqSessionDlg dlg(m_pViewManager, this);
    dlg.exec();
}

void KonqMainWindow::slotSessionActivated(QAction *action)
{
    QString dirpath = action->data().toString();
    KonqSessionManager::self()->restoreSessions(dirpath);
}

void KonqMainWindow::updateClosedItemsAction()
{
    bool available = m_pUndoManager->undoAvailable();
    m_paClosedItems->setEnabled(available);
    m_paUndo->setText(m_pUndoManager->undoText());
}

void KonqMainWindow::slotBack()
{
    slotGoHistoryActivated(-1);
}

void KonqMainWindow::slotBackActivated(QAction *action)
{
    slotGoHistoryActivated(action->data().toInt());
}

void KonqMainWindow::slotForwardAboutToShow()
{
    m_paForward->menu()->clear();
    if (m_currentView) {
        KonqActions::fillHistoryPopup(m_currentView->history(), m_currentView->historyIndex(), m_paForward->menu(), false, true);
    }
}

void KonqMainWindow::slotForward()
{
    slotGoHistoryActivated(1);
}

void KonqMainWindow::slotForwardActivated(QAction *action)
{
    slotGoHistoryActivated(action->data().toInt());
}

void KonqMainWindow::checkDisableClearButton()
{
    // if the location toolbar already has the clear_location action,
    // disable the combobox's embedded clear icon.
    KToolBar *ltb = toolBar(QStringLiteral("locationToolBar"));
    QAction *clearAction = action("clear_location");
    bool enable = true;
    foreach (QToolButton *atb, qFindChildren<QToolButton *>(ltb)) {
        if (atb->defaultAction() == clearAction) {
            enable = false;
            break;
        }
    }
    KLineEdit *lineEdit = qobject_cast<KLineEdit *>(comboEdit());
    if (lineEdit) {
        lineEdit->setClearButtonShown(enable);
    }
}

void KonqMainWindow::initCombo()
{
    m_combo = new KonqCombo(0);

    m_combo->init(s_pCompletion);

    connect(m_combo, SIGNAL(activated(QString,Qt::KeyboardModifiers)),
            this, SLOT(slotURLEntered(QString,Qt::KeyboardModifiers)));
    connect(m_combo, SIGNAL(showPageSecurity()),
            this, SLOT(showPageSecurity()));

    m_pURLCompletion = new KUrlCompletion();
    m_pURLCompletion->setCompletionMode(s_pCompletion->completionMode());

    // This only turns completion off. ~ is still there in the result
    // We do want completion of user names, right?
    //m_pURLCompletion->setReplaceHome( false );  // Leave ~ alone! Will be taken care of by filters!!

    connect(m_combo, SIGNAL(completionModeChanged(KCompletion::CompletionMode)),
            SLOT(slotCompletionModeChanged(KCompletion::CompletionMode)));
    connect(m_combo, SIGNAL(completion(QString)),
            SLOT(slotMakeCompletion(QString)));
    connect(m_combo, SIGNAL(substringCompletion(QString)),
            SLOT(slotSubstringcompletion(QString)));
    connect(m_combo, SIGNAL(textRotation(KCompletionBase::KeyBindingType)),
            SLOT(slotRotation(KCompletionBase::KeyBindingType)));
    connect(m_combo, SIGNAL(cleared()),
            SLOT(slotClearHistory()));
    connect(m_pURLCompletion, SIGNAL(match(QString)),
            SLOT(slotMatch(QString)));

    m_combo->installEventFilter(this);

    static bool bookmarkCompletionInitialized = false;
    if (!bookmarkCompletionInitialized) {
        bookmarkCompletionInitialized = true;
        DelayedInitializer *initializer = new DelayedInitializer(QEvent::KeyPress, m_combo);
        connect(initializer, &DelayedInitializer::initialize, this, &KonqMainWindow::bookmarksIntoCompletion);
    }
}

void KonqMainWindow::bookmarksIntoCompletion()
{
    // add all bookmarks to the completion list for easy access
    addBookmarksIntoCompletion(s_bookmarkManager->root());
}

// the user changed the completion mode in the combo
void KonqMainWindow::slotCompletionModeChanged(KCompletion::CompletionMode m)
{
    s_pCompletion->setCompletionMode(m);

    KonqSettings::setSettingsCompletionMode(int(m_combo->completionMode()));
    KonqSettings::self()->save();

    // tell the other windows too (only this instance currently)
    foreach (KonqMainWindow *window, *s_lstViews) {
        if (window && window->m_combo) {
            window->m_combo->setCompletionMode(m);
            window->m_pURLCompletion->setCompletionMode(m);
        }
    }
}

// at first, try to find a completion in the current view, then use the global
// completion (history)
void KonqMainWindow::slotMakeCompletion(const QString &text)
{
    if (m_pURLCompletion) {
        m_urlCompletionStarted = true; // flag for slotMatch()

        // qDebug() << "Local Completion object found!";
        QString completion = m_pURLCompletion->makeCompletion(text);
        m_currentDir.clear();

        if (completion.isNull() && !m_pURLCompletion->isRunning()) {
            // No match() signal will come from m_pURLCompletion
            // ask the global one
            // tell the static completion object about the current completion mode
            completion = s_pCompletion->makeCompletion(text);

            // some special handling necessary for CompletionPopup
            if (m_combo->completionMode() == KCompletion::CompletionPopup ||
                    m_combo->completionMode() == KCompletion::CompletionPopupAuto) {
                m_combo->setCompletedItems(historyPopupCompletionItems(text));
            }

            else if (!completion.isNull()) {
                m_combo->setCompletedText(completion);
            }
        } else {
            // To be continued in slotMatch()...
            if (!m_pURLCompletion->dir().isEmpty()) {
                m_currentDir = m_pURLCompletion->dir();
            }
        }
    }
    // qDebug() << "Current dir:" << m_currentDir << "Current text:" << text;
}

void KonqMainWindow::slotSubstringcompletion(const QString &text)
{
    if (!m_currentView) {
        return;
    }

    QString currentURL = m_currentView->url().toDisplayString();
    bool filesFirst = currentURL.startsWith('/') ||
                      currentURL.startsWith(QLatin1String("file:/"));
    QStringList items;
    if (filesFirst && m_pURLCompletion) {
        items = m_pURLCompletion->substringCompletion(text);
    }

    items += s_pCompletion->substringCompletion(text);
    if (!filesFirst && m_pURLCompletion) {
        items += m_pURLCompletion->substringCompletion(text);
    }

    m_combo->setCompletedItems(items);
}

void KonqMainWindow::slotRotation(KCompletionBase::KeyBindingType type)
{
    // Tell slotMatch() to do nothing
    m_urlCompletionStarted = false;

    bool prev = (type == KCompletionBase::PrevCompletionMatch);
    if (prev || type == KCompletionBase::NextCompletionMatch) {
        QString completion = prev ? m_pURLCompletion->previousMatch() :
                             m_pURLCompletion->nextMatch();

        if (completion.isNull()) {  // try the history KCompletion object
            completion = prev ? s_pCompletion->previousMatch() :
                         s_pCompletion->nextMatch();
        }
        if (completion.isEmpty() || completion == m_combo->currentText()) {
            return;
        }

        m_combo->setCompletedText(completion);
    }
}

// Handle match() from m_pURLCompletion
void KonqMainWindow::slotMatch(const QString &match)
{
    if (match.isEmpty() || !m_combo) {
        return;
    }

    // Check flag to avoid match() raised by rotation
    if (m_urlCompletionStarted) {
        m_urlCompletionStarted = false;

        // some special handling necessary for CompletionPopup
        if (m_combo->completionMode() == KCompletion::CompletionPopup ||
                m_combo->completionMode() == KCompletion::CompletionPopupAuto) {
            QStringList items = m_pURLCompletion->allMatches();
            items += historyPopupCompletionItems(m_combo->currentText());
            items.removeDuplicates();  // when items from completion are also in history
            // items.sort(); // should we?
            m_combo->setCompletedItems(items);
        } else if (!match.isNull()) {
            m_combo->setCompletedText(match);
        }
    }
}

void KonqMainWindow::slotCtrlTabPressed()
{
    KonqView *view = m_pViewManager->chooseNextView(m_currentView);
    //qDebug() << m_currentView->url() << "->" << view->url();
    if (view) {
        m_pViewManager->setActivePart(view->part());
        KonqFrameTabs *tabs = m_pViewManager->tabContainer();
        m_pViewManager->showTab(tabs->tabIndexContaining(view->frame()));
    }
}

void KonqMainWindow::slotClearHistory()
{
    KonqHistoryManager::kself()->emitClear();
}

void KonqMainWindow::slotClearComboHistory()
{
    if (m_combo && m_combo->count()) {
        m_combo->clearHistory();
    }
}

bool KonqMainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    if ((ev->type() == QEvent::FocusIn || ev->type() == QEvent::FocusOut) &&
            m_combo && m_combo->lineEdit() && m_combo == obj) {
        //qDebug() << obj << obj->metaObject()->className() << obj->name();

        QFocusEvent *focusEv = static_cast<QFocusEvent *>(ev);
        if (focusEv->reason() == Qt::PopupFocusReason) {
            return KParts::MainWindow::eventFilter(obj, ev);
        }

        KParts::BrowserExtension *ext = 0;
        if (m_currentView) {
            ext = m_currentView->browserExtension();
        }

        if (ev->type() == QEvent::FocusIn) {
            //qDebug() << "ComboBox got the focus...";
            if (m_bLocationBarConnected) {
                //qDebug() << "Was already connected...";
                return KParts::MainWindow::eventFilter(obj, ev);
            }
            m_bLocationBarConnected = true;

            // Workaround for Qt issue: usually, QLineEdit reacts on Ctrl-D,
            // but the duplicatecurrenttab action also has Ctrl-D as accel and
            // prevents the lineedit from getting this event. IMHO the accel
            // should be disabled in favor of the focus-widget.
            // TODO: decide if the delete-character behaviour of QLineEdit
            // really is useful enough to warrant this workaround
            QAction *duplicate = actionCollection()->action(QStringLiteral("duplicatecurrenttab"));
            if (duplicate->shortcuts().contains(QKeySequence(Qt::CTRL + Qt::Key_D))) {
                duplicate->setEnabled(false);
            }

            connect(m_paCut, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(cut()));
            connect(m_paCopy, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(copy()));
            connect(m_paPaste, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(paste()));
            connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()));
            connect(m_combo->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(slotCheckComboSelection()));
            connect(m_combo->lineEdit(), SIGNAL(selectionChanged()), this, SLOT(slotCheckComboSelection()));

            slotClipboardDataChanged();
        } else if (ev->type() == QEvent::FocusOut) {
            //qDebug() << "ComboBox lost focus...";
            if (!m_bLocationBarConnected) {
                //qDebug() << "Was already disconnected...";
                return KParts::MainWindow::eventFilter(obj, ev);
            }
            m_bLocationBarConnected = false;

            // see above in FocusIn for explanation
            // action is reenabled if a view exists
            QAction *duplicate = actionCollection()->action(QStringLiteral("duplicatecurrenttab"));
            if (duplicate->shortcuts().contains(QKeySequence(Qt::CTRL + Qt::Key_D))) {
                duplicate->setEnabled(currentView() && currentView()->frame());
            }

            disconnect(m_paCut, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(cut()));
            disconnect(m_paCopy, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(copy()));
            disconnect(m_paPaste, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(paste()));
            disconnect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()));
            disconnect(m_combo->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(slotCheckComboSelection()));
            disconnect(m_combo->lineEdit(), SIGNAL(selectionChanged()), this, SLOT(slotCheckComboSelection()));

            if (ext) {
                m_paCut->setEnabled(ext->isActionEnabled("cut"));
                m_paCopy->setEnabled(ext->isActionEnabled("copy"));
                m_paPaste->setEnabled(ext->isActionEnabled("paste"));
            } else {
                m_paCut->setEnabled(false);
                m_paCopy->setEnabled(false);
                m_paPaste->setEnabled(false);
            }
        }
    } else if (ev->type() == QEvent::KeyPress) {
        QKeyEvent *keyEv = static_cast<QKeyEvent *>(ev);
        if ((keyEv->key() == Qt::Key_Tab) && (keyEv->modifiers() == Qt::ControlModifier)) {
            slotCtrlTabPressed();
            return true; // don't let QTabWidget see the event
        } else if (obj == m_combo && m_currentView && keyEv->key() == Qt::Key_Escape) {
            // reset url to current view's actual url on ESC
            m_combo->setURL(m_currentView->url().QUrl::toDisplayString(QUrl::PreferLocalFile));
            m_combo->lineEdit()->setModified(false);
            return true;
        }
    }
    return KParts::MainWindow::eventFilter(obj, ev);
}

// Only called when m_bLocationBarConnected, i.e. when the combobox has focus.
// The rest of the time, the part handles the cut/copy/paste actions.
void KonqMainWindow::slotClipboardDataChanged()
{
    const QMimeData *data = QApplication::clipboard()->mimeData();
    m_paPaste->setEnabled(data->hasText());
    slotCheckComboSelection();
}

void KonqMainWindow::slotCheckComboSelection()
{
    QLineEdit *edit = comboEdit();
    if (edit) {
        const bool hasSelection = edit->hasSelectedText();
        //qDebug() << "m_combo->lineEdit()->hasMarkedText():" << hasSelection;
        m_paCopy->setEnabled(hasSelection);
        m_paCut->setEnabled(hasSelection);
    }
}

void KonqMainWindow::slotClearLocationBar()
{
    slotStop();
    if (m_combo) {
        m_combo->clearTemporary();
    }
    focusLocationBar();
}

void KonqMainWindow::slotForceSaveMainWindowSettings()
{
    if (autoSaveSettings()) {   // don't do it on e.g. JS window.open windows with no toolbars!
        saveAutoSaveSettings();
    }
}

void KonqMainWindow::slotShowMenuBar()
{
    menuBar()->setVisible(!menuBar()->isVisible());
    slotForceSaveMainWindowSettings();
}

void KonqMainWindow::slotShowStatusBar()
{
    if (m_currentView) {
        m_currentView->frame()->statusbar()->setVisible(m_paShowStatusBar->isChecked());
    }

    // An alternative: this will change simultaneously all of the status bars on
    // all of the current views.
    //MapViews::const_iterator end = m_mapViews.constEnd();
    //for (MapViews::const_iterator it = m_mapViews.constBegin(); it != end; ++it) {
    //  KonqView* view = it.value();
    //  view->frame()->statusbar()->setVisible(on);
    //}

    slotForceSaveMainWindowSettings();
}

void KonqMainWindow::slotUpdateFullScreen(bool set)
{
    KToggleFullScreenAction::setFullScreen(this, set);
    if (set) {
        // Create toolbar button for exiting from full-screen mode
        // ...but only if there isn't one already...

        bool haveFullScreenButton = false;

        //Walk over the toolbars and check whether there is a show fullscreen button in any of them
        foreach (KToolBar *bar, findChildren<KToolBar *>()) {
            //Are we plugged here, in a visible toolbar?
            if (bar->isVisible() &&
                    action("fullscreen")->associatedWidgets().contains(bar)) {
                haveFullScreenButton = true;
                break;
            }
        }

        if (!haveFullScreenButton) {
            QList<QAction *> lst;
            lst.append(m_ptaFullScreen);
            plugActionList(QStringLiteral("fullscreen"), lst);
        }

        m_prevMenuBarVisible = menuBar()->isVisible();
        menuBar()->hide();
        m_paShowMenuBar->setChecked(false);
    } else {
        unplugActionList(QStringLiteral("fullscreen"));

        if (m_prevMenuBarVisible) {
            menuBar()->show();
            m_paShowMenuBar->setChecked(true);
        }
    }
}

void KonqMainWindow::setLocationBarURL(const QUrl &url)
{
    setLocationBarURL(url.toString());
}

void KonqMainWindow::setLocationBarURL(const QString &url)
{
    // Don't set the location bar URL if it hasn't changed
    // or if the user had time to edit the url since the last call to openUrl (#64868)
    QLineEdit *edit = comboEdit();
    if (edit && url != edit->text() && !edit->isModified()) {
        //qDebug() << "url=" << url;
        m_combo->setURL(url);
        updateWindowIcon();
    }
}

void KonqMainWindow::setPageSecurity(PageSecurity pageSecurity)
{
    if (m_combo) {
        m_combo->setPageSecurity(pageSecurity);
    }
}

void KonqMainWindow::showPageSecurity()
{
    if (m_currentView && m_currentView->part()) {
        QAction *act = m_currentView->part()->action("security");
        if (act) {
            act->trigger();
        }
    }
}

// Called via DBUS from KonquerorApplication
void KonqMainWindow::comboAction(int action, const QString &url, const QString &senderId)
{
    if (!s_lstViews) { // this happens in "konqueror --silent"
        return;
    }

    KonqCombo *combo = 0;
    foreach (KonqMainWindow *window, *s_lstViews) {
        if (window && window->m_combo) {
            combo = window->m_combo;

            switch (action) {
            case ComboAdd:
                combo->insertPermanent(url);
                break;
            case ComboClear:
                combo->clearHistory();
                break;
            case ComboRemove:
                combo->removeURL(url);
                break;
            default:
                break;
            }
        }
    }

    // only one instance should save...
    if (combo && senderId == QDBusConnection::sessionBus().baseService()) {
        combo->saveItems();
    }
}

QString KonqMainWindow::locationBarURL() const
{
    return (m_combo ? m_combo->currentText() : QString());
}

void KonqMainWindow::focusLocationBar()
{
    if (m_combo && (m_combo->isVisible() || !isVisible())) {
        m_combo->setFocus();
    }
}

void KonqMainWindow::startAnimation()
{
    m_paAnimatedLogo->start();
    m_paStop->setEnabled(true);
}

void KonqMainWindow::stopAnimation()
{
    m_paAnimatedLogo->stop();
    m_paStop->setEnabled(false);
}

void KonqMainWindow::setUpEnabled(const QUrl &url)
{
    bool bHasUpURL = ((!url.path().isEmpty() && url.path() != QLatin1String("/") && url.path()[0] == '/')
                      || !url.query().isEmpty() /*e.g. lists.kde.org*/);

    m_paUp->setEnabled(bHasUpURL);
}

void KonqMainWindow::initActions()
{
    // Note about this method : don't call setEnabled() on any of the actions.
    // They are all disabled then re-enabled with enableAllActions
    // If any one needs to be initially disabled, put that code in enableAllActions

    // For the popup menu only.
    m_pMenuNew = new KNewFileMenu(actionCollection(), QStringLiteral("new_menu"), this);

    // File menu

    QAction *action = actionCollection()->addAction(QStringLiteral("new_window"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("window-new")));
    action->setText(i18n("New &Window"));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotNewWindow);
    actionCollection()->setDefaultShortcuts(action, KStandardShortcut::shortcut(KStandardShortcut::New));
    action = actionCollection()->addAction(QStringLiteral("duplicate_window"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("window-duplicate")));
    action->setText(i18n("&Duplicate Window"));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotDuplicateWindow);
    actionCollection()->setDefaultShortcut(action, Qt::CTRL+Qt::SHIFT+Qt::Key_D);
    action = actionCollection()->addAction(QStringLiteral("sendURL"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("mail-message-new")));
    action->setText(i18n("Send &Link Address..."));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotSendURL);
    action = actionCollection()->addAction(QStringLiteral("sendPage"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("mail-message-new")));
    action->setText(i18n("S&end File..."));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotSendFile);
    action = actionCollection()->addAction(QStringLiteral("open_location"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("document-open-remote")));
    action->setText(i18n("&Open Location"));
    actionCollection()->setDefaultShortcut(action, Qt::ALT+Qt::Key_O);
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotOpenLocation);

    action = actionCollection()->addAction(QStringLiteral("open_file"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
    action->setText(i18n("&Open File..."));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotOpenFile);
    actionCollection()->setDefaultShortcuts(action, KStandardShortcut::shortcut(KStandardShortcut::Open));

#if 0
    m_paFindFiles = new KToggleAction(QIcon::fromTheme("edit-find"), i18n("&Find File..."), this);
    actionCollection()->addAction("findfile", m_paFindFiles);
    connect(m_paFindFiles, &KToggleAction::triggered, this, &KonqMainWindow::slotToolFind);
    actionCollection()->setDefaultShortcuts(m_paFindFiles, KStandardShortcut::shortcut(KStandardShortcut::Find));
#endif

    m_paPrint = actionCollection()->addAction(KStandardAction::Print, QStringLiteral("print"), 0, 0);
    actionCollection()->addAction(KStandardAction::Quit, QStringLiteral("quit"), this, SLOT(close()));

    m_paLockView = new KToggleAction(i18n("Lock to Current Location"), this);
    actionCollection()->addAction(QStringLiteral("lock"), m_paLockView);
    connect(m_paLockView, &KToggleAction::triggered, this, &KonqMainWindow::slotLockView);
    m_paLinkView = new KToggleAction(i18nc("This option links konqueror views", "Lin&k View"), this);
    actionCollection()->addAction(QStringLiteral("link"), m_paLinkView);
    connect(m_paLinkView, &KToggleAction::triggered, this, &KonqMainWindow::slotLinkView);

    // Go menu
    m_paUp = new KToolBarPopupAction(QIcon::fromTheme(QStringLiteral("go-up")), i18n("&Up"), this);
    actionCollection()->addAction(QStringLiteral("go_up"), m_paUp);
    actionCollection()->setDefaultShortcuts(m_paUp, KStandardShortcut::shortcut(KStandardShortcut::Up));
    connect(m_paUp, SIGNAL(triggered()), this, SLOT(slotUp()));
    connect(m_paUp->menu(), SIGNAL(aboutToShow()), this, SLOT(slotUpAboutToShow()));
    connect(m_paUp->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotUpActivated(QAction*)));

    QPair< KGuiItem, KGuiItem > backForward = KStandardGuiItem::backAndForward();

    // Trash bin of closed tabs
    m_paClosedItems = new KToolBarPopupAction(QIcon::fromTheme(QStringLiteral("edit-undo-closed-tabs")),  i18n("Closed Items"), this);
    actionCollection()->addAction(QStringLiteral("closeditems"), m_paClosedItems);
    m_closedItemsGroup = new QActionGroup(m_paClosedItems->menu());

    // set the closed tabs list shown
    connect(m_paClosedItems, &KToolBarPopupAction::triggered, m_pUndoManager, &KonqUndoManager::undoLastClosedItem);
    connect(m_paClosedItems->menu(), SIGNAL(aboutToShow()), this, SLOT(slotClosedItemsListAboutToShow()));
    connect(m_closedItemsGroup, &QActionGroup::triggered, m_pUndoManager, &KonqUndoManager::slotClosedItemsActivated);
    connect(m_pViewManager, &KonqViewManager::aboutToRemoveTab, this, &KonqMainWindow::slotAddClosedUrl);
    connect(m_pUndoManager, &KonqUndoManager::openClosedTab, m_pViewManager, &KonqViewManager::openClosedTab);
    connect(m_pUndoManager, &KonqUndoManager::openClosedWindow, m_pViewManager, &KonqViewManager::openClosedWindow);
    connect(m_pUndoManager, &KonqUndoManager::closedItemsListChanged, this, &KonqMainWindow::updateClosedItemsAction);

    m_paSessions = new KActionMenu(i18n("Sessions"), this);
    actionCollection()->addAction(QStringLiteral("sessions"), m_paSessions);
    m_sessionsGroup = new QActionGroup(m_paSessions->menu());
    connect(m_paSessions->menu(), SIGNAL(aboutToShow()), this, SLOT(slotSessionsListAboutToShow()));
    connect(m_sessionsGroup, &QActionGroup::triggered, this, &KonqMainWindow::slotSessionActivated);

    m_paBack = new KToolBarPopupAction(QIcon::fromTheme(backForward.first.iconName()), backForward.first.text(), this);
    actionCollection()->addAction(QStringLiteral("go_back"), m_paBack);
    actionCollection()->setDefaultShortcuts(m_paBack, KStandardShortcut::shortcut(KStandardShortcut::Back));
    connect(m_paBack, SIGNAL(triggered()), this, SLOT(slotBack()));
    connect(m_paBack->menu(), SIGNAL(aboutToShow()), this, SLOT(slotBackAboutToShow()));
    connect(m_paBack->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotBackActivated(QAction*)));

    m_paForward = new KToolBarPopupAction(QIcon::fromTheme(backForward.second.iconName()), backForward.second.text(), this);
    actionCollection()->addAction(QStringLiteral("go_forward"), m_paForward);
    actionCollection()->setDefaultShortcuts(m_paForward, KStandardShortcut::shortcut(KStandardShortcut::Forward));
    connect(m_paForward, SIGNAL(triggered()), this, SLOT(slotForward()));
    connect(m_paForward->menu(), SIGNAL(aboutToShow()), this, SLOT(slotForwardAboutToShow()));
    connect(m_paForward->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotForwardActivated(QAction*)));

    m_paHome = actionCollection()->addAction(KStandardAction::Home);
    connect(m_paHome, SIGNAL(triggered(bool)), this, SLOT(slotHome()));
    m_paHomePopup = new KToolBarPopupAction (QIcon::fromTheme(QStringLiteral("go-home")), i18n("Home"), this);
    actionCollection()->addAction(QStringLiteral("go_home_popup"), m_paHomePopup);
    connect(m_paHomePopup, SIGNAL(triggered()), this, SLOT(slotHome()));
    connect(m_paHomePopup->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotHomePopupActivated(QAction*)));

    KonqMostOftenURLSAction *mostOften = new KonqMostOftenURLSAction(i18nc("@action:inmenu Go", "Most Often Visited"), this);
    actionCollection()->addAction(QStringLiteral("go_most_often"), mostOften);
    connect(mostOften, &KonqMostOftenURLSAction::activated, this, &KonqMainWindow::slotOpenURL);

    KonqHistoryAction *historyAction = new KonqHistoryAction(i18nc("@action:inmenu Go", "Recently Visited"), this);
    actionCollection()->addAction(QStringLiteral("history"), historyAction);
    connect(historyAction, &KonqHistoryAction::activated, this, &KonqMainWindow::slotOpenURL);

    action = actionCollection()->addAction(QStringLiteral("go_history"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-history")));
    // Ctrl+Shift+H, shortcut from firefox
    // TODO: and Ctrl+H should open the sidebar history module
    actionCollection()->setDefaultShortcut(action, Qt::CTRL+Qt::SHIFT+Qt::Key_H);
    action->setText(i18nc("@action:inmenu Go", "Show History"));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotGoHistory);

    // Settings menu

    // This list is just for the call to authorizeControlModule; see slotConfigure for the real code
    QStringList configureModules;
    configureModules << QStringLiteral("khtml_general") << QStringLiteral("bookmarks") <<
                     QStringLiteral("filebehavior") << QStringLiteral("filetypes") << QStringLiteral("kcmtrash") <<
                     QStringLiteral("khtml_appearance") << QStringLiteral("khtml_behavior") << QStringLiteral("khtml_java_js") <<
                     QStringLiteral("khtml_filter") << QStringLiteral("ebrowsing") <<
                     QStringLiteral("kcmhistory") << QStringLiteral("cookies") <<
                     QStringLiteral("cache") << QStringLiteral("proxy") <<
                     QStringLiteral("crypto") << QStringLiteral("useragent") <<
                     QStringLiteral("khtml_plugins") << QStringLiteral("kcmkonqyperformance");

    if (!KAuthorized::authorizeControlModules(configureModules).isEmpty()) {
        actionCollection()->addAction(KStandardAction::Preferences, this, SLOT(slotConfigure()));
    }

    actionCollection()->addAction(KStandardAction::KeyBindings, guiFactory(), SLOT(configureShortcuts()));
    actionCollection()->addAction(KStandardAction::ConfigureToolbars, this, SLOT(slotConfigureToolbars()));

    m_paConfigureExtensions = actionCollection()->addAction(QStringLiteral("options_configure_extensions"));
    m_paConfigureExtensions->setText(i18n("Configure Extensions..."));
    connect(m_paConfigureExtensions, &QAction::triggered, this, &KonqMainWindow::slotConfigureExtensions);
    m_paConfigureSpellChecking = actionCollection()->addAction(QStringLiteral("configurespellcheck"));
    m_paConfigureSpellChecking->setIcon(QIcon::fromTheme(QStringLiteral("tools-check-spelling")));
    m_paConfigureSpellChecking->setText(i18n("Configure Spell Checking..."));
    connect(m_paConfigureSpellChecking, &QAction::triggered, this, &KonqMainWindow::slotConfigureSpellChecking);

    // Window menu
    m_paSplitViewHor = actionCollection()->addAction(QStringLiteral("splitviewh"));
    m_paSplitViewHor->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    m_paSplitViewHor->setText(i18n("Split View &Left/Right"));
    connect(m_paSplitViewHor, &QAction::triggered, this, &KonqMainWindow::slotSplitViewHorizontal);
    actionCollection()->setDefaultShortcut(m_paSplitViewHor, Qt::CTRL+Qt::SHIFT+Qt::Key_L);
    m_paSplitViewVer = actionCollection()->addAction(QStringLiteral("splitviewv"));
    m_paSplitViewVer->setIcon(QIcon::fromTheme(QStringLiteral("view-split-top-bottom")));
    m_paSplitViewVer->setText(i18n("Split View &Top/Bottom"));
    connect(m_paSplitViewVer, &QAction::triggered, this, &KonqMainWindow::slotSplitViewVertical);
    actionCollection()->setDefaultShortcut(m_paSplitViewVer, Qt::CTRL+Qt::SHIFT+Qt::Key_T);
    m_paAddTab = actionCollection()->addAction(QStringLiteral("newtab"));
    m_paAddTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    m_paAddTab->setText(i18n("&New Tab"));
    connect(m_paAddTab, &QAction::triggered, this, &KonqMainWindow::slotAddTab);
    QList<QKeySequence> addTabShortcuts;
    addTabShortcuts.append(QKeySequence(Qt::CTRL+Qt::Key_T));
    addTabShortcuts.append(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_N));
    actionCollection()->setDefaultShortcuts(m_paAddTab, addTabShortcuts);

    m_paDuplicateTab = actionCollection()->addAction(QStringLiteral("duplicatecurrenttab"));
    m_paDuplicateTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-duplicate")));
    m_paDuplicateTab->setText(i18n("&Duplicate Current Tab"));
    connect(m_paDuplicateTab, &QAction::triggered, this, &KonqMainWindow::slotDuplicateTab);
    actionCollection()->setDefaultShortcut(m_paDuplicateTab, Qt::CTRL+Qt::Key_D);
    m_paBreakOffTab = actionCollection()->addAction(QStringLiteral("breakoffcurrenttab"));
    m_paBreakOffTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-detach")));
    m_paBreakOffTab->setText(i18n("Detach Current Tab"));
    connect(m_paBreakOffTab, &QAction::triggered, this, &KonqMainWindow::slotBreakOffTab);
    actionCollection()->setDefaultShortcut(m_paBreakOffTab, Qt::CTRL+Qt::SHIFT+Qt::Key_B);
    m_paRemoveView = actionCollection()->addAction(QStringLiteral("removeview"));
    m_paRemoveView->setIcon(QIcon::fromTheme(QStringLiteral("view-close")));
    m_paRemoveView->setText(i18n("&Close Active View"));
    connect(m_paRemoveView, &QAction::triggered, this, &KonqMainWindow::slotRemoveView);
    actionCollection()->setDefaultShortcut(m_paRemoveView, Qt::CTRL+Qt::SHIFT+Qt::Key_W);
    m_paRemoveTab = actionCollection()->addAction(QStringLiteral("removecurrenttab"));
    m_paRemoveTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-close")));
    m_paRemoveTab->setText(i18n("Close Current Tab"));
    connect(m_paRemoveTab, &QAction::triggered, this, &KonqMainWindow::slotRemoveTab, Qt::QueuedConnection /* exit Ctrl+W handler before deleting */);
    actionCollection()->setDefaultShortcut(m_paRemoveTab, Qt::CTRL+Qt::Key_W);
    m_paRemoveTab->setAutoRepeat(false);
    m_paRemoveOtherTabs = actionCollection()->addAction(QStringLiteral("removeothertabs"));
    m_paRemoveOtherTabs->setIcon(QIcon::fromTheme(QStringLiteral("tab-close-other")));
    m_paRemoveOtherTabs->setText(i18n("Close &Other Tabs"));
    connect(m_paRemoveOtherTabs, &QAction::triggered, this, &KonqMainWindow::slotRemoveOtherTabs);

    m_paActivateNextTab = actionCollection()->addAction(QStringLiteral("activatenexttab"));
    m_paActivateNextTab->setText(i18n("Activate Next Tab"));
    connect(m_paActivateNextTab, &QAction::triggered, this, &KonqMainWindow::slotActivateNextTab);
    actionCollection()->setDefaultShortcuts(m_paActivateNextTab, QApplication::isRightToLeft() ? KStandardShortcut::tabPrev() : KStandardShortcut::tabNext());
    m_paActivatePrevTab = actionCollection()->addAction(QStringLiteral("activateprevtab"));
    m_paActivatePrevTab->setText(i18n("Activate Previous Tab"));
    connect(m_paActivatePrevTab, &QAction::triggered, this, &KonqMainWindow::slotActivatePrevTab);
    actionCollection()->setDefaultShortcuts(m_paActivatePrevTab, QApplication::isRightToLeft() ? KStandardShortcut::tabNext() : KStandardShortcut::tabPrev());

    QString actionname;
    for (int i = 1; i < 13; i++) {
        actionname.sprintf("activate_tab_%02d", i);
        QAction *action = actionCollection()->addAction(actionname);
        action->setText(i18n("Activate Tab %1", i));
        connect(action, &QAction::triggered, this, &KonqMainWindow::slotActivateTab);
    }

    m_paMoveTabLeft = actionCollection()->addAction(QStringLiteral("tab_move_left"));
    m_paMoveTabLeft->setText(i18n("Move Tab Left"));
    m_paMoveTabLeft->setIcon(QIcon::fromTheme(QStringLiteral("arrow-left")));
    connect(m_paMoveTabLeft, &QAction::triggered, this, &KonqMainWindow::slotMoveTabLeft);
    actionCollection()->setDefaultShortcut(m_paMoveTabLeft, Qt::CTRL+Qt::SHIFT+Qt::Key_Left);
    m_paMoveTabRight = actionCollection()->addAction(QStringLiteral("tab_move_right"));
    m_paMoveTabRight->setText(i18n("Move Tab Right"));
    m_paMoveTabRight->setIcon(QIcon::fromTheme(QStringLiteral("arrow-right")));
    connect(m_paMoveTabRight, &QAction::triggered, this, &KonqMainWindow::slotMoveTabRight);
    actionCollection()->setDefaultShortcut(m_paMoveTabRight, Qt::CTRL+Qt::SHIFT+Qt::Key_Right);

#ifndef NDEBUG
    action = actionCollection()->addAction(QStringLiteral("dumpdebuginfo"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-dump-debug-info")));
    action->setText(i18n("Dump Debug Info"));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotDumpDebugInfo);
#endif

    m_ptaFullScreen = KStandardAction::fullScreen(0, 0, this, this);
    actionCollection()->addAction(m_ptaFullScreen->objectName(), m_ptaFullScreen);
    QList<QKeySequence> fullScreenShortcut = m_ptaFullScreen->shortcuts();
    fullScreenShortcut.append(Qt::Key_F11);
    actionCollection()->setDefaultShortcuts(m_ptaFullScreen, fullScreenShortcut);
    connect(m_ptaFullScreen, &KToggleFullScreenAction::toggled, this, &KonqMainWindow::slotUpdateFullScreen);

    QList<QKeySequence> reloadShortcut = KStandardShortcut::shortcut(KStandardShortcut::Reload);
    QKeySequence reloadAlternate(Qt::CTRL + Qt::Key_R);
    if (!reloadShortcut.contains(reloadAlternate)) {
        reloadShortcut.append(reloadAlternate);
    }
    m_paReload = actionCollection()->addAction(QStringLiteral("reload"));
    m_paReload->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    m_paReload->setText(i18n("&Reload"));
    connect(m_paReload, SIGNAL(triggered()), SLOT(slotReload()));
    actionCollection()->setDefaultShortcuts(m_paReload, reloadShortcut);
    m_paReloadAllTabs = actionCollection()->addAction(QStringLiteral("reload_all_tabs"));
    m_paReloadAllTabs->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh-all")));
    m_paReloadAllTabs->setText(i18n("&Reload All Tabs"));
    connect(m_paReloadAllTabs, &QAction::triggered, this, &KonqMainWindow::slotReloadAllTabs);
    actionCollection()->setDefaultShortcut(m_paReloadAllTabs, Qt::SHIFT+Qt::Key_F5);
    // "Forced"/ "Hard" reload action - re-downloads all e.g. images even if a cached
    // version already exists.
    m_paForceReload = actionCollection()->addAction(QStringLiteral("hard_reload"));
    // TODO - request new icon? (view-refresh will do for the time being)
    m_paForceReload->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    m_paForceReload->setText(i18n("&Force Reload"));
    connect(m_paForceReload, &QAction::triggered, this, &KonqMainWindow::slotForceReload);
    QList<QKeySequence> forceReloadShortcuts;
    forceReloadShortcuts.append(QKeySequence(Qt::CTRL+Qt::Key_F5));
    forceReloadShortcuts.append(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_R));
    actionCollection()->setDefaultShortcuts(m_paForceReload, forceReloadShortcuts);

    m_paUndo = KStandardAction::undo(m_pUndoManager, SLOT(undo()), this);
    actionCollection()->addAction(QStringLiteral("undo"), m_paUndo);
    connect(m_pUndoManager, SIGNAL(undoTextChanged(QString)),
            this, SLOT(slotUndoTextChanged(QString)));

    // Those are connected to the browserextension directly
    m_paCut = KStandardAction::cut(0, 0, this);
    actionCollection()->addAction(QStringLiteral("cut"), m_paCut);

    QList<QKeySequence> cutShortcuts(m_paCut->shortcuts());
    cutShortcuts.removeAll(QKeySequence(Qt::SHIFT+Qt::Key_Delete)); // used for deleting files
    actionCollection()->setDefaultShortcuts(m_paCut, cutShortcuts);

    m_paCopy = KStandardAction::copy(0, 0, this);
    actionCollection()->addAction(QStringLiteral("copy"), m_paCopy);
    m_paPaste = KStandardAction::paste(0, 0, this);
    actionCollection()->addAction(QStringLiteral("paste"), m_paPaste);
    m_paStop = actionCollection()->addAction(QStringLiteral("stop"));
    m_paStop->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));
    m_paStop->setText(i18n("&Stop"));
    connect(m_paStop, &QAction::triggered, this, &KonqMainWindow::slotStop);
    actionCollection()->setDefaultShortcut(m_paStop, Qt::Key_Escape);

    m_paAnimatedLogo = new KonqAnimatedLogo;
    QWidgetAction *logoAction = new QWidgetAction(this);
    actionCollection()->addAction(QStringLiteral("konq_logo"), logoAction);
    logoAction->setDefaultWidget(m_paAnimatedLogo);
    // Set icon and text so that it's easier to figure out what the action is in the toolbar editor
    logoAction->setText(i18n("Throbber"));
    logoAction->setIcon(QIcon::fromTheme(QStringLiteral("kde")));

    // Location bar
    m_locationLabel = new KonqDraggableLabel(this, i18n("L&ocation: "));
    QWidgetAction *locationAction = new QWidgetAction(this);
    actionCollection()->addAction(QStringLiteral("location_label"), locationAction);
    locationAction->setText(i18n("L&ocation: "));
    connect(locationAction, &QWidgetAction::triggered, this, &KonqMainWindow::slotLocationLabelActivated);
    locationAction->setDefaultWidget(m_locationLabel);
    m_locationLabel->setBuddy(m_combo);

    QWidgetAction *comboAction = new QWidgetAction(this);
    actionCollection()->addAction(QStringLiteral("toolbar_url_combo"), comboAction);
    comboAction->setText(i18n("Location Bar"));
    actionCollection()->setDefaultShortcut(comboAction, Qt::Key_F6);
    connect(comboAction, &QWidgetAction::triggered, this, &KonqMainWindow::slotLocationLabelActivated);
    comboAction->setDefaultWidget(m_combo);
    actionCollection()->setShortcutsConfigurable(comboAction, false);

    m_combo->setWhatsThis(i18n("<html>Location Bar<br /><br />Enter a web address or search term.</html>"));

    QAction *clearLocation = actionCollection()->addAction(QStringLiteral("clear_location"));
    clearLocation->setIcon(QIcon::fromTheme(QApplication::isRightToLeft() ? "edit-clear-locationbar-rtl" : "edit-clear-locationbar-ltr"));
    clearLocation->setText(i18n("Clear Location Bar"));
    actionCollection()->setDefaultShortcut(clearLocation, Qt::CTRL+Qt::Key_L);
    connect(clearLocation, SIGNAL(triggered()),
            SLOT(slotClearLocationBar()));
    clearLocation->setWhatsThis(i18n("<html>Clear Location bar<br /><br />"
                                     "Clears the contents of the location bar.</html>"));

    // Bookmarks menu
    m_pamBookmarks = new KBookmarkActionMenu(s_bookmarkManager->root(),
            i18n("&Bookmarks"), this);
    actionCollection()->addAction(QStringLiteral("bookmarks"), m_pamBookmarks);
    m_pamBookmarks->setDelayed(false);

    // The actual menu needs a different action collection, so that the bookmarks
    // don't appear in kedittoolbar
    m_bookmarksActionCollection = new KActionCollection(static_cast<QWidget *>(this));

    m_pBookmarkMenu = new KonqBookmarkMenu(s_bookmarkManager, m_pBookmarksOwner, m_pamBookmarks, m_bookmarksActionCollection);

    QAction *addBookmark = m_bookmarksActionCollection->action(QStringLiteral("add_bookmark"));
    if (addBookmark) {
        // Keep the "Add bookmark" action visible though (#153835)
        // -> We should think of a way to mark actions as "not configurable in toolbars" and
        // "should not appear in shortcut dialog (!= isShortcutConfigurable)" instead, and use
        // a single actionCollection.
        actionCollection()->addAction(QStringLiteral("add_bookmark"), m_bookmarksActionCollection->takeAction(addBookmark));
    } else {
        qDebug() << "Action add_bookmark not found!";
    }

    m_paShowMenuBar = KStandardAction::showMenubar(this, SLOT(slotShowMenuBar()), this);
    actionCollection()->addAction(KStandardAction::name(KStandardAction::ShowMenubar), m_paShowMenuBar);

    m_paShowStatusBar = KStandardAction::showStatusbar(this, SLOT(slotShowStatusBar()), this);
    actionCollection()->addAction(KStandardAction::name(KStandardAction::ShowStatusbar), m_paShowStatusBar);

    action = actionCollection()->addAction(QStringLiteral("konqintro"));
    action->setText(i18n("Kon&queror Introduction"));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotIntro);

    QAction *goUrl = actionCollection()->addAction(QStringLiteral("go_url"));
    goUrl->setIcon(QIcon::fromTheme(QStringLiteral("go-jump-locationbar")));
    goUrl->setText(i18n("Go"));
    connect(goUrl, &QAction::triggered, this, &KonqMainWindow::goURL);
    goUrl->setWhatsThis(i18n("<html>Go<br /><br />"
                             "Goes to the page that has been entered into the location bar.</html>"));

    enableAllActions(false);

    // help stuff
    m_paUp->setWhatsThis(i18n("<html>Enter the parent folder<br /><br />"
                              "For instance, if the current location is file:/home/%1 clicking this "
                              "button will take you to file:/home.</html>",  KUser().loginName()));
    m_paUp->setStatusTip(i18n("Enter the parent folder"));

    m_paBack->setWhatsThis(i18n("Move backwards one step in the browsing history"));
    m_paBack->setStatusTip(i18n("Move backwards one step in the browsing history"));

    m_paForward->setWhatsThis(i18n("Move forward one step in the browsing history"));
    m_paForward->setStatusTip(i18n("Move forward one step in the browsing history"));

    m_paClosedItems->setWhatsThis(i18n("Move backwards one step in the closed tabs history"));
    m_paClosedItems->setStatusTip(i18n("Move backwards one step in the closed tabs history"));

    m_paReload->setWhatsThis(i18n("<html>Reload the currently displayed document<br /><br />"
                                  "This may, for example, be needed to refresh web pages that have been "
                                  "modified since they were loaded, in order to make the changes visible.</html>"));
    m_paReload->setStatusTip(i18n("Reload the currently displayed document"));

    m_paReloadAllTabs->setWhatsThis(i18n("<html>Reload all currently displayed documents in tabs<br /><br />"
                                         "This may, for example, be needed to refresh web pages that have been "
                                         "modified since they were loaded, in order to make the changes visible.</html>"));
    m_paReloadAllTabs->setStatusTip(i18n("Reload all currently displayed document in tabs"));

    m_paStop->setWhatsThis(i18n("<html>Stop loading the document<br /><br />"
                                "All network transfers will be stopped and Konqueror will display the content "
                                "that has been received so far.</html>"));

    m_paForceReload->setWhatsThis(i18n("<html>Reload the currently displayed document<br /><br />"
                                       "This may, for example, be needed to refresh web pages that have been "
                                       "modified since they were loaded, in order to make the changes visible. Any images on the page are downloaded again, even if cached copies exist.</html>"));

    m_paForceReload->setStatusTip(i18n("Force a reload of the currently displayed document and any contained images"));

    m_paStop->setStatusTip(i18n("Stop loading the document"));

    m_paCut->setWhatsThis(i18n("<html>Cut the currently selected text or item(s) and move it "
                               "to the system clipboard<br /><br />"
                               "This makes it available to the <b>Paste</b> command in Konqueror "
                               "and other KDE applications.</html>"));
    m_paCut->setStatusTip(i18n("Move the selected text or item(s) to the clipboard"));

    m_paCopy->setWhatsThis(i18n("<html>Copy the currently selected text or item(s) to the "
                                "system clipboard<br /><br />"
                                "This makes it available to the <b>Paste</b> command in Konqueror "
                                "and other KDE applications.</html>"));
    m_paCopy->setStatusTip(i18n("Copy the selected text or item(s) to the clipboard"));

    m_paPaste->setWhatsThis(i18n("<html>Paste the previously cut or copied clipboard "
                                 "contents<br /><br />"
                                 "This also works for text copied or cut from other KDE applications.</html>"));
    m_paPaste->setStatusTip(i18n("Paste the clipboard contents"));

    m_paPrint->setWhatsThis(i18n("<html>Print the currently displayed document<br /><br />"
                                 "You will be presented with a dialog where you can set various "
                                 "options, such as the number of copies to print and which printer "
                                 "to use.<br /><br />"
                                 "This dialog also provides access to special KDE printing "
                                 "services such as creating a PDF file from the current document.</html>"));
    m_paPrint->setStatusTip(i18n("Print the current document"));

    m_paLockView->setStatusTip(i18n("A locked view cannot change folders. Use in combination with 'link view' to explore many files from one folder"));
    m_paLinkView->setStatusTip(i18n("Sets the view as 'linked'. A linked view follows folder changes made in other linked views."));
}

void KonqExtendedBookmarkOwner::openBookmark(const KBookmark &bm, Qt::MouseButtons mb, Qt::KeyboardModifiers km)
{
    qDebug() << bm.url() << km << mb;

    const QString url = bm.url().url();

    KonqOpenURLRequest req;
    req.browserArgs.setNewTab(true);
    req.newTabInFront = KonqSettings::newTabsInFront();
    req.forceAutoEmbed = true;

    if (km & Qt::ShiftModifier) {
        req.newTabInFront = !req.newTabInFront;
    }

    if (km & Qt::ControlModifier) {  // Ctrl Left/MMB
        m_pKonqMainWindow->openFilteredUrl(url, req);
    } else if (mb & Qt::MidButton) {
        if (KonqSettings::mmbOpensTab()) {
            m_pKonqMainWindow->openFilteredUrl(url, req);
        } else {
            const QUrl finalURL = KonqMisc::konqFilteredURL(m_pKonqMainWindow, url);
            KonqMainWindow *mw = KonqMainWindowFactory::createNewWindow(finalURL);
            mw->show();
        }
    } else {
        m_pKonqMainWindow->openFilteredUrl(url, false);
    }
}

void KonqMainWindow::slotMoveTabLeft()
{
    if (QApplication::isRightToLeft()) {
        m_pViewManager->moveTabForward();
    } else {
        m_pViewManager->moveTabBackward();
    }

    updateViewActions();
}

void KonqMainWindow::slotMoveTabRight()
{
    if (QApplication::isRightToLeft()) {
        m_pViewManager->moveTabBackward();
    } else {
        m_pViewManager->moveTabForward();
    }

    updateViewActions();
}

void KonqMainWindow::updateHistoryActions()
{
    if (m_currentView) {
        m_paBack->setEnabled(m_currentView->canGoBack());
        m_paForward->setEnabled(m_currentView->canGoForward());
    }
}

void KonqMainWindow::updateToolBarActions(bool pendingAction /*=false*/)
{
    if (!m_currentView) {
        return;
    }

    // Enables/disables actions that depend on the current view & url (mostly toolbar)
    // Up, back, forward, the edit extension, stop button, wheel
    setUpEnabled(m_currentView->url());
    m_paBack->setEnabled(m_currentView->canGoBack());
    m_paForward->setEnabled(m_currentView->canGoForward());

    if (m_currentView->isLoading()) {
        startAnimation(); // takes care of m_paStop
    } else {
        m_paAnimatedLogo->stop();
        m_paStop->setEnabled(pendingAction);    //enable/disable based on any pending actions...
    }
}

void KonqMainWindow::updateViewActions()
{
    // Update actions that depend on the current view and its mode, or on the number of views etc.

    // Don't do things in this method that depend on m_currentView->url().
    // When going 'back' in history this will be called before opening the url.
    // Use updateToolBarActions instead.

    bool enable = false;

    if (m_currentView && m_currentView->part()) {
        // Avoid qWarning from QObject::property if it doesn't exist
        if (m_currentView->part()->metaObject()->indexOfProperty("supportsUndo") != -1) {
            QVariant prop = m_currentView->part()->property("supportsUndo");
            if (prop.isValid() && prop.toBool()) {
                enable = true;
            }
        }
    }

    m_pUndoManager->updateSupportsFileUndo(enable);

//   slotUndoAvailable( m_pUndoManager->undoAvailable() );

    m_paLockView->setEnabled(true);
    m_paLockView->setChecked(m_currentView && m_currentView->isLockedLocation());

    // Can remove view if we'll still have a main view after that
    m_paRemoveView->setEnabled(mainViewsCount() > 1 ||
                               (m_currentView && m_currentView->isToggleView()));

    if (!currentView() || !currentView()->frame()) {
        m_paAddTab->setEnabled(false);
        m_paDuplicateTab->setEnabled(false);
        m_paRemoveOtherTabs->setEnabled(false);
        m_paBreakOffTab->setEnabled(false);
        m_paActivateNextTab->setEnabled(false);
        m_paActivatePrevTab->setEnabled(false);
        m_paMoveTabLeft->setEnabled(false);
        m_paMoveTabRight->setEnabled(false);
    } else {
        m_paAddTab->setEnabled(true);
        m_paDuplicateTab->setEnabled(true);
        KonqFrameTabs *tabContainer = m_pViewManager->tabContainer();
        bool state = (tabContainer->count() > 1);
        m_paRemoveOtherTabs->setEnabled(state);
        m_paBreakOffTab->setEnabled(state);
        m_paActivateNextTab->setEnabled(state);
        m_paActivatePrevTab->setEnabled(state);

        QList<KonqFrameBase *> childFrameList = tabContainer->childFrameList();
        Q_ASSERT(!childFrameList.isEmpty());
        m_paMoveTabLeft->setEnabled(currentView() ? currentView()->frame() !=
                                    (QApplication::isRightToLeft() ? childFrameList.last() : childFrameList.first()) : false);
        m_paMoveTabRight->setEnabled(currentView() ? currentView()->frame() !=
                                     (QApplication::isRightToLeft() ? childFrameList.first() : childFrameList.last()) : false);
    }

    // Can split a view if it's not a toggle view (because a toggle view can be here only once)
    bool isNotToggle = m_currentView && !m_currentView->isToggleView();
    m_paSplitViewHor->setEnabled(isNotToggle);
    m_paSplitViewVer->setEnabled(isNotToggle);

    m_paLinkView->setChecked(m_currentView && m_currentView->isLinkedView());

#if 0
    if (m_currentView && m_currentView->part() &&
            ::qobject_cast<KonqDirPart *>(m_currentView->part())) {
        KonqDirPart *dirPart = static_cast<KonqDirPart *>(m_currentView->part());
        m_paFindFiles->setEnabled(dirPart->findPart() == 0);

        // Create the copy/move options if not already done
        // TODO: move that stuff to dolphin(part)
        if (!m_paCopyFiles) {
            // F5 is the default key binding for Reload.... a la Windows.
            // mc users want F5 for Copy and F6 for move, but I can't make that default.

            m_paCopyFiles = actionCollection()->addAction("copyfiles");
            m_paCopyFiles->setText(i18n("Copy &Files..."));
            connect(m_paCopyFiles, &QAction::triggered, this, &KonqMainWindow::slotCopyFiles);
            m_paCopyFiles->setShortcut(Qt::Key_F7);
            m_paMoveFiles = actionCollection()->addAction("movefiles");
            m_paMoveFiles->setText(i18n("M&ove Files..."));
            connect(m_paMoveFiles, &QAction::triggered, this, &KonqMainWindow::slotMoveFiles);
            m_paMoveFiles->setShortcut(Qt::Key_F8);

            QList<QAction *> lst;
            lst.append(m_paCopyFiles);
            lst.append(m_paMoveFiles);
            m_paCopyFiles->setEnabled(false);
            m_paMoveFiles->setEnabled(false);
            plugActionList("operations", lst);
        }
    } else {
        m_paFindFiles->setEnabled(false);

        if (m_paCopyFiles) {
            unplugActionList("operations");
            delete m_paCopyFiles;
            m_paCopyFiles = 0;
            delete m_paMoveFiles;
            m_paMoveFiles = 0;
        }
    }
#endif
}

QString KonqMainWindow::findIndexFile(const QString &dir)
{
    QDir d(dir);

    QString f = d.filePath(QStringLiteral("index.html"));
    if (QFile::exists(f)) {
        return f;
    }

    f = d.filePath(QStringLiteral("index.htm"));
    if (QFile::exists(f)) {
        return f;
    }

    f = d.filePath(QStringLiteral("index.HTML"));
    if (QFile::exists(f)) {
        return f;
    }

    return QString();
}

void KonqMainWindow::connectExtension(KParts::BrowserExtension *ext)
{
    KParts::BrowserExtension::ActionSlotMap *actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();
    KParts::BrowserExtension::ActionSlotMap::ConstIterator it = actionSlotMap->constBegin();
    KParts::BrowserExtension::ActionSlotMap::ConstIterator itEnd = actionSlotMap->constEnd();

    for (; it != itEnd; ++it) {
        QAction *act = actionCollection()->action(it.key().data());
        //qDebug() << it.key();
        if (act) {
            // Does the extension have a slot with the name of this action ?
            if (ext->metaObject()->indexOfSlot(it.key() + "()") != -1) {
                connect(act, SIGNAL(triggered()), ext, it.value() /* SLOT(slot name) */);
                act->setEnabled(ext->isActionEnabled(it.key()));
                const QString text = ext->actionText(it.key());
                if (!text.isEmpty()) {
                    act->setText(text);
                }
                // TODO how to re-set the original action text, when switching to a part that didn't call setAction?
                // Can't test with Paste...
            } else {
                act->setEnabled(false);
            }

        } else {
            qWarning() << "Error in BrowserExtension::actionSlotMap(), unknown action : " << it.key();
        }
    }

}

void KonqMainWindow::disconnectExtension(KParts::BrowserExtension *ext)
{
    KParts::BrowserExtension::ActionSlotMap *actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();
    KParts::BrowserExtension::ActionSlotMap::ConstIterator it = actionSlotMap->constBegin();
    KParts::BrowserExtension::ActionSlotMap::ConstIterator itEnd = actionSlotMap->constEnd();

    for (; it != itEnd; ++it) {
        QAction *act = actionCollection()->action(it.key().data());
        //qDebug() << it.key();
        if (act && ext->metaObject()->indexOfSlot(it.key() + "()") != -1) {
            //qDebug() << act << act->name();
            act->disconnect(ext);
        }
    }
}

void KonqMainWindow::enableAction(const char *name, bool enabled)
{
    QAction *act = actionCollection()->action(name);
    if (!act) {
        qWarning() << "Unknown action " << name << " - can't enable";
    } else {
        if (m_bLocationBarConnected && (
                    act == m_paCopy || act == m_paCut || act == m_paPaste))
            // Don't change action state while the location bar has focus.
        {
            return;
        }
        //qDebug() << name << enabled;
        act->setEnabled(enabled);
    }

    // Update "copy files" and "move files" accordingly
    if (m_paCopyFiles && !strcmp(name, "copy")) {
        m_paCopyFiles->setEnabled(enabled);
    } else if (m_paMoveFiles && !strcmp(name, "cut")) {
        m_paMoveFiles->setEnabled(enabled);
    }
}

void KonqMainWindow::setActionText(const char *name, const QString &text)
{
    QAction *act = actionCollection()->action(name);
    if (!act) {
        qWarning() << "Unknown action " << name << "- can't enable";
    } else {
        //qDebug() << name << " text=" << text;
        act->setText(text);
    }
}

void KonqMainWindow::enableAllActions(bool enable)
{
    //qDebug() << enable;
    KParts::BrowserExtension::ActionSlotMap *actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();

    const QList<QAction *> actions = actionCollection()->actions();
    QList<QAction *>::ConstIterator it = actions.constBegin();
    QList<QAction *>::ConstIterator end = actions.constEnd();
    for (; it != end; ++it) {
        QAction *act = *it;
        if (!act->objectName().startsWith(QLatin1String("options_configure"))  /* do not touch the configureblah actions */
                && (!enable || !actionSlotMap->contains(act->objectName().toLatin1()))) {    /* don't enable BE actions */
            act->setEnabled(enable);
        }
    }
    // This method is called with enable=false on startup, and
    // then only once with enable=true when the first view is setup.
    // So the code below is where actions that should initially be disabled are disabled.
    if (enable) {
        setUpEnabled(m_currentView ? m_currentView->url() : QUrl());
        // we surely don't have any history buffers at this time
        m_paBack->setEnabled(false);
        m_paForward->setEnabled(false);

        updateViewActions(); // undo, lock, link and other view-dependent actions
        updateClosedItemsAction();

        m_paStop->setEnabled(m_currentView && m_currentView->isLoading());

        if (m_toggleViewGUIClient) {
            QList<QAction *> actions = m_toggleViewGUIClient->actions();
            for (int i = 0; i < actions.size(); ++i) {
                actions.at(i)->setEnabled(true);
            }
        }

    }
    actionCollection()->action(QStringLiteral("quit"))->setEnabled(true);
    actionCollection()->action(QStringLiteral("link"))->setEnabled(false);
}

void KonqMainWindow::disableActionsNoView()
{
    // No view -> there are some things we can't do
    m_paUp->setEnabled(false);
    m_paReload->setEnabled(false);
    m_paReloadAllTabs->setEnabled(false);
    m_paBack->setEnabled(false);
    m_paForward->setEnabled(false);
    m_paLockView->setEnabled(false);
    m_paLockView->setChecked(false);
    m_paSplitViewVer->setEnabled(false);
    m_paSplitViewHor->setEnabled(false);
    m_paRemoveView->setEnabled(false);
    m_paLinkView->setEnabled(false);
    if (m_toggleViewGUIClient) {
        QList<QAction *> actions = m_toggleViewGUIClient->actions();
        for (int i = 0; i < actions.size(); ++i) {
            actions.at(i)->setEnabled(false);
        }
    }
    // There are things we can do, though : bookmarks, view profile, location bar, new window,
    // settings, etc.
    static const char *const s_enActions[] = { "new_window", "duplicate_window", "open_location",
                                               "toolbar_url_combo", "clear_location", "animated_logo",
                                               "konqintro", "go_most_often", "go_applications",
                                               "go_trash", "go_settings", "go_network_folders", "go_autostart",
                                               "go_url", "go_media", "go_history", "options_configure_extensions", 0
                                             };
    for (int i = 0; s_enActions[i]; ++i) {
        QAction *act = action(s_enActions[i]);
        if (act) {
            act->setEnabled(true);
        }
    }
    m_combo->clearTemporary();
}

void KonqMainWindow::setCaption(const QString &caption)
{
    // KParts sends us empty captions when activating a brand new part
    // We can't change it there (in case of apps removing all parts altogether)
    // but here we never do that.
    if (!caption.isEmpty() && m_currentView) {
        //qDebug() << caption;

        // Keep an unmodified copy of the caption (before squeezing and KComponentData::makeStdCaption are applied)
        m_currentView->setCaption(caption);
        KParts::MainWindow::setCaption(KStringHandler::csqueeze(m_currentView->caption(), 128));
    }
}

void KonqMainWindow::showEvent(QShowEvent *event)
{
    //qDebug() << QTime::currentTime();
    // We need to check if our toolbars are shown/hidden here, and set
    // our menu items accordingly. We can't do it in the constructor because
    // view profiles store toolbar info, and that info is read after
    // construct time.
    m_paShowMenuBar->setChecked(!menuBar()->isHidden());
    if (m_currentView) {
        m_paShowStatusBar->setChecked(m_currentView->frame()->statusbar()->isVisible());
    }
    updateBookmarkBar(); // hide if empty

    // Call parent method
    KParts::MainWindow::showEvent(event);
}

QUrl KonqExtendedBookmarkOwner::currentUrl() const
{
    const KonqView *view = m_pKonqMainWindow->currentView();
    return view ? view->url() : QUrl();
}

QString KonqMainWindow::currentURL() const
{
    if (!m_currentView) {
        return QString();
    }
    QString url = m_currentView->url().toDisplayString();

#if 0 // do we want this?
    // Add the name filter (*.txt) at the end of the URL again
    if (m_currentView->part()) {
        const QString nameFilter = m_currentView->nameFilter();
        if (!nameFilter.isEmpty()) {
            if (!url.endsWith('/')) {
                url += '/';
            }
            url += nameFilter;
        }
    }
#endif
    return url;
}

bool KonqExtendedBookmarkOwner::supportsTabs() const
{
    return true;
}

QList<KBookmarkOwner::FutureBookmark> KonqExtendedBookmarkOwner::currentBookmarkList() const
{
    QList<KBookmarkOwner::FutureBookmark> list;
    KonqFrameTabs *tabContainer = m_pKonqMainWindow->viewManager()->tabContainer();

    foreach (KonqFrameBase *frame, tabContainer->childFrameList()) {
        if (!frame || !frame->activeChildView()) {
            continue;
        }
        KonqView *view = frame->activeChildView();
        if (view->locationBarURL().isEmpty()) {
            continue;
        }
        list << KBookmarkOwner::FutureBookmark(view->caption(), view->url(), KIO::iconNameForUrl(view->url()));
    }
    return list;
}

QString KonqExtendedBookmarkOwner::currentTitle() const
{
    return m_pKonqMainWindow->currentTitle();
}

void KonqExtendedBookmarkOwner::openInNewTab(const KBookmark &bm)
{
    bool newTabsInFront = KonqSettings::newTabsInFront();
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        newTabsInFront = !newTabsInFront;
    }

    KonqOpenURLRequest req;
    req.browserArgs.setNewTab(true);
    req.newTabInFront = newTabsInFront;
    req.openAfterCurrentPage = false;
    req.forceAutoEmbed = true;

    m_pKonqMainWindow->openFilteredUrl(bm.url().url(), req);
}

void KonqExtendedBookmarkOwner::openFolderinTabs(const KBookmarkGroup &grp)
{
    bool newTabsInFront = KonqSettings::newTabsInFront();
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        newTabsInFront = !newTabsInFront;
    }
    KonqOpenURLRequest req;
    req.browserArgs.setNewTab(true);
    req.newTabInFront = false;
    req.openAfterCurrentPage = false;
    req.forceAutoEmbed = true;

    const QList<QUrl> list = grp.groupUrlList();
    if (list.isEmpty()) {
        return;
    }

    if (list.size() > 20) {
        if (KMessageBox::questionYesNo(m_pKonqMainWindow,
                                       i18n("You have requested to open more than 20 bookmarks in tabs. "
                                            "This might take a while. Continue?"),
                                       i18nc("@title:window", "Open bookmarks folder in new tabs")) != KMessageBox::Yes) {
            return;
        }
    }

    QList<QUrl>::ConstIterator it = list.constBegin();
    QList<QUrl>::ConstIterator end = list.constEnd();
    --end;
    for (; it != end; ++it) {
        m_pKonqMainWindow->openFilteredUrl((*it).toString(), req);
    }
    if (newTabsInFront) {
        req.newTabInFront = true;
    }
    m_pKonqMainWindow->openFilteredUrl((*end).toString(), req);
}

void KonqExtendedBookmarkOwner::openInNewWindow(const KBookmark &bm)
{
    const QUrl finalURL(KonqMisc::konqFilteredURL(m_pKonqMainWindow, bm.url().url()));
    KonqMainWindow *mw = KonqMainWindowFactory::createNewWindow(finalURL);
    mw->show();
}

QString KonqMainWindow::currentTitle() const
{
    return m_currentView ? m_currentView->caption() : QString();
}

// Convert between deprecated string-based KParts::BrowserExtension::ActionGroupMap
// to newer enum-based KonqPopupMenu::ActionGroupMap
static KonqPopupMenu::ActionGroupMap convertActionGroups(const KParts::BrowserExtension::ActionGroupMap &input)
{
    KonqPopupMenu::ActionGroupMap agm;
    agm.insert(KonqPopupMenu::TopActions, input.value(QStringLiteral("topactions")));
    agm.insert(KonqPopupMenu::TabHandlingActions, input.value(QStringLiteral("tabhandling")));
    agm.insert(KonqPopupMenu::EditActions, input.value(QStringLiteral("editactions")));
    agm.insert(KonqPopupMenu::PreviewActions, input.value(QStringLiteral("preview")));
    agm.insert(KonqPopupMenu::CustomActions, input.value(QStringLiteral("partactions")));
    agm.insert(KonqPopupMenu::LinkActions, input.value(QStringLiteral("linkactions")));
    return agm;
}

void KonqMainWindow::slotPopupMenu(const QPoint &global, const QUrl &url, mode_t mode, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &browserArgs, KParts::BrowserExtension::PopupFlags flags, const KParts::BrowserExtension::ActionGroupMap &actionGroups)
{
    KFileItem item(url, args.mimeType(), mode);
    KFileItemList items;
    items.append(item);
    slotPopupMenu(global, items, args, browserArgs, flags, actionGroups);
}

void KonqMainWindow::slotPopupMenu(const QPoint &global, const KFileItemList &items, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &browserArgs, KParts::BrowserExtension::PopupFlags itemFlags, const KParts::BrowserExtension::ActionGroupMap &actionGroups)
{
    KonqView *m_oldView = m_currentView;
    KonqView *currentView = childView(static_cast<KParts::ReadOnlyPart *>(sender()->parent()));

    //qDebug() << "m_oldView=" << m_oldView << "new currentView=" << currentView << "passive:" << currentView->isPassiveMode();

    if ((m_oldView != currentView) && currentView->isPassiveMode()) {
        // Make this view active only temporarily (because it's passive)
        m_currentView = currentView;

        if (m_oldView && m_oldView->browserExtension()) {
            disconnectExtension(m_oldView->browserExtension());
        }
        if (m_currentView->browserExtension()) {
            connectExtension(m_currentView->browserExtension());
        }
    }
    // Note that if m_oldView!=currentView and currentView isn't passive,
    // then the KParts mechanism has already noticed the click in it,
    // but KonqViewManager delays the GUI-rebuilding with a single-shot timer.
    // Right after the popup shows up, currentView _will_ be m_currentView.

    //qDebug() << "current view=" << m_currentView << m_currentView->part()->metaObject()->className();

    // This action collection is used to pass actions to KonqPopupMenu.
    // It has to be a KActionCollection instead of a QList<QAction *> because we need
    // the actionStatusText signal...
    KActionCollection popupMenuCollection(static_cast<QWidget *>(0));

    popupMenuCollection.addAction(QStringLiteral("closeditems"), m_paClosedItems);

#if 0
    popupMenuCollection.addAction("find", m_paFindFiles);
#endif

    popupMenuCollection.addAction(QStringLiteral("undo"), m_paUndo);
    popupMenuCollection.addAction(QStringLiteral("cut"), m_paCut);
    popupMenuCollection.addAction(QStringLiteral("copy"), m_paCopy);
    popupMenuCollection.addAction(QStringLiteral("paste"), m_paPaste);

    // The pasteto action is used when clicking on a dir, to paste into it.
    QAction *actPaste = KStandardAction::paste(this, SLOT(slotPopupPasteTo()), this);
    actPaste->setEnabled(m_paPaste->isEnabled());
    popupMenuCollection.addAction(QStringLiteral("pasteto"), actPaste);

    prepareForPopupMenu(items, args, browserArgs);

    bool sReading = false;
    if (!m_popupUrl.isEmpty()) {
        sReading = KProtocolManager::supportsReading(m_popupUrl);
    }

    QUrl viewURL = currentView->url();
    qDebug() << "viewURL=" << viewURL;

    bool openedForViewURL = false;
    //bool dirsSelected = false;
    bool devicesFile = false;

    if (items.count() == 1) {
        const QUrl firstURL = items.first().url();
        if (!viewURL.isEmpty()) {
            //firstURL.cleanPath();
            openedForViewURL = firstURL.matches(viewURL, QUrl::StripTrailingSlash);
        }
        devicesFile = firstURL.scheme().indexOf(QLatin1String("device"), 0, Qt::CaseInsensitive) == 0;
        //dirsSelected = S_ISDIR( items.first()->mode() );
    }
    //qDebug() << "viewURL=" << viewURL;

    QUrl url = viewURL;
    bool isIntoTrash = url.scheme() == QLatin1String("trash") || url.url().startsWith(QLatin1String("system:/trash"));
    const bool doTabHandling = !openedForViewURL && !isIntoTrash && sReading;
    const bool showEmbeddingServices = items.count() == 1 && !m_popupMimeType.isEmpty() &&
                                       !isIntoTrash && !devicesFile &&
                                       (itemFlags & KParts::BrowserExtension::ShowTextSelectionItems) == 0;

    KService::List embeddingServices;
    if (showEmbeddingServices) {
        const QString currentServiceName = currentView->service()->desktopEntryName();

        // List of services for the "Preview In" submenu.
        embeddingServices = KMimeTypeTrader::self()->query(
                                m_popupMimeType,
                                QStringLiteral("KParts/ReadOnlyPart"),
                                // Obey "HideFromMenus". It defaults to false so we want "absent or true"
                                // (wow, testing for 'true' if absent doesn't work, so order matters)
                                "(not exist [X-KDE-BrowserView-HideFromMenus] or not [X-KDE-BrowserView-HideFromMenus]) "
                                "and DesktopEntryName != '" +currentServiceName + "' "
                                // I had an old local dirtree.desktop without lib, no need for invalid entries
                                "and exist [Library]");
    }

    // TODO: get rid of KParts::BrowserExtension::PopupFlags
    KonqPopupMenu::Flags popupFlags = static_cast<KonqPopupMenu::Flags>(static_cast<int>(itemFlags));

    KonqPopupMenu::ActionGroupMap popupActionGroups = convertActionGroups(actionGroups);

    PopupMenuGUIClient *konqyMenuClient = new PopupMenuGUIClient(embeddingServices,
            popupActionGroups,
            !menuBar()->isVisible() ? m_paShowMenuBar : 0,
            fullScreenMode() ? m_ptaFullScreen : 0
                                                                );
    qRegisterMetaType<KService::Ptr>("KService::Ptr");
    connect(konqyMenuClient, SIGNAL(openEmbedded(KService::Ptr)),
            this, SLOT(slotOpenEmbedded(KService::Ptr)), Qt::QueuedConnection);

    // Those actions go into the PopupMenuGUIClient, since that's the one defining them.
    QList<QAction *> tabHandlingActions;
    if (doTabHandling) {
        if (browserArgs.forcesNewWindow()) {
            QAction *act = konqyMenuClient->actionCollection()->addAction(QStringLiteral("sameview"));
            act->setText(i18n("Open in T&his Window"));
            act->setStatusTip(i18n("Open the document in current window"));
            connect(act, &QAction::triggered, this, &KonqMainWindow::slotPopupThisWindow);
            tabHandlingActions.append(act);
        }
        QAction *actNewWindow = konqyMenuClient->actionCollection()->addAction(QStringLiteral("newview"));
        actNewWindow->setIcon(QIcon::fromTheme(QStringLiteral("window-new")));
        actNewWindow->setText(i18n("Open in New &Window"));
        actNewWindow->setStatusTip(i18n("Open the document in a new window"));
        connect(actNewWindow, &QAction::triggered, this, &KonqMainWindow::slotPopupNewWindow);
        tabHandlingActions.append(actNewWindow);

        QAction *actNewTab = konqyMenuClient->actionCollection()->addAction(QStringLiteral("openintab"));
        actNewTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
        actNewTab->setText(i18n("Open in &New Tab"));
        connect(actNewTab, &QAction::triggered, this, &KonqMainWindow::slotPopupNewTab);
        actNewTab->setStatusTip(i18n("Open the document in a new tab"));
        tabHandlingActions.append(actNewTab);

        QAction *separator = new QAction(konqyMenuClient->actionCollection());
        separator->setSeparator(true);
        tabHandlingActions.append(separator);
    }

    if (doTabHandling) {
        popupActionGroups.insert(KonqPopupMenu::TabHandlingActions, tabHandlingActions);
    }

    QPointer<KonqPopupMenu> pPopupMenu = new KonqPopupMenu(
        items,
        viewURL,
        popupMenuCollection,
        popupFlags,
        // This parent ensures that if the part destroys itself (e.g. KHTML redirection),
        // it will close the popupmenu
        currentView->part()->widget());
    pPopupMenu->setNewFileMenu(m_pMenuNew);
    pPopupMenu->setBookmarkManager(s_bookmarkManager);
    pPopupMenu->setActionGroups(popupActionGroups);

    if (openedForViewURL && !viewURL.isLocalFile()) {
        pPopupMenu->setURLTitle(currentView->caption());
    }

    QPointer<KParts::BrowserExtension> be = ::qobject_cast<KParts::BrowserExtension *>(sender());

    if (be) {
        QObject::connect(this, &KonqMainWindow::popupItemsDisturbed, pPopupMenu.data(), &KonqPopupMenu::close);
        QObject::connect(be, SIGNAL(itemsRemoved(KFileItemList)),
                         this, SLOT(slotItemsRemoved(KFileItemList)));
    }

    QPointer<QObject> guard(this);   // #149736, window could be deleted inside popupmenu event loop
    pPopupMenu->exec(global);

    delete pPopupMenu;

    // We're sort of misusing KActionCollection here, but we need it for the actionStatusText signal...
    // Anyway. If the action belonged to the view, and the view got deleted, we don't want ~KActionCollection
    // to iterate over those deleted actions
    /*KActionPtrList lst = popupMenuCollection.actions();
    KActionPtrList::iterator it = lst.begin();
    for ( ; it != lst.end() ; ++it )
        popupMenuCollection.take( *it );*/

    if (guard.isNull()) { // the placement of this test is very important, double-check #149736 if moving stuff around
        return;
    }

    if (be) {
        QObject::disconnect(be, SIGNAL(itemsRemoved(KFileItemList)),
                            this, SLOT(slotItemsRemoved(KFileItemList)));
    }

    delete konqyMenuClient;
    m_popupItems.clear();

    // Deleted by konqyMenuClient's actioncollection
    //delete actNewTab;
    //delete actNewWindow;

    delete actPaste;

    // Restore current view if current is passive
    if ((m_oldView != currentView) && (currentView == m_currentView) && currentView->isPassiveMode()) {
        //qDebug() << "restoring active view" << m_oldView;
        if (m_currentView->browserExtension()) {
            disconnectExtension(m_currentView->browserExtension());
        }
        if (m_oldView) {
            if (m_oldView->browserExtension()) {
                connectExtension(m_oldView->browserExtension());
                m_currentView = m_oldView;
            }
            // Special case: RMB + renaming in sidebar; setFocus would abort editing.
            QWidget *fw = focusWidget();
            if (!fw || !::qobject_cast<QLineEdit *>(fw)) {
                m_oldView->part()->widget()->setFocus();
                m_pViewManager->setActivePart(m_oldView->part());
            }
        }
    }
}

void KonqMainWindow::prepareForPopupMenu(const KFileItemList &items, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &browserArgs)
{
    if (!items.isEmpty()) {
        m_popupUrl = items.first().url();
        m_popupMimeType = items.first().mimetype();
    } else {
        m_popupUrl = QUrl();
        m_popupMimeType.clear();
    }
    // We will need these if we call the newTab slot
    m_popupItems = items;
    m_popupUrlArgs = args;
    m_popupUrlArgs.setMimeType(QString());   // Reset so that Open in New Window/Tab does mimetype detection
    m_popupUrlBrowserArgs = browserArgs;
}

void KonqMainWindow::slotItemsRemoved(const KFileItemList &items)
{
    QListIterator<KFileItem> it(items);
    while (it.hasNext()) {
        if (m_popupItems.contains(it.next())) {
            emit popupItemsDisturbed();
            return;
        }
    }
}

void KonqMainWindow::slotOpenEmbedded(KService::Ptr service)
{
    if (!m_currentView) {
        return;
    }

    m_currentView->stop();
    m_currentView->setLocationBarURL(m_popupUrl);
    m_currentView->setTypedURL(QString());
    if (m_currentView->changePart(m_popupMimeType,
                                  service->desktopEntryName(), true)) {
        m_currentView->openUrl(m_popupUrl, m_popupUrl.toDisplayString(QUrl::PreferLocalFile));
    }
}

void KonqMainWindow::slotPopupPasteTo()
{
    if (!m_currentView || m_popupUrl.isEmpty()) {
        return;
    }
    m_currentView->callExtensionURLMethod("pasteTo", m_popupUrl);
}

void KonqMainWindow::slotReconfigure()
{
    reparseConfiguration();
}

void KonqMainWindow::reparseConfiguration()
{
    qDebug();

    KonqSettings::self()->load();
    m_pViewManager->applyConfiguration();
    KonqMouseEventFilter::self()->reparseConfiguration();

    if (m_combo) {
        m_combo->setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
    }

    MapViews::ConstIterator it = m_mapViews.constBegin();
    MapViews::ConstIterator end = m_mapViews.constEnd();
    for (; it != end; ++it) {
        (*it)->reparseConfiguration();
    }
}

void KonqMainWindow::saveProperties(KConfigGroup &config)
{
    // Ensure no crash if the sessionmanager timer fires before the ctor is done
    // This can happen via ToggleViewGUIClient -> KServiceTypeTrader::query
    // -> KSycoca running kbuildsycoca -> nested event loop.
    if (m_fullyConstructed) {
        KonqFrameBase::Options flags = KonqFrameBase::saveHistoryItems;
        m_pViewManager->saveViewConfigToGroup(config, flags);
    }
}

void KonqMainWindow::readProperties(const KConfigGroup &configGroup)
{
    m_pViewManager->loadViewConfigFromGroup(configGroup, QString() /*no profile name*/);
    // read window settings
    applyMainWindowSettings(configGroup);
}

void KonqMainWindow::setInitialFrameName(const QString &name)
{
    m_initialFrameName = name;
}

void KonqMainWindow::updateOpenWithActions()
{
    unplugActionList(QStringLiteral("openwithbase"));
    unplugActionList(QStringLiteral("openwith"));

    qDeleteAll(m_openWithActions);
    m_openWithActions.clear();

    delete m_openWithMenu;
    m_openWithMenu = 0;

    if (!KAuthorized::authorizeAction(QStringLiteral("openwith"))) {
        return;
    }

    m_openWithMenu = new KActionMenu(i18n("&Open With"), this);

    const KService::List &services = m_currentView->appServiceOffers();
    KService::List::ConstIterator it = services.constBegin();
    const KService::List::ConstIterator end = services.constEnd();

    const int baseOpenWithItems = qMax(KonqSettings::openWithItems(), 0);

    int idxService = 0;
    for (; it != end; ++it, ++idxService) {
        QAction *action;

        if (idxService < baseOpenWithItems) {
            action = new QAction(i18n("Open with %1", (*it)->name()), this);
        } else {
            action = new QAction((*it)->name(), this);
        }
        action->setIcon(QIcon::fromTheme((*it)->icon()));

        connect(action, SIGNAL(triggered()),
                this, SLOT(slotOpenWith()));

        actionCollection()->addAction((*it)->desktopEntryName(), action);
        if (idxService < baseOpenWithItems) {
            m_openWithActions.append(action);
        } else {
            m_openWithMenu->addAction(action);
        }
    }

    if (services.count() > 0) {
        plugActionList(QStringLiteral("openwithbase"), m_openWithActions);
        QList<QAction *> openWithActionsMenu;
        if (idxService > baseOpenWithItems) {
            openWithActionsMenu.append(m_openWithMenu);
        }
        QAction *sep = new QAction(this);
        sep->setSeparator(true);
        openWithActionsMenu.append(sep);
        plugActionList(QStringLiteral("openwith"), openWithActionsMenu);
    }
}

void KonqMainWindow::updateViewModeActions()
{
    unplugViewModeActions();
    Q_FOREACH (QAction *action, m_viewModesGroup->actions()) {
        Q_FOREACH (QWidget *w, action->associatedWidgets()) {
            w->removeAction(action);
        }
        delete action;
    }

    delete m_viewModeMenu;
    m_viewModeMenu = 0;

    const KService::List services = m_currentView->partServiceOffers();
    if (services.count() <= 1) {
        return;
    }

    m_viewModeMenu = new KActionMenu(i18nc("@action:inmenu View", "&View Mode"), this);
    //actionCollection()->addAction( "viewModeMenu", m_viewModeMenu );

    KService::List::ConstIterator it = services.constBegin();
    const KService::List::ConstIterator end = services.constEnd();
    for (; it != end; ++it) {
        const KService::Ptr service = *it;

        const QString desktopEntryName = service->desktopEntryName();
        bool bIsCurrentView = desktopEntryName == m_currentView->service()->desktopEntryName();

        const QList<KServiceAction> actions = service->actions();
        if (!actions.isEmpty()) {

            // The service provides several view modes, like DolphinPart
            // -> create one action per view mode
            Q_FOREACH (const KServiceAction &serviceAction, actions) {
                // Create a KToggleAction for each view mode, and plug it into the menu
                KToggleAction *action = new KToggleAction(QIcon::fromTheme(serviceAction.icon()), serviceAction.text(), this);
                //actionCollection()->addAction(desktopEntryName /*not unique!*/, action);
                action->setObjectName(desktopEntryName + QLatin1String("-viewmode"));
                action->setData(QVariant(serviceAction.name()));
                action->setActionGroup(m_viewModesGroup);
                m_viewModeMenu->menu()->addAction(action);
                if (bIsCurrentView && m_currentView->internalViewMode() == serviceAction.name()) {
                    action->setChecked(true);
                }
            }

        } else {
            // The service only provides one view mode (common case)

            QString serviceText = service->genericName();
            if (serviceText.isEmpty()) {
                serviceText = service->name();
            }

            // Create a KToggleAction for this view mode, and plug it into the menu
            KToggleAction *action = new KToggleAction(QIcon::fromTheme(service->icon()), serviceText, this);
            // NOTE: "-viewmode" is appended to desktopEntryName to avoid overwritting existing
            // action, e.g. konsolepart added through ToggleViewGUIClient in the ctor will be
            // overwritten by the view mode konsolepart action added here.  #266517.
            actionCollection()->addAction(desktopEntryName + QLatin1String("-viewmode"), action);
            action->setActionGroup(m_viewModesGroup);
            m_viewModeMenu->menu()->addAction(action);

            action->setChecked(bIsCurrentView);
        }
    }

    // No view mode for actions toggable views
    // (The other way would be to enforce a better servicetype for them, than Browser/View)
    if (!m_currentView->isToggleView()
            /* already tested: && services.count() > 1 */
            && m_viewModeMenu) {
        plugViewModeActions();
    }
}

void KonqMainWindow::slotInternalViewModeChanged()
{
    KParts::ReadOnlyPart *part = static_cast<KParts::ReadOnlyPart *>(sender());
    KonqView *view = m_mapViews.value(part);
    if (view) {
        const QString actionName = view->service()->desktopEntryName();
        const QString actionData = view->internalViewMode();
        Q_FOREACH (QAction *action, m_viewModesGroup->actions()) {
            if (action->objectName() == actionName + QLatin1String("-viewmode") &&
                    action->data().toString() == actionData) {
                action->setChecked(true);
                break;
            }
        }
    }
}

void KonqMainWindow::plugViewModeActions()
{
    QList<QAction *> lst;

    if (m_viewModeMenu) {
        lst.append(m_viewModeMenu);
    }

    plugActionList(QStringLiteral("viewmode"), lst);
}

void KonqMainWindow::unplugViewModeActions()
{
    unplugActionList(QStringLiteral("viewmode"));
}

void KonqMainWindow::updateBookmarkBar()
{
    KToolBar *bar = qFindChild<KToolBar *>(this, QStringLiteral("bookmarkToolBar"));
    if (!bar) {
        return;
    }
    if (m_paBookmarkBar && bar->actions().isEmpty()) {
        bar->hide();
    }

}

void KonqMainWindow::closeEvent(QCloseEvent *e)
{
    // This breaks session management (the window is withdrawn in kwin)
    // so let's do this only when closed by the user.

    if (!qApp->isSavingSession()) {
        KonqFrameTabs *tabContainer = m_pViewManager->tabContainer();
        if (tabContainer->count() > 1) {
            KSharedConfig::Ptr config = KSharedConfig::openConfig();
            KConfigGroup cs(config, QStringLiteral("Notification Messages"));

            if (!cs.hasKey("MultipleTabConfirm")) {
                switch (
                    KMessageBox::warningYesNoCancel(
                        this,
                        i18n("You have multiple tabs open in this window, "
                             "are you sure you want to quit?"),
                        i18nc("@title:window", "Confirmation"),
                        KStandardGuiItem::closeWindow(),
                        KGuiItem(i18n("C&lose Current Tab"), QStringLiteral("tab-close")),
                        KStandardGuiItem::cancel(),
                        QStringLiteral("MultipleTabConfirm")
                    )
                ) {
                case KMessageBox::Yes :
                    break;
                case KMessageBox::No :
                    e->ignore();
                    slotRemoveTab();
                    return;
                case KMessageBox::Cancel :
                    e->ignore();
                    return;
                }
            }
        }

        const int originalTabIndex = tabContainer->currentIndex();
        for (int tabIndex = 0; tabIndex < tabContainer->count(); ++tabIndex) {
            KonqFrameBase *tab = tabContainer->tabAt(tabIndex);
            if (!KonqModifiedViewsCollector::collect(tab).isEmpty()) {
                m_pViewManager->showTab(tabIndex);
                const QString question = m_pViewManager->isTabBarVisible()
                                         ? i18n("This tab contains changes that have not been submitted.\nClosing the window will discard these changes.")
                                         : i18n("This page contains changes that have not been submitted.\nClosing the window will discard these changes.");
                if (KMessageBox::warningContinueCancel(
                            this, question,
                            i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"), QStringLiteral("application-exit")),
                            KStandardGuiItem::cancel(), QStringLiteral("discardchangesclose")) != KMessageBox::Continue) {
                    e->ignore();
                    m_pViewManager->showTab(originalTabIndex);
                    return;
                }
            }
        }

        if (settingsDirty() && autoSaveSettings()) {
            saveAutoSaveSettings();
        }

        addClosedWindowToUndoList();
    }
    // We're going to close - tell the parts
    MapViews::ConstIterator it = m_mapViews.constBegin();
    MapViews::ConstIterator end = m_mapViews.constEnd();
    for (; it != end; ++it) {
        if ((*it)->part() && (*it)->part()->widget()) {
            QApplication::sendEvent((*it)->part()->widget(), e);
        }
    }
    KParts::MainWindow::closeEvent(e);
}

void KonqMainWindow::addClosedWindowToUndoList()
{
    qDebug();

    // 1. We get the current title
    int numTabs = m_pViewManager->tabContainer()->childFrameList().count();
    QString title(i18n("no name"));

    if (m_currentView) {
        title = m_currentView->caption();
    }

    // 2. Create the KonqClosedWindowItem and  save its config
    KonqClosedWindowItem *closedWindowItem = new KonqClosedWindowItem(title, m_pUndoManager->newCommandSerialNumber(), numTabs);
    saveProperties(closedWindowItem->configGroup());

    // 3. Add the KonqClosedWindowItem to the undo list
    m_paClosedItems->setEnabled(true);
    m_pUndoManager->addClosedWindowItem(closedWindowItem);

    qDebug() << "done";
}

void KonqMainWindow::updateWindowIcon()
{
    const QString url = m_combo->currentText();
    const QPixmap pix = KonqPixmapProvider::self()->pixmapFor(url, KIconLoader::SizeSmall);
    KParts::MainWindow::setWindowIcon(pix);

    QPixmap big = pix;
    if (!url.isEmpty()) {
        big = KonqPixmapProvider::self()->pixmapFor(url, KIconLoader::SizeMedium);
    }

    KWindowSystem::setIcons(winId(), big, pix);
}

void KonqMainWindow::slotIntro()
{
    openUrl(0, QUrl(QStringLiteral("about:")));
}

void KonqMainWindow::goURL()
{
    QLineEdit *lineEdit = comboEdit();
    if (!lineEdit) {
        return;
    }

    QKeyEvent event(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QChar('\n'));
    QApplication::sendEvent(lineEdit, &event);
}

/**
 * Adds the URL of a KonqView to the closed tabs list.
 * This slot gets called each time a View is closed.
 */
void KonqMainWindow::slotAddClosedUrl(KonqFrameBase *tab)
{
    qDebug();
    QString title(i18n("no name")), url(QStringLiteral("about:blank"));

    // Did the tab contain a single frame, or a splitter?
    KonqFrame *frame = dynamic_cast<KonqFrame *>(tab);
    if (!frame) {
        KonqFrameContainer *frameContainer = dynamic_cast<KonqFrameContainer *>(tab);
        if (frameContainer->activeChildView()) {
            frame = frameContainer->activeChildView()->frame();
        }
    }

    KParts::ReadOnlyPart *part = frame ? frame->part() : 0;
    if (part) {
        url = part->url().url();
    }
    if (frame) {
        title = frame->title().trimmed();
    }
    if (title.isEmpty()) {
        title = url;
    }
    title = KStringHandler::csqueeze(title, 50);

    // Now we get the position of the tab
    const int index =  m_pViewManager->tabContainer()->childFrameList().indexOf(tab);

    KonqClosedTabItem *closedTabItem = new KonqClosedTabItem(url, title, index, m_pUndoManager->newCommandSerialNumber());

    QString prefix = KonqFrameBase::frameTypeToString(tab->frameType()) + QString::number(0);
    closedTabItem->configGroup().writeEntry("RootItem", prefix);
    prefix.append(QLatin1Char('_'));
    KonqFrameBase::Options flags = KonqFrameBase::saveHistoryItems;
    tab->saveConfig(closedTabItem->configGroup(), prefix, flags, 0L, 0, 1);

    m_paClosedItems->setEnabled(true);
    m_pUndoManager->addClosedTabItem(closedTabItem);

    qDebug() << "done";
}

void KonqMainWindow::slotLocationLabelActivated()
{
    focusLocationBar();
    QLineEdit *edit = comboEdit();
    if (edit) {
        edit->selectAll();
    }
}

void KonqMainWindow::slotOpenURL(const QUrl &url)
{
    openUrl(0, url);
}

bool KonqMainWindow::sidebarVisible() const
{
    QAction *a = m_toggleViewGUIClient->action(QStringLiteral("konq_sidebartng"));
    return (a && static_cast<KToggleAction *>(a)->isChecked());
}

bool KonqMainWindow::fullScreenMode() const
{
    return m_ptaFullScreen->isChecked();
}

void KonqMainWindow::slotAddWebSideBar(const QUrl &url, const QString &name)
{
    if (url.isEmpty() && name.isEmpty()) {
        return;
    }

    qDebug() << "Requested to add URL" << url << " [" << name << "] to the sidebar!";

    QAction *a = m_toggleViewGUIClient->action(QStringLiteral("konq_sidebartng"));
    if (!a) {
        KMessageBox::sorry(0, i18n("Your sidebar is not functional or unavailable. A new entry cannot be added."), i18nc("@title:window", "Web Sidebar"));
        return;
    }

    int rc = KMessageBox::questionYesNo(0,
                                        i18n("Add new web extension \"%1\" to your sidebar?",
                                                name.isEmpty() ? name : url.toDisplayString()),
                                        i18nc("@title:window", "Web Sidebar"), KGuiItem(i18n("Add")), KGuiItem(i18n("Do Not Add")));

    if (rc == KMessageBox::Yes) {
        // Show the sidebar
        if (!static_cast<KToggleAction *>(a)->isChecked()) {
            a->trigger();
        }

        // Tell it to add a new panel
        MapViews::ConstIterator it;
        for (it = viewMap().constBegin(); it != viewMap().constEnd(); ++it) {
            KonqView *view = it.value();
            if (view) {
                KService::Ptr svc = view->service();
                if (svc->desktopEntryName() == QLatin1String("konq_sidebartng")) {
                    emit view->browserExtension()->addWebSideBar(url, name);
                    break;
                }
            }
        }
    }
}

void KonqMainWindow::addBookmarksIntoCompletion(const KBookmarkGroup &group)
{
    const QString http = QStringLiteral("http");
    const QString ftp = QStringLiteral("ftp");

    if (group.isNull()) {
        return;
    }

    for (KBookmark bm = group.first();
            !bm.isNull(); bm = group.next(bm)) {
        if (bm.isGroup()) {
            addBookmarksIntoCompletion(bm.toGroup());
            continue;
        }

        QUrl url = bm.url();
        if (!url.isValid()) {
            continue;
        }

        QString u = url.toDisplayString();
        s_pCompletion->addItem(u);

        if (url.isLocalFile()) {
            s_pCompletion->addItem(url.toLocalFile());
        } else if (url.scheme() == http) {
            s_pCompletion->addItem(u.mid(7));
        } else if (url.scheme() == ftp &&
                   url.host().startsWith(ftp)) {
            s_pCompletion->addItem(u.mid(6));
        }
    }
}

//
// the smart popup completion code , <l.lunak@kde.org>
//

// prepend http://www. or http:// if there's no protocol in 's'
// This is used only when there are no completion matches
static QString hp_tryPrepend(const QString &s)
{
    if (s.isEmpty() || s[0] == QLatin1Char('/') || s[0] == QLatin1Char('~')) {
        return QString();
    }

    bool containsSpace = false;

    for (int pos = 0;
            pos < s.length() - 2; // 4 = ://x
            ++pos) {
        if (s[ pos ] == ':' && s[ pos + 1 ] == '/' && s[ pos + 2 ] == '/') {
            return QString();
        }
        if (!s[ pos ].isLetter()) {
            break;
        }
        if (s[pos].isSpace()) {
            containsSpace = true;
            break;
        }
    }

    if (containsSpace || s.at(s.length() - 1).isSpace()) {
        return QString();
    }

    return (s.startsWith(QLatin1String("www.")) ? "http://" : "http://www.") + s;
}

static void hp_removeDupe(KCompletionMatches &l, const QString &dupe,
                          KCompletionMatches::Iterator it_orig)
{
    KCompletionMatches::Iterator it = it_orig + 1;
    while (it != l.end()) {
        if ((*it).value() == dupe) {
            (*it_orig).first = qMax((*it_orig).first, (*it).key());
            it = l.erase(it);
            continue;
        }
        ++it;
    }
}

// remove duplicates like 'http://www.kde.org' and 'http://www.kde.org/'
// (i.e. the trailing slash)
// some duplicates are also created by prepending protocols
static void hp_removeDuplicates(KCompletionMatches &l)
{
    QString http = QStringLiteral("http://");
    QString ftp = QStringLiteral("ftp://ftp.");
    QString file = QStringLiteral("file:");
    QString file2 = QStringLiteral("file://");
    l.removeDuplicates();
    for (KCompletionMatches::Iterator it = l.begin();
            it != l.end();
            ++it) {
        QString str = (*it).value();
        if (str.startsWith(http)) {
            if (str.indexOf('/', 7) < 0) {    // http://something<noslash>
                hp_removeDupe(l, str + '/', it);
                hp_removeDupe(l, str.mid(7) + '/', it);
            } else if (str[ str.length() - 1 ] == '/') {
                hp_removeDupe(l, str.left(str.length() - 1), it);
                hp_removeDupe(l, str.left(str.length() - 1).mid(7), it);
            }
            hp_removeDupe(l, str.mid(7), it);
        } else if (str.startsWith(ftp)) { // ftp://ftp.
            hp_removeDupe(l, str.mid(6), it);    // remove dupes without ftp://
        } else if (str.startsWith(file2)) {
            hp_removeDupe(l, str.mid(7), it);    // remove dupes without file://
        } else if (str.startsWith(file)) {
            hp_removeDupe(l, str.mid(5), it);    // remove dupes without file:
        }
    }
}

static void hp_removeCommonPrefix(KCompletionMatches &l, const QString &prefix)
{
    for (KCompletionMatches::Iterator it = l.begin();
            it != l.end();
        ) {
        if ((*it).value().startsWith(prefix)) {
            it = l.erase(it);
            continue;
        }
        ++it;
    }
}

// don't include common prefixes like 'http://', i.e. when s == 'h', include
// http://hotmail.com but don't include everything just starting with 'http://'
static void hp_checkCommonPrefixes(KCompletionMatches &matches, const QString &s)
{
    static const char *const prefixes[] = {
        "http://",
        "https://",
        "www.",
        "ftp://",
        "http://www.",
        "https://www.",
        "ftp://ftp.",
        "file:",
        "file://",
        NULL
    };
    for (const char *const *pos = prefixes;
            *pos != NULL;
            ++pos) {
        QString prefix = *pos;
        if (prefix.startsWith(s)) {
            hp_removeCommonPrefix(matches, prefix);
        }
    }
}

QStringList KonqMainWindow::historyPopupCompletionItems(const QString &s)
{
    const QString http = QStringLiteral("http://");
    const QString https = QStringLiteral("https://");
    const QString www = QStringLiteral("http://www.");
    const QString wwws = QStringLiteral("https://www.");
    const QString ftp = QStringLiteral("ftp://");
    const QString ftpftp = QStringLiteral("ftp://ftp.");
    const QString file = QStringLiteral("file:"); // without /, because people enter /usr etc.
    const QString file2 = QStringLiteral("file://");
    if (s.isEmpty()) {
        return QStringList();
    }
    KCompletionMatches matches = s_pCompletion->allWeightedMatches(s);
    hp_checkCommonPrefixes(matches, s);
    bool checkDuplicates = false;
    if (!s.startsWith(ftp)) {
        matches += s_pCompletion->allWeightedMatches(ftp + s);
        if (QStringLiteral("ftp.").startsWith(s)) {
            hp_removeCommonPrefix(matches, ftpftp);
        }
        checkDuplicates = true;
    }
    if (!s.startsWith(https)) {
        matches += s_pCompletion->allWeightedMatches(https + s);
        if (QStringLiteral("www.").startsWith(s)) {
            hp_removeCommonPrefix(matches, wwws);
        }
        checkDuplicates = true;
    }
    if (!s.startsWith(http)) {
        matches += s_pCompletion->allWeightedMatches(http + s);
        if (QStringLiteral("www.").startsWith(s)) {
            hp_removeCommonPrefix(matches, www);
        }
        checkDuplicates = true;
    }
    if (!s.startsWith(www)) {
        matches += s_pCompletion->allWeightedMatches(www + s);
        checkDuplicates = true;
    }
    if (!s.startsWith(wwws)) {
        matches += s_pCompletion->allWeightedMatches(wwws + s);
        checkDuplicates = true;
    }
    if (!s.startsWith(ftpftp)) {
        matches += s_pCompletion->allWeightedMatches(ftpftp + s);
        checkDuplicates = true;
    }
    if (!s.startsWith(file)) {
        matches += s_pCompletion->allWeightedMatches(file + s);
        checkDuplicates = true;
    }
    if (!s.startsWith(file2)) {
        matches += s_pCompletion->allWeightedMatches(file2 + s);
        checkDuplicates = true;
    }
    if (checkDuplicates) {
        hp_removeDuplicates(matches);
    }
    QStringList items = matches.list();
    if (items.count() == 0
            && !s.contains(':') && !s.isEmpty() && s[ 0 ] != '/') {
        QString pre = hp_tryPrepend(s);
        if (!pre.isNull()) {
            items += pre;
        }
    }
    return items;
}

#ifndef NDEBUG
void KonqMainWindow::dumpViewList()
{
    qDebug() << m_mapViews.count() << "views:";

    MapViews::Iterator end = m_mapViews.end();
    for (MapViews::Iterator it = m_mapViews.begin(); it != end; ++it) {
        KonqView *view = it.value();
        qDebug() << view << view->part();
    }
}
#endif

void KonqMainWindow::insertChildFrame(KonqFrameBase *frame, int /*index*/)
{
    m_pChildFrame = frame;
    m_pActiveChild = frame;
    frame->setParentContainer(this);
    if (centralWidget() && centralWidget() != frame->asQWidget()) {
        centralWidget()->setParent(0);   // workaround Qt-4.1.2 crash (reported)
        setCentralWidget(0);
    }
    setCentralWidget(frame->asQWidget());
}

void KonqMainWindow::childFrameRemoved(KonqFrameBase *frame)
{
    Q_ASSERT(frame == m_pChildFrame);
    Q_UNUSED(frame)
    m_pChildFrame = 0;
    m_pActiveChild = 0;
}

void KonqMainWindow::saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options, KonqFrameBase *docContainer, int id, int depth)
{
    if (m_pChildFrame) {
        m_pChildFrame->saveConfig(config, prefix, options, docContainer, id, depth);
    }
}

void KonqMainWindow::copyHistory(KonqFrameBase *other)
{
    if (m_pChildFrame) {
        m_pChildFrame->copyHistory(other);
    }
}

void KonqMainWindow::setTitle(const QString &/*title*/, QWidget * /*sender*/)
{
}

void KonqMainWindow::setTabIcon(const QUrl &/*url*/, QWidget * /*sender*/)
{
}

QWidget *KonqMainWindow::asQWidget()
{
    return this;
}

KonqFrameBase::FrameType KonqMainWindow::frameType() const
{
    return KonqFrameBase::MainWindow;
}

KonqFrameBase *KonqMainWindow::childFrame()const
{
    return m_pChildFrame;
}

void KonqMainWindow::setActiveChild(KonqFrameBase * /*activeChild*/)
{
}

void KonqMainWindow::setWorkingTab(int index)
{
    m_workingTab = index;
}

bool KonqMainWindow::isMimeTypeAssociatedWithSelf(const QString &mimeType)
{
    return isMimeTypeAssociatedWithSelf(mimeType, KMimeTypeTrader::self()->preferredService(mimeType, QStringLiteral("Application")));
}

bool KonqMainWindow::isMimeTypeAssociatedWithSelf(const QString &/*mimeType*/, const KService::Ptr &offer)
{
    // Prevention against user stupidity : if the associated app for this mimetype
    // is konqueror/kfmclient, then we'll loop forever. So we have to
    // 1) force embedding first, if that works we're ok
    // 2) check what KRun is going to do before calling it.
    return (offer && (offer->desktopEntryName() == QLatin1String("konqueror") ||
                      offer->exec().trimmed().startsWith(QLatin1String("kfmclient"))));
}

bool KonqMainWindow::refuseExecutingKonqueror(const QString &mimeType)
{
    if (activeViewsNotLockedCount() > 0) {   // if I lock the only view, then there's no error: open links in a new window
        KMessageBox::error(this, i18n("There appears to be a configuration error. You have associated Konqueror with %1, but it cannot handle this file type.", mimeType));
        return true; // we refuse indeed
    }
    return false; // no error
}

bool KonqMainWindow::event(QEvent *e)
{
    if (e->type() == QEvent::StatusTip) {
        if (m_currentView && m_currentView->frame()->statusbar()) {
            KonqFrameStatusBar *statusBar = m_currentView->frame()->statusbar();
            statusBar->message(static_cast<QStatusTipEvent *>(e)->tip());
        }
    }

    if (KonqFileSelectionEvent::test(e) ||
            KonqFileMouseOverEvent::test(e)) {
        // Forward the event to all views
        MapViews::ConstIterator it = m_mapViews.constBegin();
        MapViews::ConstIterator end = m_mapViews.constEnd();
        for (; it != end; ++it) {
            QApplication::sendEvent((*it)->part(), e);
        }
        return true;
    }
    if (KParts::OpenUrlEvent::test(e)) {
        KParts::OpenUrlEvent *ev = static_cast<KParts::OpenUrlEvent *>(e);

        // Forward the event to all views
        MapViews::ConstIterator it = m_mapViews.constBegin();
        MapViews::ConstIterator end = m_mapViews.constEnd();
        for (; it != end; ++it) {
            // Don't resend to sender
            if (it.key() != ev->part()) {
                //qDebug() << "Sending event to view" << it.key()->metaObject()->className();
                QApplication::sendEvent(it.key(), e);
            }
        }
    }
    return KParts::MainWindow::event(e);
}

void KonqMainWindow::slotUndoTextChanged(const QString &newText)
{
    m_paUndo->setText(newText);
}

KonqView *KonqMainWindow::currentView() const
{
    return m_currentView;
}

bool KonqMainWindow::accept(KonqFrameVisitor *visitor)
{
    return visitor->visit(this)
           && (!m_pChildFrame || m_pChildFrame->accept(visitor))
           && visitor->endVisit(this);
}

void KonqMainWindow::applyWindowSizeFromProfile(const KConfigGroup &profileGroup)
{
    // KMainWindow::restoreWindowSize is protected so this logic can't move to KonqViewManager
    const QSize size = KonqViewManager::readDefaultSize(profileGroup, this); // example: "Width=80%"
    if (size.isValid()) {
        resize(size);
    }
    restoreWindowSize(profileGroup); // example: "Width 1400=1120"
}

QLineEdit *KonqMainWindow::comboEdit()
{
    return m_combo ? m_combo->lineEdit() : 0;
}
