/***************************************************************************
                          keyservers.cpp  -  description
                             -------------------
    begin                : Tue Nov 26 2002
    copyright          : (C) 2002 by Jean-Baptiste Mardelle
    email                : bj@altern.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "keyservers.h"

#include <QCursor>
#include <QLabel>

#include <KConfig>
#include <KMessageBox>
#include <KLocale>
#include <KDebug>
#include <KDateTime>

#include "kgpgsettings.h"
#include "detailedconsole.h"
#include "convert.h"
#include "gpgproc.h"


using namespace KgpgCore;

ConnectionDialog::ConnectionDialog(QWidget *parent)
                : KProgressDialog(parent, i18n("Keyserver"), i18n("<b>Connecting to the server...</b>"))
{
    setModal(true);

    progressBar()->setRange(0, 0);
    show();
}

KeyServer::KeyServer(QWidget *parent, const bool &modal, const bool &autoclose)
         : KDialog(parent)
{
    m_searchproc = NULL;

    setCaption(i18n("Key Server"));
    setButtons(Close);
    setModal(modal);

    m_autoclose = autoclose;

    page = new keyServerWidget();
    setMainWidget(page);

    QStringList serverlist = getServerList();
    page->kCBexportks->addItems(serverlist);
    page->kCBimportks->addItems(serverlist);
    page->kLEimportid->setFocus();

    connect(page->Buttonimport, SIGNAL(clicked()), this, SLOT(slotImport()));
    connect(page->Buttonsearch, SIGNAL(clicked()), this, SLOT(slotSearch()));
    connect(page->Buttonexport, SIGNAL(clicked()), this, SLOT(slotPreExport()));
    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
    connect(page->cBproxyI, SIGNAL(toggled(bool)), this, SLOT(slotEnableProxyI(bool)));
    connect(page->cBproxyE, SIGNAL(toggled(bool)), this, SLOT(slotEnableProxyE(bool)));
    connect(page->kLEimportid, SIGNAL(textChanged(const QString &)), this, SLOT(slotTextChanged(const QString &)));

    page->cBproxyI->setChecked(KGpgSettings::useProxy());
    page->cBproxyE->setChecked(KGpgSettings::useProxy());

    QString httpproxy = qgetenv("http_proxy");
    if (!httpproxy.isEmpty())
    {
        page->cBproxyI->setEnabled(true);
        page->cBproxyE->setEnabled(true);
        page->kLEproxyI->setText(httpproxy);
        page->kLEproxyE->setText(httpproxy);
    }

    KgpgInterface *interface = new KgpgInterface();
    connect (interface, SIGNAL(readPublicKeysFinished(KgpgCore::KgpgKeyList, KgpgInterface*)), this, SLOT(slotReadKeys(KgpgCore::KgpgKeyList, KgpgInterface*)));
    interface->readPublicKeys();

    page->Buttonimport->setEnabled(!page->kLEimportid->text().isEmpty());
    page->Buttonsearch->setEnabled(!page->kLEimportid->text().isEmpty());
    setMinimumSize(sizeHint());
}

void KeyServer::slotReadKeys(KgpgKeyList list, KgpgInterface *interface)
{
    delete interface;
    for (int i = 0; i < list.size(); ++i)
    {
        const KgpgKey key = list.at(i);

        bool dead = false;
        if ((key.trust() == 'i') || (key.trust() == 'd') || (key.trust() == 'r') || (key.trust() == 'e'))
            dead = true;

        if (!dead)
        {
            QString line = key.name();
            if (!key.comment().isEmpty()) line += " (" + key.comment() + ')';
            if (!key.email().isEmpty())    line += " <" + key.email() + '>';
            if (line.length() > 37)
            {
                line.remove(35, line.length());
                line += "...";
            }

            if (!line.isEmpty())
                page->kCBexportkey->addItem(key.id() + ": " + line);
        }
    }
}

void KeyServer::refreshKeys(QStringList keys)
{
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    QString keyserv = getServerList().first();

    QString proxy;
    if (KGpgSettings::useProxy())
        proxy = qgetenv("http_proxy");

    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(downloadKeysFinished(QList<int>, QStringList, bool, QString, KgpgInterface*)), this, SLOT(slotDownloadKeysFinished(QList<int>, QStringList, bool, QString, KgpgInterface*)));
    connect(interface, SIGNAL(downloadKeysAborted(KgpgInterface*)), this, SLOT(slotAbort(KgpgInterface*)));

    m_importpop = new ConnectionDialog(this);
    connect(m_importpop, SIGNAL(cancelClicked()), interface, SLOT(downloadKeysAbort()));

    interface->downloadKeys(keys, keyserv, true, proxy);
}

void KeyServer::slotImport()
{
    if (page->kCBimportks->currentText().isEmpty())
        return;

    if (page->kLEimportid->text().isEmpty())
    {
        KMessageBox::sorry(this, i18n("You must enter a search string."));
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(downloadKeysFinished(QList<int>, QStringList, bool, QString, KgpgInterface*)), this, SLOT(slotDownloadKeysFinished(QList<int>, QStringList, bool, QString, KgpgInterface*)));
    connect(interface, SIGNAL(downloadKeysAborted(KgpgInterface*)), this, SLOT(slotAbort(KgpgInterface*)));

    m_importpop = new ConnectionDialog(this);
    connect(m_importpop, SIGNAL(cancelClicked()), interface, SLOT(downloadKeysAbort()));

    interface->downloadKeys(page->kLEimportid->text().simplified().split(' '), page->kCBimportks->currentText(), false, page->kLEproxyI->text());
}

void KeyServer::slotDownloadKeysFinished(QList<int> result, QStringList keys, bool imported, QString log, KgpgInterface *interface)
{
    delete m_importpop;
    m_importpop = 0;
    delete interface;

    QApplication::restoreOverrideCursor();

    QString resultmessage;
    if (imported)
    {
        if (result[0]  != 0) resultmessage += i18np("<qt>%1 key processed.<br/></qt>", "<qt>%1 keys processed.<br/></qt>", result[0]);
        if (result[4]  != 0) resultmessage += i18np("<qt>%1 key unchanged.<br/></qt>", "<qt>%1 keys unchanged.<br/></qt>", result[4]);
        if (result[7]  != 0) resultmessage += i18np("<qt>%1 signature imported.<br/></qt>", "<qt>%1 signatures imported.<br/></qt>", result[7]);
        if (result[1]  != 0) resultmessage += i18np("<qt>%1 key without ID.<br/></qt>", "<qt>%1 keys without ID.<br/></qt>", result[1]);
        if (result[3]  != 0) resultmessage += i18np("<qt>%1 RSA key imported.<br/></qt>", "<qt>%1 RSA keys imported.<br/></qt>", result[3]);
        if (result[5]  != 0) resultmessage += i18np("<qt>%1 user ID imported.<br/></qt>", "<qt>%1 user IDs imported.<br/></qt>", result[5]);
        if (result[6]  != 0) resultmessage += i18np("<qt>%1 subkey imported.<br/></qt>", "<qt>%1 subkeys imported.<br/></qt>", result[6]);
        if (result[8]  != 0) resultmessage += i18np("<qt>%1 revocation certificate imported.<br/></qt>", "<qt>%1 revocation certificates imported.<br/></qt>", result[8]);
        if (result[9]  != 0) resultmessage += i18np("<qt>%1 secret key processed.<br/></qt>", "<qt>%1 secret keys processed.<br/></qt>", result[9]);
        if (result[10] != 0) resultmessage += i18np("<qt><b>%1 secret key imported.</b><br/></qt>", "<qt><b>%1 secret keys imported.</b><br/></qt>", result[10]);
        if (result[11] != 0) resultmessage += i18np("<qt>%1 secret key unchanged.<br/></qt>", "<qt>%1 secret keys unchanged.<br/></qt>", result[11]);
        if (result[12] != 0) resultmessage += i18np("<qt>%1 secret key not imported.<br/></qt>", "<qt>%1 secret keys not imported.<br/></qt>", result[12]);
        if (result[2]  != 0) resultmessage += i18np("<qt><b>%1 key imported:</b><br/></qt>", "<qt><b>%1 keys imported:</b><br/></qt>", result[2]);
    }
    else
        resultmessage = i18n("No key imported... \nCheck detailed log for more infos");

    if (!keys.empty())
        emit importFinished(keys);

    (void) new KgpgDetailedInfo(0, resultmessage, log, keys);
}

void KeyServer::slotAbort(KgpgInterface *interface)
{
    delete interface;
    QApplication::restoreOverrideCursor();
    if (m_autoclose)
        close();
}

void KeyServer::slotExport(const QString &keyId)
{
    if (page->kCBexportks->currentText().isEmpty())
        return;

    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(uploadKeysFinished(QString, KgpgInterface*)), this, SLOT(slotUploadKeysFinished(QString, KgpgInterface*)));
    connect(interface, SIGNAL(uploadKeysAborted(KgpgInterface*)), this, SLOT(slotAbort(KgpgInterface*)));

    m_importpop = new ConnectionDialog(this);
    connect(m_importpop, SIGNAL(cancelClicked()), interface, SLOT(uploadKeysAbort()));

    interface->uploadKeys(keyId.simplified().split(' '), page->kCBimportks->currentText(), expattr, page->kLEproxyI->text());
}

void KeyServer::slotUploadKeysFinished(QString message, KgpgInterface *interface)
{
    delete m_importpop;
    m_importpop = 0;
    delete interface;

    QApplication::restoreOverrideCursor();

    if (message.isEmpty())
	message = i18n("Upload to keyserver finished without errors");
    KMessageBox::information(this, message);
}

void KeyServer::slotSearch()
{
    if (page->kCBimportks->currentText().isEmpty())
        return;

    if (page->kLEimportid->text().isEmpty())
    {
        KMessageBox::sorry(this, i18n("You must enter a search string."));
        return;
    }

	page->Buttonsearch->setEnabled(false);
	if (m_searchproc)
		return;

    //m_listpop = new KeyServer( this,"result",WType_Dialog | WShowModal);

    m_dialogserver = new KDialog(this );
    m_dialogserver->setCaption( i18n("Import Key From Keyserver") );
    m_dialogserver->setButtons( KDialog::Ok | KDialog::Close );
    m_dialogserver->setDefaultButton( KDialog::Ok);
    m_dialogserver->setModal( true );

    m_dialogserver->setButtonText(KDialog::Ok, i18n("&Import"));
    m_dialogserver->enableButtonOk(false);
    m_listpop = new searchRes(0);
    //m_listpop->setMinimumWidth(250);
    //m_listpop->adjustSize();
    m_listpop->statusText->setText(i18n("Connecting to the server..."));

    connect(m_listpop->kLVsearch, SIGNAL(itemSelectionChanged()), this, SLOT(transferKeyID()));
    connect(m_dialogserver, SIGNAL(okClicked()), this, SLOT(slotPreImport()));
    connect(m_listpop->kLVsearch, SIGNAL(itemActivated(QTreeWidgetItem *, int)), m_dialogserver, SIGNAL(okClicked()));
    connect(m_dialogserver, SIGNAL(closeClicked()), this, SLOT(handleQuit()));
    connect(m_listpop, SIGNAL(destroyed()), this, SLOT(slotAbortSearch()));

    m_listpop->kLVsearch->setSelectionMode(QAbstractItemView::ExtendedSelection);

    m_count = 0;
    m_keyid = QString();
    m_readmessage.clear();
    m_kitem = NULL;

    QString keyserv = page->kCBimportks->currentText();

	bool useproxy = page->cBproxyI->isChecked();
	QString proxy = page->kLEproxyI->text();

	m_searchproc = new GPGProc(this);

	if (useproxy) {
		m_searchproc->setEnvironment(QStringList("http_proxy=" + proxy));
		*m_searchproc << "--keyserver-options" << "honor-http-proxy";
	} else
		*m_searchproc << "--keyserver-options" << "no-honor-http-proxy";
	*m_searchproc << "--keyserver" << keyserv << "--status-fd=1" << "--command-fd=0";
	*m_searchproc << "--with-colons"<< "--search-keys"  << page->kLEimportid->text().simplified().toLocal8Bit();

	m_keynumbers = 0;
	connect(m_searchproc, SIGNAL(processExited(GPGProc *)), this, SLOT(slotSearchResult(GPGProc *)));
	connect(m_searchproc, SIGNAL(readReady(GPGProc *)), this, SLOT(slotSearchRead(GPGProc *)));
	m_searchproc->start();

    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    m_dialogserver->setMainWidget(m_listpop);
    m_listpop->setMinimumSize(m_listpop->sizeHint());
    m_dialogserver->exec();
}

void KeyServer::slotAbortSearch()
{
	if (m_searchproc != NULL) {
		if (m_searchproc->state() == QProcess::Running) {
			QApplication::restoreOverrideCursor();
			disconnect(m_searchproc, 0, 0, 0);
			m_searchproc->kill();
		}
		delete m_searchproc;
		m_searchproc = NULL;
	}
	page->Buttonsearch->setEnabled(true);

    if (m_dialogserver)
    {
        delete m_dialogserver;
        m_dialogserver = 0L;
    }
}

void KeyServer::slotSearchRead(GPGProc *p)
{
    QString required;

    while (p->readln(required, true) >= 0) {
        if (required.contains("keysearch.prompt")) {
            if (m_count < 4)
                p->write("N\n");
            else
                p->write("Q\n");
        } else if (required.contains("GOT_IT")) {
            m_count++;
            required.clear();
        } else if (required.startsWith("pub")) {
            if (m_keyid.length() > 0)
              CreateUidEntry();
            m_keyid = required;
            m_kitem = NULL;
        } else if (required.startsWith("uid")) {
            QString kid = required.section(':', 1, 1);

            if (m_kitem != NULL) {
              QTreeWidgetItem *k = new QTreeWidgetItem(m_kitem);
              k->setText(0, kid);
            } else {
              m_kitem = new QTreeWidgetItem(m_listpop->kLVsearch);
              m_kitem->setText(0, kid);
              m_keynumbers++;
            }
            m_count = 0;
        }
    }
}

void KeyServer::slotSearchResult(GPGProc *p)
{
    // add last key id
    if (m_kitem != NULL)
      CreateUidEntry();

	m_searchproc->deleteLater();
	m_searchproc = NULL;
	page->Buttonsearch->setEnabled(true);
    QString nb;
    m_dialogserver->enableButtonOk(true);
    QApplication::restoreOverrideCursor();
    nb = nb.setNum(m_keynumbers);
    //m_listpop->kLVsearch->setColumnText(0,i18n("Found %1 matching keys").arg(nb));
    m_listpop->statusText->setText(i18n("Found %1 matching keys", nb));

    if (m_listpop->kLVsearch->topLevelItemCount() > 0)
    {
        m_listpop->kLVsearch->setCurrentItem(m_listpop->kLVsearch->topLevelItem(0));
        transferKeyID();
    }
}

void KeyServer::slotSetText(const QString &text)
{
    page->kLEimportid->setText(text);
}

void KeyServer::slotTextChanged(const QString &text)
{
    page->Buttonimport->setEnabled(!text.isEmpty());
    page->Buttonsearch->setEnabled(!text.isEmpty() && (m_searchproc == NULL));
}

void KeyServer::slotSetExportAttribute(const QString *state)
{
    if (state != 0)
        expattr = QString(*state);
    else
        expattr = QString();
}

void KeyServer::slotEnableProxyI(const bool &on)
{
    page->kLEproxyI->setEnabled(on);
}

void KeyServer::slotEnableProxyE(const bool &on)
{
    page->kLEproxyE->setEnabled(on);
}

void KeyServer::transferKeyID()
{
    if (m_listpop->kLVsearch->topLevelItemCount() == 0)
        return;

    QStringList keysToSearch;
    m_listpop->kLEID->clear();
    const QList<QTreeWidgetItem*> &searchList = m_listpop->kLVsearch->selectedItems();

    foreach (QTreeWidgetItem *searchItem, searchList)
    {
        if (searchItem)
        {
            QTreeWidgetItem *item = searchItem->parent();

            if (item == NULL)
                item = searchItem;

            QString id = item->data(0, Qt::UserRole).toString();
            if (!keysToSearch.contains(id))
                keysToSearch << id;
        }
    }

//     kDebug(2100) << keysToSearch;
    m_listpop->kLEID->setText(keysToSearch.join(" "));
}

void KeyServer::slotPreImport()
{
    transferKeyID();
    if (m_listpop->kLEID->text().isEmpty())
    {
        KMessageBox::sorry(this, i18n("You must choose a key."));
        return;
    }
    page->kLEimportid->setText(m_listpop->kLEID->text());
    m_dialogserver->close();
    slotImport();
}

void KeyServer::slotPreExport()
{
    slotExport(page->kCBexportkey->currentText().section(':', 0, 0));
}

void KeyServer::slotOk()
{
    accept();
}

QStringList KeyServer::getServerList()
{
    KConfig config("kgpgrc", KConfig::SimpleConfig);
    KConfigGroup group = config.group("Servers");

    QStringList serverlist;
    serverlist << KgpgInterface::getGpgSetting("keyserver", KGpgSettings::gpgConfigPath());     // From gpg config
    serverlist << group.readEntry("Server_List").split(',', QString::SkipEmptyParts);           // From kgpg config

    return serverlist;
}

void KeyServer::handleQuit()
{
	if (m_searchproc != NULL) {
		if (m_searchproc->state() == QProcess::Running) {
			QApplication::restoreOverrideCursor();
			disconnect(m_searchproc, 0, 0, 0);
			m_searchproc->kill();
		}
		delete m_searchproc;
		m_searchproc = NULL;
	}
	m_dialogserver->close();
	page->Buttonsearch->setEnabled(true);
}

void KeyServer::slotSetKeyserver(const QString &server)
{
    page->kCBimportks->setCurrentIndex(page->kCBimportks->findText(server));
}

void KeyServer::CreateUidEntry(void)
{
    Q_ASSERT(m_keyid.section(':', 1, 1).length() > 0);

    QString id = m_keyid.section(':', 1, 1).right(16);
    KDateTime kd;
    kd.setTime_t(m_keyid.section(':', 4, 4).toULongLong());

    QTreeWidgetItem *k = new QTreeWidgetItem(m_kitem);
    if (m_keyid.section(':', 6, 6) == "r")
    {
        k->setText(0, i18n("ID %1, %2 bit %3 key, created %4 revoked", id, m_keyid.section(':', 3, 3),
            Convert::toString(Convert::toAlgo(m_keyid.section(':', 2, 2))),
            kd.toString(KDateTime::LocalDate)));
    }
    else
    {
        k->setText(0, i18n("ID %1, %2 bit %3 key, created %4", id, m_keyid.section(':', 3, 3),
            Convert::toString(Convert::toAlgo(m_keyid.section(':', 2, 2))),
            kd.toString(KDateTime::LocalDate)));
    }
    m_kitem->setData(0, Qt::UserRole, id);
}

#include "keyservers.moc"
