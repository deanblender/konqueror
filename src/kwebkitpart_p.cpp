/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */


#include "kwebkitpart_p.h"

#include "webview.h"
#include "webpage.h"
#include "websslinfo.h"
#include "sslinfodialog_p.h"
#include "kwebkitpart.h"
#include "kwebkitpart_ext.h"

#include "settings/webkitsettings.h"
#include "ui/passwordbar.h"
#include "ui/searchbar.h"
#include <kwebwallet.h>

#include <KDE/KLocalizedString>
#include <KDE/KStringHandler>
#include <KDE/KMessageBox>
#include <KDE/KDebug>
#include <KDE/KFileItem>
#include <KDE/KStandardDirs>
#include <KDE/KIconLoader>
#include <KDE/KConfigGroup>
#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KGlobal>
#include <KDE/KLocale>

#include <QtCore/QFile>
#include <QtGui/QApplication>
#include <QtGui/QVBoxLayout>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElement>

#define QL1S(x) QLatin1String(x)
#define QL1C(x) QLatin1Char(x)


static QString htmlError (int code, const QString& text, const KUrl& reqUrl)
{
  QString errorName, techName, description;
  QStringList causes, solutions;

  QByteArray raw = KIO::rawErrorDetail( code, text, &reqUrl );
  QDataStream stream(raw);

  stream >> errorName >> techName >> description >> causes >> solutions;

  QString url, protocol, datetime;
  url = reqUrl.url();
  protocol = reqUrl.protocol();
  datetime = KGlobal::locale()->formatDateTime( QDateTime::currentDateTime(),
                                                KLocale::LongDate );

  QString filename( KStandardDirs::locate( "data", "kwebkitpart/error.html" ) );
  QFile file( filename );
  bool isOpened = file.open( QIODevice::ReadOnly );
  if ( !isOpened )
    kWarning() << "Could not open error html template:" << filename;

  QString html = QString( QL1S( file.readAll() ) );

  html.replace( QL1S( "TITLE" ), i18n( "Error: %1", errorName ) );
  html.replace( QL1S( "DIRECTION" ), QApplication::isRightToLeft() ? "rtl" : "ltr" );
  html.replace( QL1S( "ICON_PATH" ), KUrl(KIconLoader::global()->iconPath("dialog-warning", -KIconLoader::SizeHuge)).url() );

  QString doc = QL1S( "<h1>" );
  doc += i18n( "The requested operation could not be completed" );
  doc += QL1S( "</h1><h2>" );
  doc += errorName;
  doc += QL1S( "</h2>" );

  if ( !techName.isNull() ) {
    doc += QL1S( "<h2>" );
    doc += i18n( "Technical Reason: %1", techName );
    doc += QL1S( "</h2>" );
  }

  doc += QL1S( "<h3>" );
  doc += i18n( "Details of the Request:" );
  doc += QL1S( "</h3><ul><li>" );
  doc += i18n( "URL: %1" ,  url );
  doc += QL1S( "</li><li>" );

  if ( !protocol.isNull() ) {
    doc += i18n( "Protocol: %1", protocol );
    doc += QL1S( "</li><li>" );
  }

  doc += i18n( "Date and Time: %1" ,  datetime );
  doc += QL1S( "</li><li>" );
  doc += i18n( "Additional Information: %1" ,  text );
  doc += QL1S( "</li></ul><h3>" );
  doc += i18n( "Description:" );
  doc += QL1S( "</h3><p>" );
  doc += description;
  doc += QL1S( "</p>" );

  if ( causes.count() ) {
    doc += QL1S( "<h3>" );
    doc += i18n( "Possible Causes:" );
    doc += QL1S( "</h3><ul><li>" );
    doc += causes.join( "</li><li>" );
    doc += QL1S( "</li></ul>" );
  }

  if ( solutions.count() ) {
    doc += QL1S( "<h3>" );
    doc += i18n( "Possible Solutions:" );
    doc += QL1S( "</h3><ul><li>" );
    doc += solutions.join( "</li><li>" );
    doc += QL1S( "</li></ul>" );
  }

  html.replace( QL1S("TEXT"), doc );

  return html;
}


