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

#include <stdlib.h>

#include <QPushButton>
#include <QTextCodec>
#include <QCheckBox>
#include <QCursor>
#include <QLabel>
#include <QFile>

#include <Q3ListViewItem>

#include <ksimpleconfig.h>
#include <kmessagebox.h>
#include <kcombobox.h>
#include <klocale.h>
#include <kprocio.h>
#include <kdebug.h>

#include "kgpgsettings.h"
#include "searchres.h"
#include "detailedconsole.h"
#include "keyserver.h"
#include "keyservers.h"

keyServer::keyServer(QWidget *parent, const char *name, const bool &modal, const bool &autoClose)
         : KDialogBase(Swallow, i18n("Key Server"), Close, Close, parent, name, modal)
{
    m_autoclosewindow = autoClose;
    m_config = new KSimpleConfig("kgpgrc");

    page = new keyServerWidget();
    setMainWidget(page);

    syncCombobox();
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

    QString httpproxy = getenv("http_proxy");
    if (!httpproxy.isEmpty())
    {
        page->cBproxyI->setEnabled(true);
        page->cBproxyE->setEnabled(true);
        page->kLEproxyI->setText(httpproxy);
        page->kLEproxyE->setText(httpproxy);
    }

    KgpgInterface *interface = new KgpgInterface();
    connect (interface, SIGNAL(readPublicKeysFinished(KgpgListKeys, KgpgInterface*)), this, SLOT(slotReadKeys(KgpgListKeys, KgpgInterface*)));
    interface->readPublicKeys();

    page->Buttonimport->setEnabled(!page->kLEimportid->text().isEmpty());
    page->Buttonsearch->setEnabled(!page->kLEimportid->text().isEmpty());
    setMinimumSize(sizeHint());
}

void keyServer::slotReadKeys(KgpgListKeys list, KgpgInterface *interface)
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
            if (!key.comment().isEmpty()) line += " (" + key.comment() + ")";
            if (!key.email().isEmpty())    line += " <" + key.email() + ">";
            if (line.length() > 35)
            {
                line.remove(35, line.length());
                line += "...";
            }

            if (!line.isEmpty())
                page->kCBexportkey->addItem(key.id() + ": " + line);
        }
    }
}

void keyServer::slotImport()
{
    if (page->kCBimportks->currentText().isEmpty())
        return;

    if (page->kLEimportid->text().isEmpty())
    {
        KMessageBox::sorry(this, i18n("You must enter a search string."));
        return;
    }

    m_readmessage = QString::null;
    QString keyserv = page->kCBimportks->currentText();

    m_importproc = new KProcIO();
    *m_importproc << "gpg";
    if (page->cBproxyI->isChecked())
    {
        m_importproc->setEnvironment("http_proxy", page->kLEproxyI->text());
        *m_importproc << "--keyserver-options" << "honor-http-proxy";
    }
    else
        *m_importproc << "--keyserver-options" << "no-honor-http-proxy";

    *m_importproc << "--status-fd=2" << "--keyserver" << keyserv << "--recv-keys";

    QString keyNames = page->kLEimportid->text().simplified();
    while (!keyNames.isEmpty())
    {
        QString fkeyNames = keyNames.section(' ', 0, 0);
        *m_importproc << QFile::encodeName(fkeyNames);
        keyNames.remove(0, fkeyNames.length());
        keyNames = keyNames.simplified();
    }

    connect(m_importproc, SIGNAL(processExited(KProcess *)), this, SLOT(slotImportResult(KProcess *)));
    connect(m_importproc, SIGNAL(readReady(KProcIO *)), this, SLOT(slotImportRead(KProcIO *)));
    m_importproc->start(KProcess::NotifyOnExit, true);
    m_importproc->closeWhenDone();


    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    m_importpop = new QDialog(this, 0, true, Qt::WDestructiveClose);

    QLabel *tex = new QLabel(m_importpop);
    tex->setText(i18n("<b>Connecting to the server...</b>"));

    QPushButton *buttonabort = new QPushButton(i18n("&Abort"), m_importpop);

    QVBoxLayout *vbox = new QVBoxLayout(m_importpop);
    vbox->setSpacing(3);
    vbox->addWidget(tex);
    vbox->addWidget(buttonabort);

    connect(buttonabort, SIGNAL(clicked()), m_importpop, SLOT(close()));
    connect(m_importpop, SIGNAL(destroyed()), this, SLOT(slotAbortImport()));

    m_importpop->setMinimumWidth(250);
    m_importpop->adjustSize();
    m_importpop->show();
}

