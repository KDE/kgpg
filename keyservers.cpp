/***************************************************************************
                          keyservers.cpp  -  description
                             -------------------
    begin                : Tue Nov 26 2002
    copyright            : (C) 2002 by y0k0
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

#include <kapplication.h>
#include <kcombobox.h>
#include <kdialogbase.h>
#include <kmessagebox.h>
#include <qtextcodec.h>
#include "keyservers.h"
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
#include <qcheckbox.h>
#include <kconfig.h>
#include <klineedit.h>

keyServer::keyServer(QWidget *parent, const char *name,bool modal,WFlags f):Keyserver( parent, name,modal,f)
{
        config=kapp->config();

        syncCombobox();
        kLEimportid->setFocus();


        connect(Buttonimport,SIGNAL(clicked()),this,SLOT(slotImport()));
        connect(Buttonsearch,SIGNAL(clicked()),this,SLOT(slotSearch()));
        connect(Buttonexport,SIGNAL(clicked()),this,SLOT(slotExport()));
        connect(buttonOk,SIGNAL(clicked()),this,SLOT(slotOk()));

        connect(cBproxyI,SIGNAL(toggled(bool)),this,SLOT(slotEnableProxyI(bool)));
        connect(cBproxyE,SIGNAL(toggled(bool)),this,SLOT(slotEnableProxyE(bool)));

        const char *httpproxy = getenv("http_proxy");
        if (httpproxy) {
                cBproxyI->setEnabled(true);
                cBproxyE->setEnabled(true);
                cBproxyI->setChecked(true);
                cBproxyE->setChecked(true);
                kLEproxyI->setText(httpproxy);
                kLEproxyE->setText(httpproxy);
        }


        KProcIO *encid=new KProcIO();
        *encid << "gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--with-colon"<<"--list-keys";
        QObject::connect(encid, SIGNAL(readReady(KProcIO *)),this, SLOT(slotprocread(KProcIO *)));
        encid->start(KProcess::NotifyOnExit,true);
}


keyServer::~keyServer()
{}

void keyServer::slotEnableProxyI(bool on)
{
        kLEproxyI->setEnabled(on);
}

void keyServer::slotEnableProxyE(bool on)
{
        kLEproxyE->setEnabled(on);
}



void keyServer::slotprocread(KProcIO *p)
{
        ///////////////////////////////////////////////////////////////// extract  encryption keys
        bool dead;
        QString tst;

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
                                kCBexportkey->insertItem(id+": "+tst);
                }
        }
}

void keyServer::slotSearch()
{
        if (kCBimportks->currentText().isEmpty())
                return;

        if (kLEimportid->text().isEmpty()) {
                KMessageBox::sorry(this,i18n("You must enter a search string."));
                return;
        }

        listpop = new KeyServer( this,"result",WType_Dialog | WShowModal);
        //KStatusBar *searchStatusBar=new KStatusBar(listpop);
        listpop->setMinimumWidth(250);
        listpop->adjustSize();
        sBar=listpop->statusBar();
        sBar->setSizeGripEnabled(false);
        statusmsg = new QLabel(sBar);
        sBar->addWidget(statusmsg,3,true);
        statusmsg->setText(i18n("Connecting to the server..."));
        //sBar->message(i18n("Connecting to the server..."));
        sBar->show();

        listpop->show();
        connect(listpop->kLVsearch,SIGNAL(selectionChanged()),this,SLOT(transferKeyID()));
        connect(listpop->buttonOk,SIGNAL(clicked()),this,SLOT(preimport()));
        connect(listpop->kLVsearch,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),this,SLOT(preimport()));
        connect(listpop->kLVsearch,SIGNAL(returnPressed ( QListViewItem * )),this,SLOT(preimport()));

        connect(listpop->buttonCancel,SIGNAL(clicked()),this,SLOT(handleQuit()));
        connect( listpop , SIGNAL( destroyed() ) , this, SLOT( abortSearch()));
        count=0;
        cycle=false;
        readmessage="";
        KProcIO *searchproc=new KProcIO();
        QString keyserv=kCBimportks->currentText();
        *searchproc<<"gpg";
        if (cBproxyI->isChecked()) {
                searchproc->setEnvironment("http_proxy",kLEproxyI->text());
                *searchproc<<	"--keyserver-options"<<"honor-http-proxy";
        } else
                *searchproc<<	"--keyserver-options"<<"no-honor-http-proxy";
        *searchproc<<"--keyserver"<<keyserv<<"--command-fd=0"<<"--status-fd=2"<<"--search-keys"<<kLEimportid->text().stripWhiteSpace().local8Bit();

        keyNumbers=0;
        QObject::connect(searchproc, SIGNAL(processExited(KProcess *)),this, SLOT(slotsearchresult(KProcess *)));
        QObject::connect(searchproc, SIGNAL(readReady(KProcIO *)),this, SLOT(slotsearchread(KProcIO *)));
        searchproc->start(KProcess::NotifyOnExit,true);
}

void keyServer::handleQuit()
{
        listpop->close();
}


void keyServer::abortSearch()
{
        if (listpop) {
                delete listpop;
                listpop=0L;
        }
}

void keyServer::preimport()
{
        transferKeyID();
        if (listpop->kLEID->text().isEmpty()) {
                KMessageBox::sorry(this,i18n("You must choose a key."));
                return;
        }
        kLEimportid->setText(listpop->kLEID->text());
        listpop->close();
        slotImport();
}

void keyServer::transferKeyID()
{
        if (listpop->kLVsearch->firstChild()==NULL)
                return;
        QString kid;
        if (listpop->kLVsearch->currentItem()->depth()==0)
                kid=listpop->kLVsearch->currentItem()->firstChild()->text(0).stripWhiteSpace();
        else
                kid=listpop->kLVsearch->currentItem()->text(0).stripWhiteSpace();
        kid=kid.section("key",1,1);
        kid=kid.stripWhiteSpace();
        kid=kid.left(8);
        listpop->kLEID->setText(kid);
}

void keyServer::slotsearchresult(KProcess *)
{
        QString nb;
        nb=nb.setNum(keyNumbers);
        //listpop->kLVsearch->setColumnText(0,i18n("Found %1 matching keys").arg(nb));
        statusmsg->setText(i18n("Found %1 matching keys").arg(nb));

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
                        required="";
                }

                if (required.find("GOT_IT")!=-1) {
                        count++;
                        required="";
                }

                if ((cycle) && (!required.isEmpty())) {
                        QString kid=required.stripWhiteSpace();
                        (void) new KListViewItem(kitem,kid);
                        kid=kid.section("key",1,1);
                        kid=kid.stripWhiteSpace();
                        kid=kid.left(8);
                        required="";
                }

                cycle=false;

                if ((required.find("(")!=-1) && (!required.isEmpty())) {
                        cycle=true;
                        kitem=new KListViewItem(listpop->kLVsearch,required.remove(0,required.find(")")+1).stripWhiteSpace());
                        keyNumbers++;
                        count=0;
                        required="";
                }
        }
}


void keyServer::slotExport()
{
        if (kCBexportks->currentText().isEmpty())
                return;
        readmessage="";
        exportproc=new KProcIO();
        QString keyserv=kCBexportks->currentText();

        *exportproc<<"gpg";
        if (cBproxyE->isChecked()) {
                exportproc->setEnvironment("http_proxy",kLEproxyE->text());
                *exportproc<<	"--keyserver-options"<<"honor-http-proxy";
        } else
                *exportproc<<	"--keyserver-options"<<"no-honor-http-proxy";
        *exportproc<<"--status-fd=2"<<"--keyserver"<<keyserv<<"--send-keys"<<kCBexportkey->currentText().section(':',0,0);

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
        connect(Buttonabort,SIGNAL(clicked()),this,SLOT(abortExport()));
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
        if (kCBimportks->currentText().isEmpty())
                return;
        if (kLEimportid->text().isEmpty()) {
                KMessageBox::sorry(this,i18n("You must enter a search string."));
                return;
        }
        readmessage="";
        importproc=new KProcIO();
        QString keyserv=kCBimportks->currentText();

        *importproc<<"gpg";
        if (cBproxyI->isChecked()) {
                importproc->setEnvironment("http_proxy",kLEproxyI->text());
                *importproc<<	"--keyserver-options"<<"honor-http-proxy";
        } else
                *importproc<<	"--keyserver-options"<<"no-honor-http-proxy";

        *importproc<<"--status-fd=2"<<"--keyserver"<<keyserv<<"--recv-keys";
        QString keyNames=kLEimportid->text();
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
}

void keyServer::slotimportresult(KProcess*)
{
        QString importedNb,importedNbSucess,importedNbProcess,resultMessage,importedKeys, parsedOutput;
        parsedOutput=readmessage;

        while (parsedOutput.find("IMPORTED")!=-1) {
                parsedOutput.remove(0,parsedOutput.find("IMPORTED")+8);
                importedKeys+=parsedOutput.section("\n",0,0).stripWhiteSpace().replace(QRegExp("<"),"&lt;")+"<br>";
        }

        if (readmessage.find("IMPORT_RES")!=-1) {
                importedNb=readmessage.section("IMPORT_RES",-1,-1);
                importedNb=importedNb.stripWhiteSpace();
                importedNbProcess=importedNb.section(" ",0,0);
                importedNbSucess=importedNb.section(" ",2,2);
                resultMessage=i18n("<qt>%1 key(s) processed. %2 key(s) imported.<br><b>%3</b></qt>").arg(importedNbProcess).arg(importedNbSucess).arg(importedKeys.left(600));
        } else
                resultMessage=i18n("No key imported... \nCheck detailed log for more infos");
        KDetailedInfo *m_box=new KDetailedInfo(0,"import_result",resultMessage,readmessage);
        if (importpop)
                importpop->hide();
        m_box->exec();
        if (importpop)
                delete importpop;
}

void keyServer::slotimportread(KProcIO *p)
{
        QString required;
        while (p->readln(required,true)!=-1)
                readmessage+=required+"\n";
}

void keyServer::syncCombobox()
{
QString servers;
        config->setGroup("GPG Settings");
        QString confPath=config->readPathEntry("gpg_config_path");

        QString optionsServer=KgpgInterface::getGpgSetting("keyserver",confPath);
	        if (optionsServer.isEmpty())
		optionsServer="hkp://[wwwkeys.pgp.net,wwwkeys.eu.pgp.net,wwwkeys.us.pgp.net]";
		kCBexportks->insertItem(optionsServer);
                kCBimportks->insertItem(optionsServer);
	servers=config->readEntry("key_server2","hkp://wwwkeys.eu.pgp.net");
        if (!servers.isEmpty()) {
                kCBexportks->insertItem(servers);
                kCBimportks->insertItem(servers);
        }
	servers=config->readEntry("key_server3","hkp://wwwkeys.us.pgp.net");
        if (!servers.isEmpty()) {
                kCBexportks->insertItem(servers);
                kCBimportks->insertItem(servers);
        }
}

void keyServer::slotOk()
{
        accept();
}


#include "keyservers.moc"