KWebKitPartPrivate::KWebKitPartPrivate(KWebKitPart *parent)
                   :QObject(),
                    updateHistory(true),
                    q(parent)
{
}

void KWebKitPartPrivate::init(QWidget *mainWidget)
{
    // Create the WebView...
    webView = new WebView (q, mainWidget);
    connect(webView, SIGNAL(titleChanged(const QString &)),
            q, SIGNAL(setWindowCaption(const QString &)));
    connect(webView, SIGNAL(loadFinished(bool)),
            this, SLOT(slotLoadFinished(bool)));
    connect(webView, SIGNAL(urlChanged(const QUrl &)),
            this, SLOT(slotUrlChanged(const QUrl &)));
    connect(webView, SIGNAL(linkMiddleOrCtrlClicked(const KUrl &)),
            this, SLOT(slotLinkMiddleOrCtrlClicked(const KUrl &)));

    // Create the search bar...
    searchBar = new KDEPrivate::SearchBar;
    connect(searchBar, SIGNAL(searchTextChanged(const QString &, bool)),
            this, SLOT(slotSearchForText(const QString &, bool)));

    webPage = qobject_cast<WebPage*>(webView->page());
    Q_ASSERT(webPage);

    connect(webPage, SIGNAL(loadStarted()),
            this, SLOT(slotLoadStarted()));
    connect(webPage, SIGNAL(loadAborted(const KUrl &)),
            this, SLOT(slotLoadAborted(const KUrl &)));
    connect(webPage, SIGNAL(navigationRequestFinished(const KUrl &, QWebFrame *)),
            this, SLOT(slotNavigationRequestFinished(const KUrl &, QWebFrame *)));
    connect(webPage, SIGNAL(linkHovered(const QString &, const QString &, const QString &)),
            this, SLOT(slotLinkHovered(const QString &, const QString &, const QString &)));
    connect(webPage, SIGNAL(saveFrameStateRequested(QWebFrame *, QWebHistoryItem *)),
            this, SLOT(slotSaveFrameState(QWebFrame *, QWebHistoryItem *)));
    connect(webPage, SIGNAL(jsStatusBarMessage(const QString &)),
            q, SIGNAL(setStatusBarText(const QString &)));
    connect(webView, SIGNAL(linkShiftClicked(const KUrl &)),
            webPage, SLOT(downloadUrl(const KUrl &)));
    connect(webView, SIGNAL(loadStarted()),
            searchBar, SLOT(hide()));
    connect(webView, SIGNAL(loadStarted()),
            searchBar, SLOT(clear()));

    browserExtension = new WebKitBrowserExtension(q);
    connect(webPage, SIGNAL(loadProgress(int)),
            browserExtension, SIGNAL(loadingProgress(int)));
    connect(webPage, SIGNAL(selectionChanged()),
            browserExtension, SLOT(updateEditActions()));
    connect(browserExtension, SIGNAL(saveUrl(const KUrl&)),
            webPage, SLOT(downloadUrl(const KUrl &)));

    connect(webView, SIGNAL(selectionClipboardUrlPasted(const KUrl &)),
            browserExtension, SIGNAL(openUrlRequest(const KUrl &)));

    KDEPrivate::PasswordBar *passwordBar = new KDEPrivate::PasswordBar(mainWidget);

    // Create the password bar...
    if (webPage->wallet()) {
        connect (webPage->wallet(), SIGNAL(saveFormDataRequested(const QString &, const QUrl &)),
                 passwordBar, SLOT(onSaveFormData(const QString &, const QUrl &)));
        connect(passwordBar, SIGNAL(saveFormDataAccepted(const QString &)),
                webPage->wallet(), SLOT(acceptSaveFormDataRequest(const QString &)));
        connect(passwordBar, SIGNAL(saveFormDataRejected(const QString &)),
                webPage->wallet(), SLOT(rejectSaveFormDataRequest(const QString &)));
    }

    QVBoxLayout* lay = new QVBoxLayout(mainWidget);
    lay->setMargin(0);
    lay->setSpacing(0);
    lay->addWidget(passwordBar);
    lay->addWidget(webView);
    lay->addWidget(searchBar);

    mainWidget->setFocusProxy(webView);
}

