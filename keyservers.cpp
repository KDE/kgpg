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

#include <klocale.h>
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


        KProcIO *encid=new KProcIO();
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
                        tst=KgpgInterface::checkForUtf8(tst.section(':',9,9));
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
	
	dialogServer=new KDialogBase(KDialogBase::Swallow, i18n("Key Properties"),  KDialogBase::Ok | KDialogBase::Close,KDialogBase::Ok,this,0,true);
	
	dialogServer->setButtonText(KDialogBase::Ok,i18n("&Import"));
	listpop=new searchRes();
        //listpop->setMinimumWidth(250);
        //listpop->adjustSize();
        listpop->statusText->setText(i18n("Connecting to the server..."));   
        
        connect(listpop->kLVsearch,SIGNAL(selectionChanged()),this,SLOT(transferKeyID()));
        connect(dialogServer,SIGNAL(okClicked()),this,SLOT(preimport()));
        connect(listpop->kLVsearch,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),dialogServer,SIGNAL(okClicked()));
        //connect(listpop->kLVsearch,SIGNAL(returnPressed ( QListViewItem * )),this,SLOT(preimport()));

        connect(dialogServer,SIGNAL(closeClicked()),this,SLOT(handleQuit()));
        connect( listpop , SIGNAL( destroyed() ) , this, SLOT( abortSearch()));
        count=0;
        cycle=false;
        readmessage=QString::null;
        KProcIO *searchproc=new KProcIO();
        QString keyserv=page->kCBimportks->currentText();
        *searchproc<<"gpg";
        if (page->cBproxyI->isChecked()) {
                searchproc->setEnvironment("http_proxy",page->kLEproxyI->text());
                *searchproc<<	"--keyserver-options"<<"honor-http-proxy";
        } else
                *searchproc<<	"--keyserver-options"<<"no-honor-http-proxy";
        *searchproc<<"--keyserver"<<keyserv<<"--command-fd=0"<<"--status-fd=2"<<"--search-keys"<<page->kLEimportid->text().stripWhiteSpace().local8Bit();

        keyNumbers=0;
        QObject::connect(searchproc, SIGNAL(processExited(KProcess *)),this, SLOT(slotsearchresult(KProcess *)));
        QObject::connect(searchproc, SIGNAL(readReady(KProcIO *)),this, SLOT(slotsearchread(KProcIO *)));
        searchproc->start(KProcess::NotifyOnExit,true);
	
	dialogServer->setMainWidget(listpop);
	listpop->setMinimumSize(listpop->sizeHint());
        dialogServer->exec();
}

void keyServer::handleQuit()
{
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
		{
                    if (searchList.at(i)->depth()==0)
                	kid=searchList.at(i)->firstChild()->text(0).stripWhiteSpace();
        		else
                	kid=searchList.at(i)->text(0).stripWhiteSpace();
		//
        	kid=kid.section("key",1,1);
        	kid=kid.stripWhiteSpace();
		keysToSearch.append(" "+kid.left(8));
		}
	}
	kdDebug(2100)<<keysToSearch<<endl;
	listpop->kLEID->setText(keysToSearch.stripWhiteSpace());
}

void keyServer::slotsearchresult(KProcess *)
{
        QString nb;
        nb=nb.setNum(keyNumbers);
        //listpop->kLVsearch->setColumnText(0,i18n("Found %1 matching keys").arg(nb));
        listpop->statusText->setText(i18n("Found %1 matching keys").arg(nb));

        if (listpop->kLVsearch->firstChild()!=NULL) {
                listpop->kLVsearch->setSelected(listpop->kLVsearch->firstChild(),true);
                transferKeyID();
        }
}

void keyServer::slotsearchread(KProcIO *p)
{
        QString required;
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

                if ((cycle) && (!required.isEmpty())) {
                        QString kid=required.stripWhiteSpace();
                        (void) new KListViewItem(kitem,kid);
                        kid=kid.section("key",1,1);
                        kid=kid.stripWhiteSpace();
                        kid=kid.left(8);
                        required=QString::null;
                }

                cycle=false;

                if ((required.find("(")!=-1) && (!required.isEmpty())) {
                        cycle=true;
                        kitem=new KListViewItem(listpop->kLVsearch,required.remove(0,required.find(")")+1).stripWhiteSpace());
                        keyNumbers++;
                        count=0;
                        required=QString::null;
                }
        }
}

void keyServer::slotPreExport()
{
slotExport(page->kCBexportkey->currentText().section(':',0,0));
}

