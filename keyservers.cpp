/***************************************************************************
                          keyserver.cpp  -  description
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
 
#include <kapplication.h>
#include <kcombobox.h>
#include <kdialogbase.h>
#include <kmessagebox.h>
 #include "keyservers.h"
  
keyServer::keyServer(QWidget *parent, const char *name):Keyserver( parent, name)
{
kLVservers->setFullWidth(true);

config=kapp->config();
config->setGroup("Keyservers");
QString servers=config->readEntry("servers");
if (servers.isEmpty()) servers="hkp://pgp.mit.edu,hkp://blackhole.pca.dfn.de";
while (!servers.isEmpty())
{
QString server1=servers.section(',',0,0);
server1=server1.stripWhiteSpace();
(void) new KListViewItem(kLVservers,server1);
servers.remove(0,server1.length()+1);
}

 KProcIO *encid=new KProcIO();
  *encid << "gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--with-colon"<<"--list-keys";
   QObject::connect(encid, SIGNAL(readReady(KProcIO *)),this, SLOT(slotprocread(KProcIO *)));
    encid->start(KProcess::DontCare,false);

syncCombobox();
kLEimportid->setFocus();
connect(Buttonadd,SIGNAL(clicked()),this,SLOT(slotAddServer()));
connect(Buttonedit,SIGNAL(clicked()),this,SLOT(slotEditServer()));
connect(Buttonremove,SIGNAL(clicked()),this,SLOT(slotRemoveServer()));
connect(Buttonimport,SIGNAL(clicked()),this,SLOT(slotImport()));
connect(Buttonsearch,SIGNAL(clicked()),this,SLOT(slotSearch()));
connect(Buttonexport,SIGNAL(clicked()),this,SLOT(slotExport()));
connect(buttonOk,SIGNAL(clicked()),this,SLOT(slotOk()));
connect(kLVservers,SIGNAL(doubleClicked(QListViewItem *)),this,SLOT(slotEdit(QListViewItem *)));
}


keyServer::~keyServer()
{
}

void keyServer::slotEdit(QListViewItem *)
{
slotEditServer();
}

void keyServer::slotprocread(KProcIO *p)
{
///////////////////////////////////////////////////////////////// extract  encryption keys
bool dead;
QString tst;

  while (p->readln(tst)!=-1)
  
       if (tst.startsWith("pub"))
        {
	dead=false;
          const QString trust=tst.section(':',1,1);
          QString val=tst.section(':',6,6);
	  QString id=QString("0x"+tst.section(':',4,4).right(8));
          if (val=="") val="Unlimited";
          QString tr;
          switch( trust[0] )
            {
            case 'o':
              tr= "Unknown" ;
              break;
            case 'i':
              tr= "Invalid";
	      dead=true;
              break;
            case 'd':
              tr="Disabled";
	      dead=true;
              break;
            case 'r':
              tr="Revoked";
	      dead=true;
              break;
            case 'e':
              tr="Expired";
	      dead=true;
              break;
            case 'q':
              tr="Undefined";
              break;
            case 'n':
              tr="None";
              break;
            case 'm':
              tr="Marginal";
	      break;
            case 'f':
              tr="Full";
              break;
            case 'u':
              tr="Ultimate";
              break;
            default:
              tr="?";
              break;
            }
tst=tst.section(':',9,9);

if ((dead==false) && (tst!=""))
	      kCBexportkey->insertItem(id+": "+tst);
}
}

void keyServer::slotSearch()
{
if (kCBimportks->currentText().isEmpty())
return;

if (kLEimportid->text().isEmpty())
{
KMessageBox::sorry(this,i18n("You must enter a search string"));
return;
}
listpop = new searchres( this,"result",true);
              /*QVBoxLayout *vbox1=new QVBoxLayout(listpop,3);
              QLabel *tex1=new QLabel(listpop);
              tex1->setText(i18n("<b>Found following keys:</b>"));
			kLVsearch=new KListView(listpop);
			kLVsearch->addColumn("ID");
			kLVsearch->addColumn(i18n("Keys"));
			kLVsearch->setSorting(-1);
			kLVsearch->setRootIsDecorated(true);
			kLVsearch->setAllColumnsShowFocus(true);
			kLVsearch->setFullWidth(true);
			QPushButton *Buttonimportsearch=new QPushButton(i18n("&Import"),listpop);
             vbox1->addWidget(tex1);
			  vbox1->addWidget(kLVsearch);
			  vbox1->addWidget(Buttonimportsearch);*/
              listpop->setMinimumWidth(250);
              listpop->adjustSize();
			  listpop->show();
connect(listpop->kLVsearch,SIGNAL(selectionChanged()),this,SLOT(transferKeyID()));
connect(listpop->buttonOk,SIGNAL(clicked()),this,SLOT(preimport()));
count=0;
cycle=false;
readmessage="";
searchproc=new KProcIO();
QString keyserv=kCBimportks->currentText();
      *searchproc<<"gpg"<<"--keyserver"<<keyserv<<"--command-fd=0"<<"--status-fd=2"<<"--search-keys"<<kLEimportid->text().stripWhiteSpace().local8Bit();
      QObject::connect(searchproc, SIGNAL(processExited(KProcess *)),this, SLOT(slotsearchresult(KProcess *)));
      QObject::connect(searchproc, SIGNAL(readReady(KProcIO *)),this, SLOT(slotsearchread(KProcIO *)));
      searchproc->start(KProcess::NotifyOnExit,true);

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
connect(Buttonabort,SIGNAL(clicked()),this,SLOT(abortSearch()));
}

void keyServer::abortSearch()
{
delete importpop;
delete searchproc;
delete listpop;
}

