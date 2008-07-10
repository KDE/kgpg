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

#include <qfile.h>
#include <qcheckbox.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <kcombobox.h>
#include <kdialogbase.h>
#include <kmessagebox.h>
#include <qtextcodec.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qdialog.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qhbuttongroup.h>
#include <qvbuttongroup.h>
#include <qregexp.h>
#include <qcursor.h>

#include <klocale.h>
#include <kprocess.h>
#include <kprocio.h>
#include <klistview.h>
#include <kstatusbar.h>
#include <kconfig.h>
#include <klineedit.h>
#include <ksimpleconfig.h>
#include <kaction.h>
#include <kdebug.h>


#include "kgpgsettings.h"
#include "searchres.h"
#include "detailedconsole.h"
#include "keyservers.h"

keyServer::keyServer(QWidget *parent, const char *name,bool modal,bool autoClose):KDialogBase( Swallow, i18n("Key Server"), Close, Close, parent, name,modal)
{
	autoCloseWindow=autoClose;
	config=new KSimpleConfig ("kgpgrc");
	page=new keyServerWidget();
	setMainWidget(page);

        syncCombobox();
        page->kLEimportid->setFocus();

        connect(page->Buttonimport,SIGNAL(clicked()),this,SLOT(slotImport()));
        connect(page->Buttonsearch,SIGNAL(clicked()),this,SLOT(slotSearch()));
        connect(page->Buttonexport,SIGNAL(clicked()),this,SLOT(slotPreExport()));
        connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));

        connect(page->cBproxyI,SIGNAL(toggled(bool)),this,SLOT(slotEnableProxyI(bool)));
        connect(page->cBproxyE,SIGNAL(toggled(bool)),this,SLOT(slotEnableProxyE(bool)));

        connect(page->kLEimportid,  SIGNAL( textChanged ( const QString & )), this,  SLOT( slotTextChanged( const QString &)));
	page->cBproxyI->setChecked(KGpgSettings::useProxy());
        page->cBproxyE->setChecked(KGpgSettings::useProxy());
        const char *httpproxy = getenv("http_proxy");
        if (httpproxy) {
                page->cBproxyI->setEnabled(true);
                page->cBproxyE->setEnabled(true);
                page->kLEproxyI->setText(httpproxy);
                page->kLEproxyE->setText(httpproxy);
        }


        KProcIO *encid=new KProcIO(QTextCodec::codecForLocale());
        *encid << "gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--with-colon"<<"--list-keys";
        QObject::connect(encid, SIGNAL(readReady(KProcIO *)),this, SLOT(slotprocread(KProcIO *)));
        encid->start(KProcess::NotifyOnExit,true);
        page->Buttonimport->setEnabled( !page->kLEimportid->text().isEmpty());
        page->Buttonsearch->setEnabled( !page->kLEimportid->text().isEmpty());
setMinimumSize(sizeHint());
}


keyServer::~keyServer()
{}


void keyServer::slotTextChanged( const QString &text)
{
    page->Buttonimport->setEnabled( !text.isEmpty());
    page->Buttonsearch->setEnabled( !text.isEmpty());

}
void keyServer::slotEnableProxyI(bool on)
{
        page->kLEproxyI->setEnabled(on);
}

void keyServer::slotEnableProxyE(bool on)
{
        page->kLEproxyE->setEnabled(on);
}



void keyServer::slotprocread(KProcIO *p)
{
        ///////////////////////////////////////////////////////////////// extract  encryption keys
        bool dead;
        QString tst;
	//QPixmap pixkeySingle(KGlobal::iconLoader()->loadIcon("kgpg_key1",KIcon::Small,20));
        while (p->readln(tst)!=-1) {
                //tst=tst.stripWhiteSpace();
                if (tst.startsWith("pub")) {
                        const QString trust=tst.section(':',1,1);
                        QString id=QString("0x"+tst.section(':',4,4).right(8));
                        switch( trust[0] ) {
                        case 'i':
                                dead=true;
                                break;
                        case 'd':
                                dead=true;
                                break;
                        case 'r':
                                dead=true;
                                break;
                        case 'e':
                                dead=true;
                                break;
                        default:
                                dead=false;
                                break;
                        }
                        tst=tst.section(':',9,9);
                        if (tst.length()>35) {
                                tst.remove(35,tst.length());
                                tst+="...";
                        }
                        if ((!dead) && (!tst.isEmpty()))
//                                page->kCBexportkey->insertItem(pixkeySingle,id+": "+tst);
                                page->kCBexportkey->insertItem(id+": "+tst);
                }
        }
}