void keyServer::slotExport(QString keyId)
{
        if (page->kCBexportks->currentText().isEmpty())
                return;
        readmessage=QString::null;
        exportproc=new KProcIO();
        QString keyserv=page->kCBexportks->currentText();

        *exportproc<<"gpg";
	if (!page->exportAttributes->isChecked())
                *exportproc<<"--export-options"<<"no-include-attributes";
	
        if (page->cBproxyE->isChecked()) {
                exportproc->setEnvironment("http_proxy",page->kLEproxyE->text());
                *exportproc<<	"--keyserver-options"<<"honor-http-proxy";
        } else
                *exportproc<<	"--keyserver-options"<<"no-honor-http-proxy";
        *exportproc<<"--status-fd=2"<<"--keyserver"<<keyserv<<"--send-keys"<<keyId;

        QObject::connect(exportproc, SIGNAL(processExited(KProcess *)),this, SLOT(slotexportresult(KProcess *)));
        QObject::connect(exportproc, SIGNAL(readReady(KProcIO *)),this, SLOT(slotimportread(KProcIO *)));
        exportproc->start(KProcess::NotifyOnExit,true);

        importpop = new QDialog( this,0,true);
        QVBoxLayout *vbox=new QVBoxLayout(importpop,3);
        QLabel *tex=new QLabel(importpop);
        tex->setText(i18n("<b>Connecting to the server...</b>"));
        QPushButton *Buttonabort=new QPushButton(i18n("&Abort"),importpop);
        vbox->addWidget(tex);
        vbox->addWidget(Buttonabort);
        importpop->setMinimumWidth(250);
        importpop->adjustSize();
        importpop->show();
        connect(Buttonabort,SIGNAL(clicked ()),this,SLOT(abortExport()));
}

void keyServer::abortExport()
{
        if (importpop)
                delete importpop;
        if (exportproc)
                delete exportproc;
}

void keyServer::slotexportresult(KProcess*)
{
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
        importproc=new KProcIO();
        QString keyserv=page->kCBimportks->currentText();

        *importproc<<"gpg";
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

        importpop = new QDialog( this,0,true);
        QVBoxLayout *vbox=new QVBoxLayout(importpop,3);
        QLabel *tex=new QLabel(importpop);
        tex->setText(i18n("<b>Connecting to the server...</b>"));
        QPushButton *Buttonabort=new QPushButton(i18n("&Abort"),importpop);
        vbox->addWidget(tex);
        vbox->addWidget(Buttonabort);
        importpop->setMinimumWidth(250);
        importpop->adjustSize();
        importpop->show();
        connect(Buttonabort,SIGNAL(clicked()),this,SLOT(abortImport()));
}

void keyServer::abortImport()
{
        if (importpop)
                delete importpop;
        if (importproc)
                delete importproc;
	if (autoCloseWindow) close();
}

void keyServer::slotimportresult(KProcess*)
{
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
                resultMessage=i18n("<qt>%1 key(s) processed.<br></qt>").arg(importedNbProcess);
		if (importedNbUnchanged!="0")
		resultMessage+=i18n("<qt>%1 key(s) unchanged.<br></qt>").arg(importedNbUnchanged);
		if (importedNbSig!="0")
		resultMessage+=i18n("<qt>%1 signature(s) imported.<br></qt>").arg(importedNbSig);
		if (importedNbMissing!="0")
		resultMessage+=i18n("<qt>%1 key(s) without ID.<br></qt>").arg(importedNbMissing);
		if (importedNbRSA!="0")
		resultMessage+=i18n("<qt>%1 RSA key(s) imported.<br></qt>").arg(importedNbRSA);
		if (importedNbUid!="0")
		resultMessage+=i18n("<qt>%1 user ID(s) imported.<br></qt>").arg(importedNbUid);
		if (importedNbSub!="0")
		resultMessage+=i18n("<qt>%1 subkey(s) imported.<br></qt>").arg(importedNbSub);
		if (importedNbRev!="0")
		resultMessage+=i18n("<qt>%1 revocation certificate(s) imported.<br></qt>").arg(importedNbRev);
		if (readNbSec!="0")
		resultMessage+=i18n("<qt>%1 secret key(s) processed.<br></qt>").arg(readNbSec);
		if (importedNbSec!="0")
		resultMessage+=i18n("<qt><b>%1 secret key(s) imported.</b><br></qt>").arg(importedNbSec);
		if (dupNbSec!="0")
		resultMessage+=i18n("<qt>%1 secret key(s) unchanged.<br></qt>").arg(dupNbSec);
		if (notImportesNbSec!="0")
		resultMessage+=i18n("<qt>%1 secret key(s) not imported.<br></qt>").arg(notImportesNbSec);
		if (importedNbSucess!="0")
		resultMessage+=i18n("<qt><b>%1 key(s) imported:</b><br></qt>").arg(importedNbSucess);
        } else
                resultMessage=i18n("No key imported... \nCheck detailed log for more infos");
		
	QString lastID=QString("0x"+importedKeys.last().section(" ",0,0).right(8));
	if (!lastID.isEmpty()) 
	{
	kdDebug(2100)<<"++++++++++imported key"<<lastID<<endl;	
	emit importFinished(lastID);
	}
	
        KDetailedInfo *m_box=new KDetailedInfo(0,"import_result",resultMessage,readmessage,importedKeys);
        if (importpop)
                importpop->hide();
	m_box->setMinimumWidth(300);
        m_box->exec();
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
        
	//if (optionsServer.isEmpty())
	//	optionsServer="hkp://wwwkeys.pgp.net";
	page->kCBexportks->clear();
	page->kCBimportks->clear();
	
	page->kCBexportks->insertItem(optionsServer);
        page->kCBimportks->insertItem(optionsServer);
	
	page->kCBexportks->insertStringList(QStringList::split(",",serverList));
	page->kCBimportks->insertStringList(QStringList::split(",",serverList));
        
}

void keyServer::slotOk()
{
        accept();
}


#include "keyservers.moc"