void KWebKitPartPrivate::initActions()
{
    KAction *action = q->actionCollection()->addAction(KStandardAction::SaveAs, "saveDocument",
                                                    browserExtension, SLOT(slotSaveDocument()));

    action = new KAction(i18n("Save &Frame As..."), this);
    q->actionCollection()->addAction("saveFrame", action);
    connect(action, SIGNAL(triggered(bool)), browserExtension, SLOT(slotSaveFrame()));

    action = new KAction(KIcon("document-print-frame"), i18n("Print Frame..."), this);
    q->actionCollection()->addAction("printFrame", action);
    connect(action, SIGNAL(triggered(bool)), browserExtension, SLOT(printFrame()));

    action = new KAction(KIcon("zoom-in"), i18nc("zoom in action", "Zoom In"), this);
    q->actionCollection()->addAction("zoomIn", action);
    action->setShortcut(KShortcut("CTRL++; CTRL+="));
    connect(action, SIGNAL(triggered(bool)), browserExtension, SLOT(zoomIn()));

    action = new KAction(KIcon("zoom-out"), i18nc("zoom out action", "Zoom Out"), this);
    q->actionCollection()->addAction("zoomOut", action);
    action->setShortcut(KShortcut("CTRL+-; CTRL+_"));
    connect(action, SIGNAL(triggered(bool)), browserExtension, SLOT(zoomOut()));

    action = new KAction(KIcon("zoom-original"), i18nc("reset zoom action", "Actual Size"), this);
    q->actionCollection()->addAction("zoomNormal", action);
    action->setShortcut(KShortcut("CTRL+0"));
    connect(action, SIGNAL(triggered(bool)), browserExtension, SLOT(zoomNormal()));

    action = new KAction(i18n("Zoom Text Only"), this);
    action->setCheckable(true);
    KConfigGroup cgHtml(KGlobal::config(), "HTML Settings");
    bool zoomTextOnly = cgHtml.readEntry("ZoomTextOnly", false);
    action->setChecked(zoomTextOnly);
    q->actionCollection()->addAction("zoomTextOnly", action);
    connect(action, SIGNAL(triggered(bool)), browserExtension, SLOT(toogleZoomTextOnly()));

    action = q->actionCollection()->addAction(KStandardAction::SelectAll, "selectAll",
                                           browserExtension, SLOT(slotSelectAll()));
    action->setShortcutContext(Qt::WidgetShortcut);
    webView->addAction(action);

    action = new KAction(i18n("View Do&cument Source"), this);
    q->actionCollection()->addAction("viewDocumentSource", action);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
    connect(action, SIGNAL(triggered(bool)), browserExtension, SLOT(slotViewDocumentSource()));

    action = new KAction(i18nc("Secure Sockets Layer", "SSL"), this);
    q->actionCollection()->addAction("security", action);
    connect(action, SIGNAL(triggered(bool)), SLOT(slotShowSecurity()));

    action = q->actionCollection()->addAction(KStandardAction::Find, "find", this, SLOT(slotShowSearchBar()));
    action->setWhatsThis(i18nc("find action \"whats this\" text", "<h3>Find text</h3>"
                              "Shows a dialog that allows you to find text on the displayed page."));

    action = q->actionCollection()->addAction(KStandardAction::FindNext, "findnext",
                                              searchBar, SLOT(findNext()));
    action = q->actionCollection()->addAction(KStandardAction::FindPrev, "findprev",
                                              searchBar, SLOT(findPrevious()));
}