void keyServer::slotAbortImport()
{
    QApplication::restoreOverrideCursor();
    if (m_importproc->isRunning())
    {
        disconnect(m_importproc, 0, 0, 0);
        m_importproc->kill();
        emit importFinished(QString::null);
    }

    if (m_autoclosewindow)
        close();
}

void keyServer::slotImportRead(KProcIO *p)
{
    QString required;
    while (p->readln(required, true) != -1)
        m_readmessage += required + "\n";
}

void keyServer::slotImportResult(KProcess *p)
{
    QApplication::restoreOverrideCursor();
    QString importedNb;
    QString importedNbSucess;
    QString importedNbProcess;
    QString resultMessage;
    QString parsedOutput;
    QString importedNbUnchanged;
    QString importedNbSig;
    QString notImportesNbSec;
    QString importedNbMissing;
    QString importedNbRSA;
    QString importedNbUid;
    QString importedNbSub;
    QString importedNbRev;
    QString readNbSec;
    QString importedNbSec;
    QString dupNbSec;

    parsedOutput = m_readmessage;
    QStringList importedKeys;

    while (parsedOutput.contains("IMPORTED"))
    {
        parsedOutput.remove(0, parsedOutput.indexOf("IMPORTED") + 8);
        importedKeys += parsedOutput.section("\n", 0, 0).simplified();
    }

    if (m_readmessage.contains("IMPORT_RES"))
    {
        importedNb = m_readmessage.section("IMPORT_RES", -1, -1);
        importedNb = importedNb.simplified();
        importedNbProcess = importedNb.section(" ", 0, 0);
        importedNbMissing = importedNb.section(" ", 1, 1);
        importedNbSucess = importedNb.section(" ", 2, 2);
        importedNbRSA = importedNb.section(" ", 3, 3);
        importedNbUnchanged = importedNb.section(" ", 4, 4);
        importedNbUid = importedNb.section(" ", 5, 5);
        importedNbSub = importedNb.section(" ", 6, 6);
        importedNbSig = importedNb.section(" ", 7, 7);
        importedNbRev = importedNb.section(" ", 8, 8);
        readNbSec = importedNb.section(" ", 9, 9);
        importedNbSec = importedNb.section(" ", 10, 10);
        dupNbSec = importedNb.section(" ", 11, 11);
        notImportesNbSec = importedNb.section(" ", 12, 12);

        resultMessage = i18np("<qt>%n key processed.<br></qt>", "<qt>%n keys processed.<br></qt>", importedNbProcess.toULong());
        if (importedNbUnchanged != "0")
            resultMessage += i18np("<qt>One key unchanged.<br></qt>", "<qt>%n keys unchanged.<br></qt>", importedNbUnchanged.toULong());
        if (importedNbSig != "0")
            resultMessage += i18np("<qt>One signature imported.<br></qt>", "<qt>%n signatures imported.<br></qt>", importedNbSig.toULong());
        if (importedNbMissing != "0")
            resultMessage += i18np("<qt>One key without ID.<br></qt>", "<qt>%n keys without ID.<br></qt>", importedNbMissing.toULong());
        if (importedNbRSA != "0")
            resultMessage += i18np("<qt>One RSA key imported.<br></qt>", "<qt>%n RSA keys imported.<br></qt>", importedNbRSA.toULong());
        if (importedNbUid != "0")
            resultMessage += i18np("<qt>One user ID imported.<br></qt>", "<qt>%n user IDs imported.<br></qt>", importedNbUid.toULong());
        if (importedNbSub != "0")
            resultMessage += i18np("<qt>One subkey imported.<br></qt>", "<qt>%n subkeys imported.<br></qt>", importedNbSub.toULong());
        if (importedNbRev != "0")
            resultMessage += i18np("<qt>One revocation certificate imported.<br></qt>", "<qt>%n revocation certificates imported.<br></qt>", importedNbRev.toULong());
        if (readNbSec != "0")
            resultMessage += i18np("<qt>One secret key processed.<br></qt>", "<qt>%n secret keys processed.<br></qt>", readNbSec.toULong());
        if (importedNbSec != "0")
            resultMessage += i18np("<qt><b>One secret key imported.</b><br></qt>", "<qt><b>%n secret keys imported.</b><br></qt>", importedNbSec.toULong());
        if (dupNbSec != "0")
            resultMessage += i18np("<qt>One secret key unchanged.<br></qt>", "<qt>%n secret keys unchanged.<br></qt>", dupNbSec.toULong());
        if (notImportesNbSec != "0")
            resultMessage += i18np("<qt>One secret key not imported.<br></qt>", "<qt>%n secret keys not imported.<br></qt>", notImportesNbSec.toULong());
        if (importedNbSucess != "0")
            resultMessage += i18np("<qt><b>One key imported:</b><br></qt>", "<qt><b>%n keys imported:</b><br></qt>", importedNbSucess.toULong());
    }
    else
        resultMessage = i18n("No key imported... \nCheck detailed log for more infos");

    QString lastID = QString("0x" + importedKeys.last().section(" ", 0, 0).right(8));
    if (!lastID.isEmpty())
    {
        //kDebug(2100)<<"++++++++++imported key"<<lastID<<endl;
        emit importFinished(lastID);
    }

    if (m_importpop)
        m_importpop->hide();

    (void) new KgpgDetailedInfo(0, "import_result", resultMessage, m_readmessage, importedKeys);

    delete m_importpop;
    delete p;

    if (m_autoclosewindow)
        close();
}

