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

#include "kgpgoptions.h"
#include "kgpg.h"

///////////////////////   main window
kgpgOptions::kgpgOptions(QWidget *parent, const char *name,bool oascii,bool otrusted,bool opgp,bool ofilekey,bool odefkey,
QString ofkey,QString odkey):
KDialogBase( parent, name, true,i18n("Options"),Ok | Cancel)
  {
    
QWidget *page = new QWidget(this);

QVBoxLayout *vbox=new QVBoxLayout(page);

QVButtonGroup *bgroup1=new QVButtonGroup(i18n("Encryption defaults"),page);

choix3 = new QCheckBox(i18n("ASCII Armored encryption"),bgroup1);
choix4 = new QCheckBox(i18n("Allow encryption with untrusted keys"),bgroup1);
 choix5 = new QCheckBox(i18n("PGP compatibility"),bgroup1);
choix2 = new QCheckBox(i18n("Use special key for file encryption:"),bgroup1);

selkey = new KComboBox(bgroup1);
selkey->setMinimumSize(selkey->sizeHint());
selkey->setDisabled(true);

choix1 = new QCheckBox(i18n("Always encrypt with default key:"),bgroup1);
selkey2 = new KComboBox(bgroup1);
listkey();
selkey2->setMinimumSize(selkey->sizeHint());
selkey2->setDisabled(true);

connect(choix2,SIGNAL(stateChanged(int)),this,SLOT(activateselkey(int)));
connect(choix1,SIGNAL(stateChanged(int)),this,SLOT(activateselkey2(int)));

vbox->addWidget(bgroup1);

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


if (oascii==true) choix3->setChecked(true);
if (otrusted==true) choix4->setChecked(true);
if (opgp==true) choix5->setChecked(true);

selkey->setCurrentItem(ofkey);
if (odkey!="")
selkey2->setCurrentItem(namecode(odkey));

if (odefkey==true) choix1->setChecked(true);
if (ofilekey==true) choix2->setChecked(true);



page->show();
page->resize(page->maximumSize());
setMainWidget(page);
}

void kgpgOptions::activateselkey(int state)
{
if (state==2) selkey->setDisabled(false);
else selkey->setDisabled(true);
}

void kgpgOptions::activateselkey2(int state)
{
if (state==2) selkey2->setDisabled(false);
else selkey2->setDisabled(true);
}

bool kgpgOptions::getascii()
{
return(choix3->isChecked());
}

bool kgpgOptions::getuntrusted()
{
return(choix4->isChecked());
}

bool kgpgOptions::getpgp()
{
return(choix5->isChecked());
}

bool kgpgOptions::fileenc()
{
return(choix2->isChecked());
}

bool kgpgOptions::defaultenc()
{
return(choix1->isChecked());
}

QString kgpgOptions::getfilekey()
{
return(selkey->currentText());
}


QString kgpgOptions::getdefkey()
{
return(idcode(selkey2->currentText()));
}

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
	selkey->insertItem(name);
	selkey2->insertItem(name);
      }
    }
  }
  pclose(fp);
  if (counter==0)
  {
	ids+="0";
	selkey->insertItem("none");
	selkey2->insertItem("none"); 
  }
  }

