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

#include <qwhatsthis.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qvbox.h>


#include "kgpgoption.h"
#include "kgpgoptions.h"
#include "kgpg.h"

///////////////////////   main window

kgpgOptions::kgpgOptions(QWidget *parent, const char *name):KOptions( parent, name, true)
  {
//KSimpleConfig *config= new KSimpleConfig("kgpgtst");

config=kapp->config();
  config->setGroup("General Options");
  bool ascii=config->readBoolEntry("Ascii armor",true);
  bool untrusted=config->readBoolEntry("Allow untrusted keys",false);
  bool pgpcomp=config->readBoolEntry("PGP compatibility",false);
  bool encrypttodefault=config->readBoolEntry("encrypt to default key",false);
  QString defaultkey=config->readEntry("default key");
  bool encryptfileto=config->readBoolEntry("encrypt files to",false);
  QString filekey=config->readEntry("file key");
if (ascii==true) ascii_2_2->setChecked(true);
if (untrusted==true) untrusted_2_2->setChecked(true);
if (pgpcomp==true) pgp_2_2->setChecked(true);
if (encrypttodefault==true) defaut_2_2->setChecked(true);
if (encryptfileto==true) file_2_2->setChecked(true);
listkey();
if (filekey!=NULL)
filekey_2_2->setCurrentItem(filekey);
if (defaultkey!=NULL)
defautkey_2_2->setCurrentItem(namecode(defaultkey));
connect(buttonOk,SIGNAL(clicked()),this,SLOT(slotOk()));
}

void kgpgOptions::slotOk()
{
  config->setGroup("General Options");
  config->writeEntry("Ascii armor",ascii_2_2->isChecked());
  config->writeEntry("Allow untrusted keys",untrusted_2_2->isChecked());
  config->writeEntry("PGP compatibility",pgp_2_2->isChecked());
  config->writeEntry("encrypt to default key",defaut_2_2->isChecked());
  config->writeEntry("default key",idcode(defautkey_2_2->currentText()));
  config->writeEntry("encrypt files to",file_2_2->isChecked());
  config->writeEntry("file key",filekey_2_2->currentText());
  config->sync();
}

/*
QWhatsThis::add(selkey,i18n("<b>Special file key</b>: files will be encrypted only with this key"));
QWhatsThis::add(selkey2,i18n("<b>Default key</b>: all messages/files will also be encrypted with this key"));
QWhatsThis::add(choix3,i18n("<b>ASCII encryption</b>: makes it possible to open the encrypted file/message in a text editor"));
QWhatsThis::add(choix2,i18n("<b>Use special key for file encryption</b>: allows faster operations. When you encrypt a file " 
"from konqueror, kgpg will not ask you anything and will automatically use the special key."));
QWhatsThis::add(choix1,i18n("<b>Always encrypt to default key</b>: ensures that all messages/files are also "
"encrypted with your selected default key. It means that when you encrypt a message/file for someone, it will also be "
"encrypted with your default key, so you can always decrypt it for further use."));
QWhatsThis::add(choix4,i18n("<b>Allow encryption with untrusted keys</b>: when you import a public key, it is usually "
"marked as untrusted and you cannot use it unless you sign it in order to make it 'trusted'. Checking this "
"box enables you to use any key, even if it has not be signed."));
QWhatsThis::add(choix5,i18n("<b>PGP compatibility</b>: this option ensures that your messages can be decrypted by PGP 5.x"
" and higher."));
*/

QString kgpgOptions::namecode(QString kid)
{
if (kid!=NULL)
{
for ( int counter = 0; counter<names.count(); counter++ ) 
        if (QString("0x"+ids[counter].right(8))==kid) return names[counter];
}
else return QString("");
}


QString kgpgOptions::idcode(QString kname)
{
if (kname!=NULL)
{
for ( int counter = 0; counter<names.count(); counter++ ) 
        if (names[counter]==kname) return QString("0x"+ids[counter].right(8));
}
else return QString("");
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
	name=name.section('<',1,1);
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

//#include "kgpgoptions.moc"