bool KWebKitPartPrivate::handleError(const KUrl &u, QWebFrame *frame, bool handleUserAbort)
{
    if ( u.protocol() == "error" && u.hasSubUrl() ) {
        /**
         * The format of the error url is that two variables are passed in the query:
         * error = int kio error code, errText = QString error text from kio
         * and the URL where the error happened is passed as a sub URL.
         */
        KUrl::List urls = KUrl::split(u);

        if ( urls.count() > 1 ) {
            KUrl mainURL = urls.first();
            int error = mainURL.queryItem( "error" ).toInt();

            // error=0 isn't a valid error code, so 0 means it's missing from the URL
            if ( error == 0 )
                error = KIO::ERR_UNKNOWN;

            if (handleUserAbort && error == KIO::ERR_USER_CANCELED) {
                webView->setUrl(webView->url());
                emit browserExtension->setLocationBarUrl(KUrl(webView->url()).prettyUrl());
            } else {
                const QString errorText = mainURL.queryItem( "errText" );
                urls.pop_front();
                KUrl reqUrl = KUrl::join( urls );
                emit browserExtension->setLocationBarUrl(reqUrl.prettyUrl());                
                frame->setHtml(htmlError(error, errorText, reqUrl));
            }
            return true;
        }
    }

    return false;
}


void KWebKitPartPrivate::slotLoadStarted()
{
    emit q->started(0);
}

void KWebKitPartPrivate::slotLoadFinished(bool ok)
{
    updateHistory = true;

    if (ok) {
        if (WebKitSettings::self()->isFormCompletionEnabled() && webPage->wallet()) {
            webPage->wallet()->fillFormData(webPage->mainFrame());
        }

        QString linkStyle;

        QColor linkColor = WebKitSettings::self()->vLinkColor();
        if (linkColor.isValid())
            linkStyle += QString::fromLatin1("a:visited {color: rgb(%1,%2,%3);}\n")
                         .arg(linkColor.red()).arg(linkColor.green()).arg(linkColor.blue());

        linkColor = WebKitSettings::self()->linkColor();
        if (linkColor.isValid())
            linkStyle += QString::fromLatin1("a:active {color: rgb(%1,%2,%3);}\n")
                         .arg(linkColor.red()).arg(linkColor.green()).arg(linkColor.blue());

        if (WebKitSettings::self()->underlineLink()) {
            linkStyle += QL1S("a:link {text-decoration:underline;}\n");
        } else if (WebKitSettings::self()->hoverLink()) {
            linkStyle += QL1S("a:hover {text-decoration:underline;}\n");
        }

        if (!linkStyle.isEmpty()) {
            webPage->mainFrame()->documentElement().setAttribute(QL1S("style"), linkStyle);
        }

        // Restore page state as necessary...
        webPage->restoreAllFrameState();

        if (webView->title().trimmed().isEmpty()) {
            // If the document title is empty, then set it to the current url
            const QString caption = webView->url().toString((QUrl::RemoveQuery|QUrl::RemoveFragment));
            emit q->setWindowCaption(caption);

            // The urlChanged signal is emitted if and only if the main frame
            // receives the title of the page so we manually invoke the slot as
            // a work around here for pages that do not contain it, such as
            // text documents...
            slotUrlChanged(webView->url());
        }
    }

    /*
      NOTE #1: QtWebKit will not kill a META redirect request even if one
      triggers the WebPage::Stop action!! As such the code below is useless
      and disabled for now.

      NOTE #2: QWebFrame::metaData only provides access to META tags that
      contain a 'name' attribute and completely ignores those that do not.
      This of course includes other META redirect tags such as 'http-equiv'.
      Hence the convoluted code below is needed to extract the META redirect
      period from the list of META tags...
    */
#if 0
    bool refresh = false;
    QMapIterator<QString,QString> it (webView->page()->mainFrame()->metaData());
    while (it.hasNext()) {
      it.next();
      //kDebug() << "meta-key: " << it.key() << "meta-value: " << it.value();
      // HACK: QtWebKit does not parse the value of http-equiv property and
      // as such uses an empty key with a value when
      if (it.key().isEmpty() && it.value().contains(QRegExp("[0-9];url"))) {
        refresh = true;
        break;
      }
    }
    emit q->completed(refresh);
#else
    emit q->completed();
#endif
}