void keyServer::preimport()
{
if (listpop->kLEID->text().isEmpty())
{
KMessageBox::sorry(this,i18n("You must choose a key"));
return;
}
kLEimportid->setText(listpop->kLEID->text());
delete listpop;
slotImport();
}

void keyServer::transferKeyID()
{
if (listpop->kLVsearch->firstChild()==NULL) return;
QString kid;
if (listpop->kLVsearch->currentItem()->depth()==0)
kid=listpop->kLVsearch->currentItem()->firstChild()->text(0).stripWhiteSpace();
else kid=listpop->kLVsearch->currentItem()->text(0).stripWhiteSpace();
kid=kid.section("key",1,1);
kid=kid.stripWhiteSpace();
kid=kid.left(8);
listpop->kLEID->setText(kid);
}

void keyServer::slotsearchresult(KProcess *)
{
if (listpop->kLVsearch->firstChild()!=NULL) 
{
listpop->kLVsearch->setSelected(listpop->kLVsearch->firstChild(),true);
transferKeyID();
}
}

void keyServer::slotsearchread(KProcIO *p)
{
QString required;
while (p->readln(required,true)!=-1)
{
if (cycle)
{
QString kid=required.stripWhiteSpace();
(void) new KListViewItem(kitem,kid);
kid=kid.section("key",1,1);
kid=kid.stripWhiteSpace();
kid=kid.left(8);
//kitem->setText(0,kid);
}
cycle=false;
if (required.find("(")!=-1)
{
cycle=true;
kitem=new KListViewItem(listpop->kLVsearch,required.section(')',1,1).stripWhiteSpace());
count=0;
}
 if (required.find("keysearch.prompt")!=-1)
 {
 if (count<3) {p->writeStdin("N");count++;} 
 else 
 {
 delete importpop;
 p->writeStdin("Q");
 }
}
}

}


void keyServer::slotExport()
{
if (kCBexportks->currentText().isEmpty())
return;
readmessage="";
importproc=new KProcIO();
QString keyserv=kCBexportks->currentText();
      *importproc<<"gpg"<<"--keyserver"<<keyserv<<"--send-keys"<<kCBexportkey->currentText().section(':',0,0);
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


void keyServer::slotImport()
{
if (kCBimportks->currentText().isEmpty())
return;
if (kLEimportid->text().isEmpty())
{
KMessageBox::sorry(this,i18n("You must enter a search string"));
return;
}
readmessage="";
importproc=new KProcIO();
QString keyserv=kCBimportks->currentText();
	  *importproc<<"gpg"<<"--recv-keys"<<"--keyserver"<<keyserv<<kLEimportid->text().local8Bit();
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
delete importpop;
delete importproc;
}


void keyServer::slotimportresult(KProcess*)
{
delete importpop;
KMessageBox::information(this,readmessage);
}

void keyServer::slotimportread(KProcIO *p)
{
QString required;
while (p->readln(required,true)!=-1)
readmessage+=required+"\n";
}

void keyServer::syncCombobox()
{
kCBimportks->clear();
kCBexportks->clear();

QListViewItem *firstserver = kLVservers->firstChild();
  while (firstserver!=NULL)
  {
  kCBexportks->insertItem(firstserver->text(0));
  kCBimportks->insertItem(firstserver->text(0));  
	if (firstserver->nextSibling())
    firstserver = firstserver->nextSibling();
    else
      break;
}
}

void keyServer::slotEditServer()
{
KDialogBase *importpop = new KDialogBase(this, "urldialog", true, i18n("Edit a keyserver"),KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, true );
              QWidget *page = new QWidget(importpop);
			  QHBoxLayout *vbox=new QHBoxLayout(page,3);
              KLineEdit *lined=new KLineEdit(page);
			  vbox->addWidget(lined);
			  importpop->setMainWidget(page);
			  page->setMinimumSize(250,50);
			  lined->setFocus();
			  lined->setText(kLVservers->currentItem()->text(0));
			  page->show();
if ((importpop->exec()==QDialog::Accepted) && (!lined->text().stripWhiteSpace().isEmpty()))
{
kLVservers->currentItem()->setText(0,lined->text());
syncCombobox();
}
}


void keyServer::slotAddServer()
{
KDialogBase *importpop = new KDialogBase(this, "urldialog", true, i18n("Add a keyserver"),KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, true );
              QWidget *page = new QWidget(importpop);
			  QHBoxLayout *vbox=new QHBoxLayout(page,3);
              KComboBox *protocol=new KComboBox(page);
			  protocol->insertItem("hkp://");
			  protocol->insertItem("ldap://");
			  protocol->insertItem("mailto://");
              KLineEdit *lined=new KLineEdit(page);
              vbox->addWidget(protocol);
			  vbox->addWidget(lined);
			  importpop->setMainWidget(page);
			  page->setMinimumSize(300,50);
			  lined->setFocus();
			  page->show();
if ((importpop->exec()==QDialog::Accepted) && (!lined->text().stripWhiteSpace().isEmpty()))
{
(void) new KListViewItem(kLVservers,QString(protocol->currentText()+lined->text()));
syncCombobox();
}
}


void keyServer::slotRemoveServer()
{
if (kLVservers->currentItem()!=NULL)
{
kLVservers->takeItem(kLVservers->currentItem());
if (kLVservers->firstChild()!=NULL) kLVservers->firstChild()->setSelected(true);
syncCombobox();
}
}

void keyServer::slotOk()
{
QString serverslist;

QListViewItem *firstserver = kLVservers->firstChild();
  while (firstserver!=NULL)
  {
    serverslist+=firstserver->text(0);
	if (firstserver->nextSibling())
     {firstserver = firstserver->nextSibling();serverslist+=",";}
    else
      break;
}

config->setGroup("Keyservers");
config->writeEntry("servers",serverslist);
config->sync();
accept();
}
#include "keyservers.moc"