void keyServer::slotSearch()
{
        if (page->kCBimportks->currentText().isEmpty())
                return;

        if (page->kLEimportid->text().isEmpty()) {
                KMessageBox::sorry(this,i18n("You must enter a search string."));
                return;
        }

        //listpop = new KeyServer( this,"result",WType_Dialog | WShowModal);

	dialogServer=new KDialogBase(KDialogBase::Swallow, i18n("Import Key From Keyserver"),  KDialogBase::Ok | KDialogBase::Close,KDialogBase::Ok,this,0,true);

	dialogServer->setButtonText(KDialogBase::Ok,i18n("&Import"));
	dialogServer->enableButtonOK(false);
	listpop=new searchRes();
	listpop->kLVsearch->setColumnWidthMode(0,QListView::Manual);
	listpop->kLVsearch->setColumnWidthMode(1,QListView::Manual);
	listpop->kLVsearch->setColumnWidth(0,150);
	listpop->kLVsearch->setColumnWidth(1,130);
        listpop->statusText->setText(i18n("Connecting to the server..."));
        connect(listpop->kLVsearch,SIGNAL(selectionChanged()),this,SLOT(transferKeyID()));
        connect(dialogServer,SIGNAL(okClicked()),this,SLOT(preimport()));
        connect(listpop->kLVsearch,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),dialogServer,SIGNAL(okClicked()));

        connect(dialogServer,SIGNAL(closeClicked()),this,SLOT(handleQuit()));
        connect( listpop , SIGNAL( destroyed() ) , this, SLOT( abortSearch()));
        count=0;
        cycle=false;
        readmessage=QString::null;
        searchproc=new KProcIO(QTextCodec::codecForLocale());
        QString keyserv=page->kCBimportks->currentText();
        *searchproc<<"gpg"<<"--utf8-strings";
        if (page->cBproxyI->isChecked()) {
                searchproc->setEnvironment("http_proxy",page->kLEproxyI->text());
                *searchproc<<	"--keyserver-options"<<"honor-http-proxy";
        } else
                *searchproc<<	"--keyserver-options"<<"no-honor-http-proxy";
        *searchproc<<"--keyserver"<<keyserv<<"--command-fd=0"<<"--status-fd=2"<<"--search-keys"<<page->kLEimportid->text().stripWhiteSpace();

        keyNumbers=0;
        QObject::connect(searchproc, SIGNAL(processExited(KProcess *)),this, SLOT(slotsearchresult(KProcess *)));
        QObject::connect(searchproc, SIGNAL(readReady(KProcIO *)),this, SLOT(slotsearchread(KProcIO *)));
        searchproc->start(KProcess::NotifyOnExit,true);
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	dialogServer->setMainWidget(listpop);
	listpop->setMinimumSize(listpop->sizeHint());
	listpop->setMinimumWidth(550);
        dialogServer->exec();
}

void keyServer::handleQuit()
{
	if (searchproc->isRunning())
	{
	    QApplication::restoreOverrideCursor();
	    disconnect(searchproc,0,0,0);
	    searchproc->kill();
	}
	dialogServer->close();
}


void keyServer::abortSearch()
{
        if (dialogServer) {
                delete dialogServer;
                dialogServer=0L;
        }
}

void keyServer::preimport()
{
        transferKeyID();
        if (listpop->kLEID->text().isEmpty()) {
                KMessageBox::sorry(this,i18n("You must choose a key."));
                return;
        }
        page->kLEimportid->setText(listpop->kLEID->text());
        dialogServer->close();
        slotImport();
}

void keyServer::transferKeyID()
{
        if (!listpop->kLVsearch->firstChild())
                return;
        QString kid,keysToSearch;
	listpop->kLEID->clear();
	QPtrList< QListViewItem >searchList=listpop->kLVsearch->selectedItems();

	for ( uint i = 0; i < searchList.count(); ++i )
	{
                if ( searchList.at(i) )
		keysToSearch.append(" "+searchList.at(i)->text(3));
	}
//	kdDebug(2100)<<keysToSearch<<endl;
	listpop->kLEID->setText(keysToSearch.stripWhiteSpace());
}

void keyServer::slotsearchresult(KProcess *)
{
        QString nb;
	dialogServer->enableButtonOK(true);
	QApplication::restoreOverrideCursor();
        nb=nb.setNum(keyNumbers);
        //listpop->kLVsearch->setColumnText(0,i18n("Found %1 matching keys").arg(nb));
        listpop->statusText->setText(i18n("Found %1 matching keys").arg(nb));

        if (listpop->kLVsearch->firstChild()!=NULL) {
                listpop->kLVsearch->setSelected(listpop->kLVsearch->firstChild(),true);
		listpop->kLVsearch->setCurrentItem(listpop->kLVsearch->firstChild());
                transferKeyID();
        }
}