void KWebKitPartPrivate::slotLoadAborted(const KUrl & url)
{
    q->closeUrl();
    if (url.isValid())
      emit browserExtension->openUrlRequest(url);
    else
      q->setUrl(webView->url());
}

void  KWebKitPartPrivate::slotNavigationRequestFinished(const KUrl& url, QWebFrame *frame)
{
    if (frame) {

        if (handleError(url, frame)) {
            return;
        }

        if (frame == webPage->mainFrame()) {
            if (webPage->sslInfo().isValid())
                browserExtension->setPageSecurity(KWebKitPartPrivate::Encrypted);
            else
                browserExtension->setPageSecurity(KWebKitPartPrivate::Unencrypted);
        }
    }
}

void KWebKitPartPrivate::slotUrlChanged(const QUrl& url)
{
    if (url.scheme() != QL1S("error") && url.toString() != QL1S("about:blank")) {
        q->setUrl(url);
        emit browserExtension->setLocationBarUrl(KUrl(url).prettyUrl());
    }
}

void KWebKitPartPrivate::slotShowSecurity()
{
    if (webPage->sslInfo().isValid()) {
        KSslInfoDialog *dlg = new KSslInfoDialog;
        dlg->setSslInfo(webPage->sslInfo().certificateChain(),
                        webPage->sslInfo().peerAddress().toString(),
                        q->url().host(),
                        webPage->sslInfo().protocol(),
                        webPage->sslInfo().ciphers(),
                        webPage->sslInfo().usedChiperBits(),
                        webPage->sslInfo().supportedChiperBits(),
                        KSslInfoDialog::errorsFromString(webPage->sslInfo().certificateErrors()));

        dlg->exec();
    } else {
        KMessageBox::information(0, i18n("The SSL information for this site "
                                         "appears to be corrupt."), i18nc("Secure Sockets Layer", "SSL"));
    }
}

void KWebKitPartPrivate::slotSaveFrameState(QWebFrame *frame, QWebHistoryItem *item)
{
    Q_UNUSED (item);
    if (!frame->parentFrame() && updateHistory) {
        emit browserExtension->openUrlNotify();
    }
}