void keyServer::slotExport(const QString &keyId)
{
    if (page->kCBexportks->currentText().isEmpty())
        return;

    m_readmessage = QString::null;
    m_exportproc = new KProcIO();
    QString keyserv = page->kCBexportks->currentText();

    *m_exportproc << "gpg";
    if (!page->exportAttributes->isChecked())
        *m_exportproc << "--export-options" << "no-include-attributes";

    if (page->cBproxyE->isChecked())
    {
        m_exportproc->setEnvironment("http_proxy", page->kLEproxyE->text());
        *m_exportproc << "--keyserver-options" << "honor-http-proxy";
    }
    else
        *m_exportproc << "--keyserver-options" << "no-honor-http-proxy";

    *m_exportproc << "--status-fd=2" << "--keyserver" << keyserv << "--send-keys" << keyId;

    connect(m_exportproc, SIGNAL(processExited(KProcess *)), this, SLOT(slotExportResult(KProcess *)));
    connect(m_exportproc, SIGNAL(readReady(KProcIO *)), this, SLOT(slotImportRead(KProcIO *)));
    m_exportproc->start(KProcess::NotifyOnExit, true);

    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    m_importpop = new QDialog(this, 0, true, Qt::WDestructiveClose);
    QVBoxLayout *vbox = new QVBoxLayout(m_importpop);
    vbox->setSpacing(3);

    QLabel *tex = new QLabel(m_importpop);
    tex->setText(i18n("<b>Connecting to the server...</b>"));

    QPushButton *Buttonabort = new QPushButton(i18n("&Abort"), m_importpop);
    vbox->addWidget(tex);
    vbox->addWidget(Buttonabort);
    m_importpop->setMinimumWidth(250);
    m_importpop->adjustSize();

    connect(m_importpop, SIGNAL(destroyed()), this, SLOT(slotAbortExport()));
    connect(Buttonabort, SIGNAL(clicked()), m_importpop, SLOT(close()));

    m_importpop->show();
}

