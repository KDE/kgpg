/***************************************************************************
                          keygen.cpp  -  description
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

 ///////////////////////////////////////////////             code for new key generation

 
#include <qwhatsthis.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qvbox.h>

 #include "keygener.h"

///////////////////////   main window
keyGenerate::keyGenerate(QWidget *parent, const char *name):KDialogBase( parent, name, true,i18n("Key generation"),Apply | Ok | Cancel)
  {
expert=false;
setButtonApplyText(i18n("Expert mode"));

QWidget *page = new QWidget(this);
QVBoxLayout *vbox=new QVBoxLayout(page);

QVButtonGroup *bgroup1=new QVButtonGroup(i18n("Generate a key pair"),page);

(void) new QLabel(i18n("Name"),bgroup1);
kname=new KLineEdit("",bgroup1);
kname->setFocus();
(void) new QLabel(i18n("E-Mail"),bgroup1);
mail=new KLineEdit("",bgroup1);

(void) new QLabel(i18n("Comment (optional)"),bgroup1);
comment=new KLineEdit("",bgroup1);

(void) new QLabel(i18n("Expiration"),bgroup1);
  QHButtonGroup *bgroup=new  QHButtonGroup(bgroup1); 
numb=new KLineEdit("0",bgroup);
numb->setMaxLength(4);
numb->setDisabled(true);
keyexp = new KComboBox(bgroup);
keyexp->insertItem(i18n("Never"),0);
keyexp->insertItem(i18n("days"),1);
keyexp->insertItem(i18n("weeks"),2);
keyexp->insertItem(i18n("months"),3);
keyexp->insertItem(i18n("years"),4);
keyexp->setMinimumSize(keyexp->sizeHint());
connect(keyexp,SIGNAL(activated(int)),this,SLOT(activateexp(int)));

(void) new QLabel(i18n("Key size:"),bgroup1);
keysize = new KComboBox(bgroup1);
keysize->insertItem("768");
keysize->insertItem("1024");
keysize->insertItem("2048");
keysize->insertItem("4096");
keysize->setCurrentItem("1024");
keysize->setMinimumSize(keysize->sizeHint());

(void) new QLabel(i18n("Algorithm:"),bgroup1);
keykind = new KComboBox(bgroup1);
keykind->insertItem("DSA & ElGamal");
keykind->insertItem("RSA");
keykind->insertItem("ElGamal");
keykind->setMinimumSize(keykind->sizeHint());

vbox->addWidget(bgroup1);
page->show();
page->resize(page->maximumSize());
setMainWidget(page);
}

 void keyGenerate::slotOk()
 {
 if (QString(kname->text()).stripWhiteSpace()=="") 
 {
 KMessageBox::sorry(0,i18n("You must give a name"));
 return;
 }
 QString vmail=mail->text();
 if ((vmail.find(" ")!=-1) || (vmail.find(".")==-1) || (vmail.find("@")==-1))
 {
KMessageBox::sorry(0,i18n("E-Mail adress not valid"));
 return; 
 } 
 accept();
 }
 
  void keyGenerate::slotApply()
 {
 expert=true;
 accept();
 }
 
void keyGenerate::activateexp(int state)
{
if (state==0) numb->setDisabled(true);
else numb->setDisabled(false);
}

bool keyGenerate::getmode()
{
return(expert);
}


QString keyGenerate::getkeytype()
{
return(keykind->currentText());
}

QString keyGenerate::getkeysize()
{
return(keysize->currentText());
}

int keyGenerate::getkeyexp()
{
return(keyexp->currentItem());
}

QString keyGenerate::getkeynumb()
{
if (numb->text()!=NULL)
return(numb->text());
else return ("");
}

QString keyGenerate::getkeyname()
{
if (kname->text()!=NULL)
return(kname->text());
else return ("");
}

QString keyGenerate::getkeymail()
{
if (mail->text()!=NULL)
return(mail->text());
else return ("");
}

QString keyGenerate::getkeycomm()
{
if (comment->text()!=NULL)
return(comment->text());
else return ("");
}

#include "keygener.moc"