void keyServer::slotsearchread(KProcIO *p)
{
        QString required;
	QString keymail,keyname;
        while (p->readln(required,true)!=-1) {
                //required=QString::fromUtf8(required);

                if (required.find("keysearch.prompt")!=-1) {
                        if (count<4)
                                p->writeStdin("N");
                        else {
                                p->writeStdin("Q");
                                p->closeWhenDone();
                        }
                        required=QString::null;
                }

                if (required.find("GOT_IT")!=-1) {
                        count++;
                        required=QString::null;
                }

                if (required.find("(")==0) {
                        cycle=true;
			QString fullname=required.remove(0,required.find(")")+1).stripWhiteSpace();
			if (fullname.find("<")!=-1) {
	                keymail=fullname.section('<',-1,-1);
			if (keymail.endsWith(">")) keymail.truncate(keymail.length()-1);
                	keyname=fullname.section('<',0,0);
        		} else {
                	keymail=QString::null;
			keyname=fullname;
		        }
                        kitem=new KListViewItem(listpop->kLVsearch,keyname,keymail,QString::null,QString::null);
                        keyNumbers++;
                        count=0;
                        required=QString::null;
                }

                if ((cycle) && (!required.isEmpty())) {
                        QString subkey=required.stripWhiteSpace();
			if (subkey.find(" key ")!=-1 && subkey.at(0).isDigit ())
			{
                        QString kid=subkey.section(",",0,0).stripWhiteSpace();
                        kid=kid.right(8);
			kitem->setText(3,kid);
			QString creation=subkey.section("created",1,1);
			if (creation.startsWith(":")) creation=creation.right(creation.length()-1);
			kitem->setText(2,creation);
			cycle=false;
			}
			else
			{
			if (subkey.find("<")!=-1) {
	                keymail=subkey.section('<',-1,-1);
        	        if (keymail.endsWith(">")) keymail.truncate(keymail.length()-1);
                	keyname=subkey.section('<',0,0);
        		} else {
                	keymail=QString::null;
			keyname=subkey;
		        }
			KListViewItem *subk = new KListViewItem(kitem,keyname,keymail,QString::null,QString::null);
			subk->setSelectable(false);
			}
                        required=QString::null;
                }
        }
}

void keyServer::slotPreExport()
{
	slotExport(QStringList(page->kCBexportkey->currentText().section(':', 0, 0)));
}

void keyServer::slotExport(QStringList keyIds)
{
        if (page->kCBexportks->currentText().isEmpty())
                return;
        readmessage=QString::null;
        exportproc=new KProcIO(QTextCodec::codecForLocale());
        QString keyserv=page->kCBexportks->currentText();

        *exportproc<<"gpg"<<"--utf8-strings";
	if (!page->exportAttributes->isChecked())
                *exportproc<<"--export-options"<<"no-include-attributes";

        if (page->cBproxyE->isChecked()) {
                exportproc->setEnvironment("http_proxy",page->kLEproxyE->text());
                *exportproc<<	"--keyserver-options"<<"honor-http-proxy";
        } else
                *exportproc<<	"--keyserver-options"<<"no-honor-http-proxy";
        *exportproc << "--status-fd=2" << "--keyserver" << keyserv << "--send-keys" << keyIds;

        QObject::connect(exportproc, SIGNAL(processExited(KProcess *)),this, SLOT(slotexportresult(KProcess *)));
        QObject::connect(exportproc, SIGNAL(readReady(KProcIO *)),this, SLOT(slotimportread(KProcIO *)));
        exportproc->start(KProcess::NotifyOnExit,true);
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
        importpop = new QDialog( this,0,true,Qt::WDestructiveClose);
        QVBoxLayout *vbox=new QVBoxLayout(importpop,3);
        QLabel *tex=new QLabel(importpop);
        tex->setText(i18n("<b>Connecting to the server...</b>"));
        QPushButton *Buttonabort=new QPushButton(i18n("&Abort"),importpop);
        vbox->addWidget(tex);
        vbox->addWidget(Buttonabort);
        importpop->setMinimumWidth(250);
        importpop->adjustSize();
        importpop->show();
        connect(importpop,SIGNAL(destroyed ()),this,SLOT(abortExport()));
	connect(Buttonabort,SIGNAL(clicked ()),importpop,SLOT(close()));
}