void keyServer::slotAbortExport()
{
    QApplication::restoreOverrideCursor();
    if (m_exportproc->isRunning())
    {
        disconnect(m_exportproc, 0, 0, 0);
        m_exportproc->kill();
    }
}

void keyServer::slotExportResult(KProcess*)
{
    QApplication::restoreOverrideCursor();
    KMessageBox::information(0, m_readmessage);
    delete m_importpop;
}

void keyServer::slotSearch()
{
    if (page->kCBimportks->currentText().isEmpty())
        return;

    if (page->kLEimportid->text().isEmpty())
    {
        KMessageBox::sorry(this, i18n("You must enter a search string."));
        return;
    }

    //m_listpop = new KeyServer( this,"result",WType_Dialog | WShowModal);

    m_dialogserver = new KDialogBase(KDialogBase::Swallow, i18n("Import Key From Keyserver"),  KDialogBase::Ok | KDialogBase::Close, KDialogBase::Ok, this, 0, true);

    m_dialogserver->setButtonText(KDialogBase::Ok, i18n("&Import"));
    m_dialogserver->enableButtonOK(false);
    m_listpop = new searchRes();
    //m_listpop->setMinimumWidth(250);
    //m_listpop->adjustSize();
    m_listpop->statusText->setText(i18n("Connecting to the server..."));

    connect(m_listpop->kLVsearch, SIGNAL(selectionChanged()), this, SLOT(transferKeyID()));
    connect(m_dialogserver, SIGNAL(okClicked()), this, SLOT(slotPreImport()));
    connect(m_listpop->kLVsearch, SIGNAL(doubleClicked(Q3ListViewItem *, const QPoint &, int)), m_dialogserver, SIGNAL(okClicked()));
    //connect(m_listpop->kLVsearch,SIGNAL(returnPressed ( Q3ListViewItem * )),this,SLOT(slotPreImport()));
    connect(m_dialogserver, SIGNAL(closeClicked()), this, SLOT(handleQuit()));
    connect(m_listpop, SIGNAL(destroyed()), this, SLOT(slotAbortSearch()));

    m_count = 0;
    m_cycle = false;
    m_readmessage = QString::null;
    QString keyserv = page->kCBimportks->currentText();

    m_searchproc = new KProcIO();
    *m_searchproc << "gpg";
    if (page->cBproxyI->isChecked())
    {
        m_searchproc->setEnvironment("http_proxy", page->kLEproxyI->text());
        *m_searchproc << "--keyserver-options" << "honor-http-proxy";
    }
    else
        *m_searchproc << "--keyserver-options" << "no-honor-http-proxy";

    *m_searchproc << "--keyserver" << keyserv << "--command-fd=0" << "--status-fd=2" << "--search-keys" << page->kLEimportid->text().simplified().toLocal8Bit();

    m_keynumbers = 0;
    connect(m_searchproc, SIGNAL(processExited(KProcess *)), this, SLOT(slotSearchResult(KProcess *)));
    connect(m_searchproc, SIGNAL(readReady(KProcIO *)), this, SLOT(slotSearchRead(KProcIO *)));
    m_searchproc->start(KProcess::NotifyOnExit, true);

    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    m_dialogserver->setMainWidget(m_listpop);
    m_listpop->setMinimumSize(m_listpop->sizeHint());
    m_dialogserver->exec();
}

void keyServer::slotAbortSearch()
{
    if (m_dialogserver)
    {
        delete m_dialogserver;
        m_dialogserver = 0L;
    }
}

