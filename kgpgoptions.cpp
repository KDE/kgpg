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

#include "kgpgoption.h"
#include "kgpgoptions.h"
#include "kgpg.h"


///////////////////////   main window

kgpgOptions::kgpgOptions(QWidget *parent, const char *name):KOptions( parent, name)
  {


config=kapp->config();
  config->setGroup("General Options");
  bool ascii=config->readBoolEntry("Ascii armor",true);
  bool untrusted=config->readBoolEntry("Allow untrusted keys",false);
  bool hideid=config->readBoolEntry("Hide user ID",false);
  bool pgpcomp=config->readBoolEntry("PGP compatibility",false);
  bool encrypttodefault=config->readBoolEntry("encrypt to default key",false);
  QString defaultkey=config->readEntry("default key");
  bool encryptfileto=config->readBoolEntry("encrypt files to",false);
  bool displaymailfirst=config->readBoolEntry("display mail first",true);
QString filekey=config->readEntry("file key");
config->setGroup("Service Menus");
QString smenu;
smenu=config->readEntry("Decrypt");
if (smenu!=NULL) kCBdecrypt->setCurrentItem(smenu);
smenu=config->readEntry("Sign");
if (smenu!=NULL) kCBsign->setCurrentItem(smenu);

if (ascii) ascii_2_2->setChecked(true);
if (untrusted) untrusted_2_2->setChecked(true);
if (hideid) hide_2_2->setChecked(true);
if (pgpcomp) pgp_2_2->setChecked(true);
if (encrypttodefault) defaut_2_2->setChecked(true);
if (encryptfileto) file_2_2->setChecked(true);
if (displaymailfirst) cbMailFirst->setChecked(true);

listkey();
if (filekey!=NULL)
filekey_2_2->setCurrentItem(filekey);
if (defaultkey!=NULL)
defautkey_2_2->setCurrentItem(namecode(defaultkey));
connect(buttonOk,SIGNAL(clicked()),this,SLOT(slotOk()));
connect(bcheckMime,SIGNAL(clicked()),this,SLOT(checkMimes()));
}


kgpgOptions::~kgpgOptions()
{
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
	configl2.writeEntry("Exec","kgpg -S %u");
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
	configl2.writeEntry("Exec","kgpg -d %u");
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
  config->writeEntry("encrypt to default key",defaut_2_2->isChecked());
  config->writeEntry("default key",idcode(defautkey_2_2->currentText()));
  config->writeEntry("encrypt files to",file_2_2->isChecked());
  config->writeEntry("file key",filekey_2_2->currentText());
  config->writeEntry("display mail first",cbMailFirst->isChecked());
  config->setGroup("Service Menus");
  config->writeEntry("Decrypt",kCBdecrypt->currentText());
  config->writeEntry("Sign",kCBsign->currentText());
  config->sync();

  if (kCBsign->currentText()==i18n("All files")) slotInstallSign("all/allfiles");
  else slotRemoveMenu("signfile.desktop");

  if (kCBdecrypt->currentText()==i18n("All files")) slotInstallDecrypt("all/allfiles");
  else if (kCBdecrypt->currentText().startsWith(i18n("Encrypted"))) slotInstallDecrypt("all/allfiles");
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
  QString tst,name,trustedvals="idre-";
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
	name=name.section('<',-1,-1);
        name=name.section('>',0,0);
        names+=name;
	ids+=tst.section(':',4,4);
	filekey_2_2->insertItem(name);
	defautkey_2_2->insertItem(name);
      }
    }
  }
  pclose(fp);
  if (counter==0)
  {
	ids+="0";
	filekey_2_2->insertItem("none");
	defautkey_2_2->insertItem("none");
  }
  }

#include "kgpgoptions.moc"