void keyServer::abortExport()
{
	QApplication::restoreOverrideCursor();
        if (exportproc->isRunning())
	{
	    disconnect(exportproc,0,0,0);
	    exportproc->kill();
	}
}

void keyServer::slotexportresult(KProcess*)
{
	QApplication::restoreOverrideCursor();
        KMessageBox::information(0,readmessage);
        if (importpop)
                delete importpop;
}


void keyServer::slotImport()
{
        if (page->kCBimportks->currentText().isEmpty())
                return;
        if (page->kLEimportid->text().isEmpty()) {
                KMessageBox::sorry(this,i18n("You must enter a search string."));
                return;
        }
        readmessage=QString::null;
        importproc=new KProcIO(QTextCodec::codecForLocale());
        QString keyserv=page->kCBimportks->currentText();

        *importproc<<"gpg"<<"--utf8-strings";
        if (page->cBproxyI->isChecked()) {
                importproc->setEnvironment("http_proxy",page->kLEproxyI->text());
                *importproc<<	"--keyserver-options"<<"honor-http-proxy";
        } else
                *importproc<<	"--keyserver-options"<<"no-honor-http-proxy";

        *importproc<<"--status-fd=2"<<"--keyserver"<<keyserv<<"--recv-keys";
        QString keyNames=page->kLEimportid->text();
        keyNames=keyNames.stripWhiteSpace();
        keyNames=keyNames.simplifyWhiteSpace();
        while (!keyNames.isEmpty()) {
                QString fkeyNames=keyNames.section(' ',0,0);
                keyNames.remove(0,fkeyNames.length());
                keyNames=keyNames.stripWhiteSpace();
                *importproc<<QFile::encodeName(fkeyNames);
        }

        QObject::connect(importproc, SIGNAL(processExited(KProcess *)),this, SLOT(slotimportresult(KProcess *)));
        QObject::connect(importproc, SIGNAL(readReady(KProcIO *)),this, SLOT(slotimportread(KProcIO *)));
        importproc->start(KProcess::NotifyOnExit,true);
	importproc->closeWhenDone();
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
        importpop = new QDialog( this,0,true,Qt::WDestructiveClose);
        QVBoxLayout *vbox=new QVBoxLayout(importpop,3);
        QLabel *tex=new QLabel(importpop);
        tex->setText(i18n("<b>Connecting to the server...</b>"));
        QPushButton *Buttonabort=new QPushButton(i18n("&Abort"),importpop);
        vbox->addWidget(tex);
        vbox->addWidget(Buttonabort);
        importpop->setMinimumWidth(250);
        importpop->adjustSize();
        importpop->show();
	connect(Buttonabort,SIGNAL(clicked()),importpop,SLOT(close()));
	connect(importpop,SIGNAL(destroyed ()),this,SLOT(abortImport()));
}

void keyServer::abortImport()
{
	QApplication::restoreOverrideCursor();
	if (importproc->isRunning())
	{
	    disconnect(importproc,0,0,0);
	    importproc->kill();
	    emit importFinished(QString::null);
	}
	if (autoCloseWindow) close();
}