void KWebKitPartPrivate::slotLinkHovered(const QString &link, const QString &title, const QString &content)
{
    Q_UNUSED(title);
    Q_UNUSED(content);

    QString message;

    if (link.isEmpty()) {
        message = QL1S("");
        emit browserExtension->mouseOverInfo(KFileItem());
    } else {
        QUrl linkUrl (link);
        const QString scheme = linkUrl.scheme();

        if (QString::compare(scheme, QL1S("mailto"), Qt::CaseInsensitive) == 0) {
            message += i18nc("status bar text when hovering email links; looks like \"Email: xy@kde.org - CC: z@kde.org -BCC: x@kde.org - Subject: Hi translator\"", "Email: ");

            // Workaround: for QUrl's parsing deficiencies of "mailto:foo@bar.com".
            if (!linkUrl.hasQuery())
              linkUrl = QUrl(scheme + '?' + linkUrl.path());

            QMap<QString, QStringList> fields;
            QPair<QString, QString> queryItem;

            Q_FOREACH (queryItem, linkUrl.queryItems()) {
                //kDebug() << "query: " << queryItem.first << queryItem.second;
                if (queryItem.first.contains(QL1C('@')) && queryItem.second.isEmpty())
                    fields["to"] << queryItem.first;
                if (QString::compare(queryItem.first, QL1S("to"), Qt::CaseInsensitive) == 0)
                    fields["to"] << queryItem.second;
                if (QString::compare(queryItem.first, QL1S("cc"), Qt::CaseInsensitive) == 0)
                    fields["cc"] << queryItem.second;
                if (QString::compare(queryItem.first, QL1S("bcc"), Qt::CaseInsensitive) == 0)
                    fields["bcc"] << queryItem.second;
                if (QString::compare(queryItem.first, QL1S("subject"), Qt::CaseInsensitive) == 0)
                    fields["subject"] << queryItem.second;
            }

            if (fields.contains(QL1S("to")))
                message += fields.value(QL1S("to")).join(QL1S(", "));
            if (fields.contains(QL1S("cc")))
                message += i18nc("status bar text when hovering email links; looks like \"Email: xy@kde.org - CC: z@kde.org -BCC: x@kde.org - Subject: Hi translator\"", " - CC: ") + fields.value(QL1S("cc")).join(QL1S(", "));
            if (fields.contains(QL1S("bcc")))
                message += i18nc("status bar text when hovering email links; looks like \"Email: xy@kde.org - CC: z@kde.org -BCC: x@kde.org - Subject: Hi translator\"", " - BCC: ") + fields.value(QL1S("bcc")).join(QL1S(", "));
            if (fields.contains(QL1S("subject")))
                message += i18nc("status bar text when hovering email links; looks like \"Email: xy@kde.org - CC: z@kde.org -BCC: x@kde.org - Subject: Hi translator\"", " - Subject: ") + fields.value(QL1S("subject")).join(QL1S(" "));
        } else if (linkUrl.scheme() == QL1S("javascript") &&
                   link.startsWith("javascript:window.open")) {
            message = KStringHandler::rsqueeze(link, 80);
            message += i18n(" (In new window)");
        } else {
            message = link;
            QWebElementCollection collection = webPage->mainFrame()->documentElement().findAll(QL1S("a[href]"));
            Q_FOREACH(const QWebElement &element, collection) {
                //kDebug() << "link:" << link << "href" << element.attribute(QL1S("href"));
                if (element.hasAttribute(QL1S("target")) &&
                    link.contains(element.attribute(QL1S("href")), Qt::CaseInsensitive)) {
                    const QString target = element.attribute(QL1S("target")).toLower().simplified();
                    if (target == QL1S("top") || target == QL1S("_blank")) {
                        message += i18n(" (In new window)");
                        break;
                    }
                    else if (target == QL1S("_parent")) {
                        message += i18n(" (In parent frame)");
                        break;
                    }
                }
            }

            KFileItem item (linkUrl, QString(), KFileItem::Unknown);
            emit browserExtension->mouseOverInfo(item);
        }
    }

    emit q->setStatusBarText(message);
}

void KWebKitPartPrivate::slotSearchForText(const QString &text, bool backward)
{   
    QString matchText;
    QWebPage::FindFlags flags = 0;

    if (text.isEmpty()) {
       matchText = QL1S("");
    } else {
        matchText = text;
        flags = QWebPage::FindWrapsAroundDocument;

        if (backward)
            flags |= QWebPage::FindBackward;

        if (searchBar->caseSensitive())
            flags |= QWebPage::FindCaseSensitively;

        if (searchBar->highlightMatches())
            flags |= QWebPage::HighlightAllOccurrences;
    }

    kDebug() << "matched text:" << matchText << ", backward ?" << backward;
    searchBar->setFoundMatch(webView->page()->findText(matchText, flags));
}

void KWebKitPartPrivate::slotShowSearchBar()
{
    const QString text = webView->selectedText();

    if (text.isEmpty())
        webView->pageAction(QWebPage::Undo);
    else
        searchBar->setSearchText(text.left(150));

    searchBar->show();
}

void KWebKitPartPrivate::slotLinkMiddleOrCtrlClicked(const KUrl& linkUrl)
{
    KParts::OpenUrlArguments args;
    args.setActionRequestedByUser(true);
    args.metaData()["referrer"] = q->url().url();

    emit browserExtension->createNewWindow(linkUrl, args);
}

#include "kwebkitpart_p.moc"
