/***************************************************************************
                          kgpgoptions.cpp  -  description
                             -------------------
    begin                : Mon Jul 8 2002
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

 ///////////////////////////////////////////////             code for the option dialog box

#include <klineedit.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qvbox.h>
#include <qfile.h>

#include <kdesktopfile.h>
#include <kmimetype.h>
#include <kstandarddirs.h>

#include "kgpgoptions.h"
#include "kgpg.h"


///////////////////////   main window

kgpgOptions::kgpgOptions(QWidget *parent, const char *name):KgpgOptionDialog( parent, name)
  {
config=kapp->config();

  config->setGroup("General Options");
  bool ascii=config->readBoolEntry("Ascii armor",true);
  bool untrusted=config->readBoolEntry("Allow untrusted keys",false);
  bool hideid=config->readBoolEntry("Hide user ID",false);
  bool pgpcomp=config->readBoolEntry("PGP compatibility",false);
  bool encryptfileto=config->readBoolEntry("encrypt files to",false);
  bool displaymailfirst=config->readBoolEntry("display mail first",true);
  bool clipselection=config->readBoolEntry("selection clip",false);
  QString filekey=config->readEntry("file key");
  bool allowcustom=config->readBoolEntry("allow custom option",false);
  confPath=config->readEntry("gpg config path");
  if (confPath.isEmpty()) confPath=QDir::homeDirPath()+"/.gnupg/options";
defaultKeyID=KgpgInterface::getGpgSetting("encrypt-to",confPath);
if (!defaultKeyID.isEmpty()) defaut_2_2->setChecked(true);

  kURLconfigPath->setURL(confPath);
  kLEcustom->setText(config->readEntry("custom option"));
  kLEcustomdec->setText(config->readEntry("custom decrypt"));

reloadServer();

config->setGroup("Notification Messages");
if (config->readBoolEntry("RemoteFileWarning",true)) cbTempWarning->setChecked(true);

config->setGroup("Applet");
int ufileDropEvent=config->readNumEntry("unencrypted drop event",0);
int efileDropEvent=config->readNumEntry("encrypted drop event",2);

  config->setGroup("Service Menus");
QString smenu;
smenu=config->readEntry("Decrypt");
if (smenu!=NULL) kCBdecrypt->setCurrentItem(smenu);
smenu=config->readEntry("Sign");
if (smenu!=NULL) kCBsign->setCurrentItem(smenu);

ascii_2_2->setChecked(ascii);
untrusted_2_2->setChecked(untrusted);
hide_2_2->setChecked(hideid);
pgp_2_2->setChecked(pgpcomp);

file_2_2->setChecked(encryptfileto);
custom_2_2->setChecked(allowcustom);
cbMailFirst->setChecked(displaymailfirst);
cbClipSelection->setChecked(clipselection);

kCBencrypted->setCurrentItem(efileDropEvent);
kCBunencrypted->setCurrentItem(ufileDropEvent);


listkey();
if (filekey!=NULL)
kCBfilekey->setCurrentItem(filekey);
connect(buttonOk,SIGNAL(clicked()),this,SLOT(slotOk()));
connect(bcheckMime,SIGNAL(clicked()),this,SLOT(checkMimes()));
connect(Buttonadd,SIGNAL(clicked()),this,SLOT(slotAddServer()));
connect(Buttondefault,SIGNAL(clicked()),this,SLOT(slotDefaultServer()));
connect(Buttonedit,SIGNAL(clicked()),this,SLOT(slotEditServer()));
connect(Buttonremove,SIGNAL(clicked()),this,SLOT(slotRemoveServer()));
}


kgpgOptions::~kgpgOptions()
{
}


void kgpgOptions::slotEditServer()
{
KDialogBase *serverEdit = new KDialogBase(this, "urldialog", true, i18n("Edit Keyserver"),KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, true );
              QWidget *page = new QWidget(serverEdit);
			  QHBoxLayout *vbox=new QHBoxLayout(page,3);
              KLineEdit *lined=new KLineEdit(page);
			  vbox->addWidget(lined);
			  serverEdit->setMainWidget(page);
			  page->setMinimumSize(250,50);
			  lined->setFocus();
			  lined->setText(kLVservers->currentItem()->text(0));
			  page->show();
if ((serverEdit->exec()==QDialog::Accepted) && (!lined->text().stripWhiteSpace().isEmpty()))
{
kLVservers->currentItem()->setText(0,lined->text());
}
}


void kgpgOptions::slotAddServer()
{
KDialogBase *serverAdd = new KDialogBase(this, "urldialog", true, i18n("Add Keyserver"),KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, true );
              QWidget *page = new QWidget(serverAdd);
			  QHBoxLayout *vbox=new QHBoxLayout(page,3);
              KComboBox *protocol=new KComboBox(page);
			  protocol->insertItem("hkp://");
			  protocol->insertItem("ldap://");
			  protocol->insertItem("wwwkeys.");
              KLineEdit *lined=new KLineEdit(page);
              vbox->addWidget(protocol);
			  vbox->addWidget(lined);
			  serverAdd->setMainWidget(page);
			  page->setMinimumSize(300,50);
			  lined->setFocus();
			  page->show();
if ((serverAdd->exec()==QDialog::Accepted) && (!lined->text().stripWhiteSpace().isEmpty()))
{
(void) new KListViewItem(kLVservers,QString(protocol->currentText()+lined->text()));
}
}


void kgpgOptions::slotRemoveServer()
{
if (kLVservers->currentItem()!=NULL)
{
kLVservers->takeItem(kLVservers->currentItem());
if (kLVservers->firstChild()!=NULL) kLVservers->firstChild()->setSelected(true);
}
}

void kgpgOptions::slotDefaultServer()
{
if (kLVservers->currentItem()!=NULL)
{
KgpgInterface::setGpgSetting("keyserver",kLVservers->currentItem()->text(0).stripWhiteSpace().section(' ',0,0),confPath);
kLVservers->clear();
reloadServer();
}
}


void kgpgOptions::reloadServer()
{
config->setGroup("Keyservers");
QString servers=config->readEntry("servers");
if (servers.isEmpty()) servers="hkp://pgp.mit.edu,hkp://blackhole.pca.dfn.de";
QString optionsServer=KgpgInterface::getGpgSetting("keyserver",confPath);
if (!optionsServer.isEmpty() && (servers.find(optionsServer)==-1))
servers+=","+optionsServer;
while (!servers.isEmpty())
{
QString server1=servers.section(',',0,0);
servers.remove(0,server1.length()+1);
server1=server1.stripWhiteSpace();
if (!server1.isEmpty())
{
if (server1==optionsServer) server1+=i18n(" [default]");
(void) new KListViewItem(kLVservers,server1);
}
}
}

void kgpgOptions::checkMimes()
{
	KStandardDirs *sd=new KStandardDirs();
	QString baseDir=sd->findResource("mime","application/pgp-encrypted.desktop");
	QString localDir=locateLocal("mime","application/pgp-encrypted.desktop");

KDesktopFile configb(baseDir, true, "mime");
QString pats=configb.readEntry("Patterns");
if ((pats.find("gpg")==-1) || (pats.find("asc")==-1) || (pats.find("pgp")==-1))
{
KDesktopFile configl(localDir, false, "mime");
    configl.writeEntry("Type", "MimeType");
    configl.writeEntry("MimeType", "application/pgp-encrypted");
    configl.writeEntry("Hidden", false);
	configl.writeEntry("Patterns","*.pgp;*.gpg;*.asc");
	}

 baseDir=sd->findResource("mime","application/pgp-signature.desktop");
 localDir=locateLocal("mime","application/pgp-signature.desktop");

KDesktopFile configb2(baseDir, true, "mime");
 pats=configb2.readEntry("Patterns");
if (pats.find("sig")==-1)
{
KDesktopFile configl2(localDir, false, "mime");
    configl2.writeEntry("Type", "MimeType");
    configl2.writeEntry("MimeType", "application/pgp-signature");
    configl2.writeEntry("Hidden", false);
	configl2.writeEntry("Patterns","*.sig");
	}
}


void kgpgOptions::slotInstallSign(QString mimetype)
{
QString path=locateLocal("data","konqueror/servicemenus/signfile.desktop");
  KDesktopFile configl2(path, false);
  if (configl2.isImmutable() ==false)
{
  configl2.setGroup("Desktop Entry");
    configl2.writeEntry("ServiceTypes", mimetype);
    configl2.writeEntry("Actions", "sign");
    configl2.setGroup("Desktop Action sign");
	configl2.writeEntry("Name",i18n("Sign File"));
	//configl2.writeEntry("Icon", "sign_file");
	configl2.writeEntry("Exec","kgpg -S %U");
  	  //KMessageBox::information(this,i18n("Decrypt file option is now added in Konqueror's menu."));
  }
}

void kgpgOptions::slotInstallDecrypt(QString mimetype)
{
QString path=locateLocal("data","konqueror/servicemenus/decryptfile.desktop");
  KDesktopFile configl2(path, false);
  if (configl2.isImmutable() ==false)
{
  configl2.setGroup("Desktop Entry");
    configl2.writeEntry("ServiceTypes", mimetype);
    configl2.writeEntry("Actions", "decrypt");
    configl2.setGroup("Desktop Action decrypt");
	configl2.writeEntry("Name",i18n("Decrypt File"));
	//configl2.writeEntry("Icon", "decrypt_file");
	configl2.writeEntry("Exec","kgpg -d %U");
  	  //KMessageBox::information(this,i18n("Decrypt file option is now added in Konqueror's menu."));
  }

}


void kgpgOptions::slotRemoveMenu(QString menu)
{
    QString path=locateLocal("data","konqueror/servicemenus/"+menu);
    QFile qfile(path);
    if (qfile.exists()) qfile.remove();
    {
//if (!qfile.remove()) KMessageBox::sorry(this,i18n("Cannot remove service menu. Check permissions"));
//else KMessageBox::information(this,i18n("Service menu Decrypt file has been removed"));
    }
//else KMessageBox::sorry(this,i18n("No service menu found"));
}

void kgpgOptions::slotOk()
{
  config->setGroup("General Options");
  config->writeEntry("Ascii armor",ascii_2_2->isChecked());
  config->writeEntry("Allow untrusted keys",untrusted_2_2->isChecked());
  config->writeEntry("Hide user ID",hide_2_2->isChecked());
  config->writeEntry("PGP compatibility",pgp_2_2->isChecked());
  config->writeEntry("encrypt files to",file_2_2->isChecked());
  config->writeEntry("file key",kCBfilekey->currentText());

  config->writeEntry("display mail first",cbMailFirst->isChecked());
  config->writeEntry("selection clip",cbClipSelection->isChecked());
  config->writeEntry("custom option",kLEcustom->text());
  config->writeEntry("allow custom option",custom_2_2->isChecked());
  config->writeEntry("custom decrypt",kLEcustomdec->text());
  config->writeEntry("gpg config path",kURLconfigPath->url());

  if (defaut_2_2->isChecked())
{
  config->writeEntry("default key","0x"+kCBalwayskey->currentText().section(':',0,0));
  KgpgInterface::setGpgSetting("encrypt-to",kCBalwayskey->currentText().section(':',0,0),confPath);
  }
  else
  {
  config->writeEntry("default key","");
  KgpgInterface::setGpgSetting("encrypt-to","",confPath);
}

  config->setGroup("Service Menus");
  config->writeEntry("Decrypt",kCBdecrypt->currentText());
  config->writeEntry("Sign",kCBsign->currentText());

config->setGroup("Applet");
config->writeEntry("encrypted drop event",kCBencrypted->currentItem());
config->writeEntry("unencrypted drop event",kCBunencrypted->currentItem());


config->setGroup("Notification Messages");
config->writeEntry("RemoteFileWarning",cbTempWarning->isChecked());
//else KMessageBox::enableAllMessages();

QString serverslist;

QListViewItem *firstserver = kLVservers->firstChild();
  while (firstserver!=NULL)
  {
    serverslist+=firstserver->text(0).stripWhiteSpace().section(' ',0,0);
	if (firstserver->nextSibling())
     {firstserver = firstserver->nextSibling();serverslist+=",";}
    else
      break;
}

config->setGroup("Keyservers");
config->writeEntry("servers",serverslist);


  config->sync();

  if (kCBsign->currentText()==i18n("All Files")) slotInstallSign("allfiles");
  else slotRemoveMenu("signfile.desktop");

  if (kCBdecrypt->currentText()==i18n("All Files")) slotInstallDecrypt("allfiles");
  else if (kCBdecrypt->currentText().startsWith(i18n("Encrypted"))) slotInstallDecrypt("application/pgp-encrypted");
  else slotRemoveMenu("decryptfile.desktop");
}

QString kgpgOptions::namecode(QString kid)
{
    for ( uint counter = 0; counter<names.count(); counter++ )
        if (QString("0x"+ids[counter].right(8))==kid)
            return names[counter];

    return QString::null;
}


QString kgpgOptions::idcode(QString kname)
{
    for ( uint counter = 0; counter<names.count(); counter++ )
        if (names[counter]==kname)
            return QString("0x"+ids[counter].right(8));
    return QString::null;
}

void kgpgOptions::listkey()
{
  ////////   update display of keys in main management window
  FILE *fp;
  QString tst,name,trustedvals="idre-",defaultKeyName;
  int counter=0;
  char line[130];

  fp = popen("gpg --no-tty --with-colon --list-keys", "r");
  while ( fgets( line, sizeof(line), fp))
  {
    tst=line;
    if (tst.find("pub",0,FALSE)!=-1)
    {
      name=tst.section(':',9,9);
      if ((name!="") && (trustedvals.find(tst.section(':',1,1))==-1))
      {
        counter++;
	//name=name.section('<',-1,-1);
      //  name=name.section('>',0,0);
        names+=name;
	ids+=tst.section(':',4,4);
	if (tst.section(':',4,4).right(8)==defaultKeyID)
	defaultKeyName=tst.section(':',4,4).right(8)+":"+name;
	kCBfilekey->insertItem(tst.section(':',4,4).right(8)+":"+name);
	kCBalwayskey->insertItem(tst.section(':',4,4).right(8)+":"+name);
      }
    }
  }
  if (!defaultKeyName.isEmpty()) kCBalwayskey->setCurrentItem(defaultKeyName);
  pclose(fp);
  if (counter==0)
  {
	ids+="0";
	kCBfilekey->insertItem(i18n("none"));
	kCBalwayskey->insertItem(i18n("none"));
  }
  }

#include "kgpgoptions.moc"