void keyServer::slotimportresult(KProcess*)
{
	QApplication::restoreOverrideCursor();
        QString importedNb,importedNbSucess,importedNbProcess,resultMessage, parsedOutput,importedNbUnchanged,importedNbSig;
	QString notImportesNbSec,importedNbMissing,importedNbRSA,importedNbUid,importedNbSub,importedNbRev,readNbSec;
	QString importedNbSec,dupNbSec;

        parsedOutput=readmessage;
	QStringList importedKeys;

        while (parsedOutput.find("IMPORTED")!=-1) {
                parsedOutput.remove(0,parsedOutput.find("IMPORTED")+8);
                importedKeys+=parsedOutput.section("\n",0,0).stripWhiteSpace();
        }

        if (readmessage.find("IMPORT_RES")!=-1) {
                importedNb=readmessage.section("IMPORT_RES",-1,-1);
                  importedNb=importedNb.stripWhiteSpace();
                importedNbProcess=importedNb.section(" ",0,0);
		importedNbMissing=importedNb.section(" ",1,1);
                importedNbSucess=importedNb.section(" ",2,2);
		importedNbRSA=importedNb.section(" ",3,3);
		importedNbUnchanged=importedNb.section(" ",4,4);
		importedNbUid=importedNb.section(" ",5,5);
		importedNbSub=importedNb.section(" ",6,6);
		importedNbSig=importedNb.section(" ",7,7);
		importedNbRev=importedNb.section(" ",8,8);
		readNbSec=importedNb.section(" ",9,9);
		importedNbSec=importedNb.section(" ",10,10);
		dupNbSec=importedNb.section(" ",11,11);
		notImportesNbSec=importedNb.section(" ",12,12);
                resultMessage=i18n("<qt>%n key processed.<br></qt>","<qt>%n keys processed.<br></qt>",importedNbProcess.toULong());
		if (importedNbUnchanged!="0")
		resultMessage+=i18n("<qt>One key unchanged.<br></qt>","<qt>%n keys unchanged.<br></qt>",importedNbUnchanged.toULong());
		if (importedNbSig!="0")
		resultMessage+=i18n("<qt>One signature imported.<br></qt>","<qt>%n signatures imported.<br></qt>",importedNbSig.toULong());
		if (importedNbMissing!="0")
		resultMessage+=i18n("<qt>One key without ID.<br></qt>","<qt>%n keys without ID.<br></qt>",importedNbMissing.toULong());
		if (importedNbRSA!="0")
		resultMessage+=i18n("<qt>One RSA key imported.<br></qt>","<qt>%n RSA keys imported.<br></qt>",importedNbRSA.toULong());
		if (importedNbUid!="0")
		resultMessage+=i18n("<qt>One user ID imported.<br></qt>","<qt>%n user IDs imported.<br></qt>",importedNbUid.toULong());
		if (importedNbSub!="0")
		resultMessage+=i18n("<qt>One subkey imported.<br></qt>","<qt>%n subkeys imported.<br></qt>",importedNbSub.toULong());
		if (importedNbRev!="0")
		resultMessage+=i18n("<qt>One revocation certificate imported.<br></qt>","<qt>%n revocation certificates imported.<br></qt>",importedNbRev.toULong());
		if (readNbSec!="0")
		resultMessage+=i18n("<qt>One secret key processed.<br></qt>","<qt>%n secret keys processed.<br></qt>",readNbSec.toULong());
		if (importedNbSec!="0")
		resultMessage+=i18n("<qt><b>One secret key imported.</b><br></qt>","<qt><b>%n secret keys imported.</b><br></qt>",importedNbSec.toULong());
		if (dupNbSec!="0")
		resultMessage+=i18n("<qt>One secret key unchanged.<br></qt>","<qt>%n secret keys unchanged.<br></qt>",dupNbSec.toULong());
		if (notImportesNbSec!="0")
		resultMessage+=i18n("<qt>One secret key not imported.<br></qt>","<qt>%n secret keys not imported.<br></qt>",notImportesNbSec.toULong());
		if (importedNbSucess!="0")
		resultMessage+=i18n("<qt><b>One key imported:</b><br></qt>","<qt><b>%n keys imported:</b><br></qt>",importedNbSucess.toULong());
        } else
                resultMessage=i18n("No key imported... \nCheck detailed log for more infos");

	QString lastID=QString("0x"+importedKeys.last().section(" ",0,0).right(8));
	if (!lastID.isEmpty())
	{
	//kdDebug(2100)<<"++++++++++imported key"<<lastID<<endl;
	emit importFinished(lastID);
	}

        if (importpop)
                importpop->hide();
	(void) new KDetailedInfo(0,"import_result",resultMessage,readmessage,importedKeys);

        if (importpop)
                delete importpop;
	if (autoCloseWindow) close();
}

void keyServer::slotimportread(KProcIO *p)
{
        QString required;
        while (p->readln(required,true)!=-1)
                readmessage+=required+"\n";
}

void keyServer::syncCombobox()
{
        config->setGroup("Servers");
        QString serverList=config->readEntry("Server_List");

        QString optionsServer=KgpgInterface::getGpgSetting("keyserver", KGpgSettings::gpgConfigPath());

	page->kCBexportks->clear();
	page->kCBimportks->clear();

	if (!optionsServer.isEmpty()) {
		page->kCBexportks->insertItem(optionsServer);
		page->kCBimportks->insertItem(optionsServer);
	}
	else if ( serverList.isEmpty() )
		serverList = "hkp://wwwkeys.eu.pgp.net,hkp://search.keyserver.net,hkp://wwwkeys.pgp.net,hkp://pgp.dtype.org,hkp://wwwkeys.us.pgp.net"; // same as ini kgpgoptions.cpp
	
	page->kCBexportks->insertStringList(QStringList::split(",",serverList));
	page->kCBimportks->insertStringList(QStringList::split(",",serverList));
}

void keyServer::slotOk()
{
        accept();
}


#include "keyservers.moc"