void keyServer::slotSearchRead(KProcIO *p)
{
    QString required;
    while (p->readln(required, true) != -1)
    {
        //required=QString::fromUtf8(required);
        if (required.contains("keysearch.prompt"))
        {
            if (m_count < 4)
                p->writeStdin(QByteArray("N"));
            else
            {
                p->writeStdin(QByteArray("Q"));
                p->closeWhenDone();
            }

            required = QString::null;
        }

        if (required.contains("GOT_IT"))
        {
            m_count++;
            required = QString::null;
        }

        if ((m_cycle) && (!required.isEmpty()))
        {
            QString kid = required.simplified();
            (void) new Q3ListViewItem(m_kitem, kid);
            kid = kid.section("key", 1, 1);
            kid = kid.simplified();
            kid = kid.left(8);
            required = QString::null;
        }

        m_cycle = false;

        if (required.contains('(') && !required.isEmpty())
        {
            m_cycle = true;
            m_kitem = new Q3ListViewItem(m_listpop->kLVsearch, required.remove(0, required.indexOf(')') + 1).simplified());
            m_keynumbers++;
            m_count = 0;
            required = QString::null;
        }
    }
}

void keyServer::slotSearchResult(KProcess *)
{
    QString nb;
    m_dialogserver->enableButtonOK(true);
    QApplication::restoreOverrideCursor();
    nb = nb.setNum(m_keynumbers);
    //m_listpop->kLVsearch->setColumnText(0,i18n("Found %1 matching keys").arg(nb));
    m_listpop->statusText->setText(i18n("Found %1 matching keys", nb));

    if (m_listpop->kLVsearch->firstChild() != NULL)
    {
        m_listpop->kLVsearch->setSelected(m_listpop->kLVsearch->firstChild(), true);
        transferKeyID();
    }
}

void keyServer::slotSetText(const QString &text)
{
    page->kLEimportid->setText(text);
}

void keyServer::slotTextChanged(const QString &text)
{
    page->Buttonimport->setEnabled(!text.isEmpty());
    page->Buttonsearch->setEnabled(!text.isEmpty());
}

void keyServer::slotSetExportAttribute(const bool &state)
{
    page->exportAttributes->setChecked(state);
}

void keyServer::slotEnableProxyI(const bool &on)
{
    page->kLEproxyI->setEnabled(on);
}

void keyServer::slotEnableProxyE(const bool &on)
{
    page->kLEproxyE->setEnabled(on);
}

void keyServer::transferKeyID()
{
    if (!m_listpop->kLVsearch->firstChild())
        return;

    QString kid;
    QString keysToSearch;
    m_listpop->kLEID->clear();
    QList<Q3ListViewItem*>searchList = m_listpop->kLVsearch->selectedItems();

    for (int i = 0; i < searchList.count(); ++i)
    {
        if (searchList.at(i))
        {
            if (searchList.at(i)->depth() == 0)
                kid = searchList.at(i)->firstChild()->text(0).simplified();
            else
                kid = searchList.at(i)->text(0).simplified();

            kid = kid.section("key", 1, 1);
            kid = kid.simplified();
            keysToSearch.append(" " + kid.left(8));
        }
    }

    kDebug(2100) << keysToSearch << endl;
    m_listpop->kLEID->setText(keysToSearch.simplified());
}

void keyServer::slotPreImport()
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

void keyServer::slotPreExport()
{
    slotExport(page->kCBexportkey->currentText().section(':', 0, 0));
}

void keyServer::slotOk()
{
    accept();
}

void keyServer::syncCombobox()
{
    m_config->setGroup("Servers");
    QString serverList = m_config->readEntry("Server_List");

    QString optionsServer = KgpgInterface::getGpgSetting("keyserver", KGpgSettings::gpgConfigPath());

    page->kCBexportks->clear();
    page->kCBimportks->clear();

    if (!optionsServer.isEmpty()) {
	page->kCBexportks->addItem(optionsServer);
	page->kCBimportks->addItem(optionsServer);
    }

    page->kCBexportks->addItems(serverList.split(","));
    page->kCBimportks->addItems(serverList.split(","));
}

void keyServer::handleQuit()
{
    if (m_searchproc->isRunning())
    {
        QApplication::restoreOverrideCursor();
        disconnect(m_searchproc, 0, 0, 0);
        m_searchproc->kill();
    }
    m_dialogserver->close();
}

#include "keyservers.moc"
