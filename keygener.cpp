/***************************************************************************
                          keygen.cpp  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright            : (C) 2002 by Jean-Baptiste Mardelle
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
#include <kcombobox.h>
#include <klineedit.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>
#include <qhbuttongroup.h>
#include <qvbuttongroup.h>
#include <kmessagebox.h>
#include <klocale.h>

#include "keygener.h"

///////////////////////   main window
keyGenerate::keyGenerate(QWidget *parent, const char *name):KDialogBase( parent, name, true,i18n("Key Generation"),Apply | Ok | Cancel)
{
        expert=false;
        setButtonApply(i18n("Expert Mode"));

        QWidget *page = new QWidget(this);
        QVBoxLayout *vbox=new QVBoxLayout(page);

        QVButtonGroup *bgroup1=new QVButtonGroup(i18n("Generate Key Pair"),page);

        (void) new QLabel(i18n("Name:"),bgroup1);
        kname=new KLineEdit("",bgroup1);
        kname->setFocus();
        (void) new QLabel(i18n("Email:"),bgroup1);
        mail=new KLineEdit("",bgroup1);

        (void) new QLabel(i18n("Comment (optional):"),bgroup1);
        comment=new KLineEdit("",bgroup1);

        (void) new QLabel(i18n("Expiration:"),bgroup1);
        QHButtonGroup *bgroup=new  QHButtonGroup(bgroup1);
        numb=new KLineEdit("0",bgroup);
        numb->setMaxLength(4);
        numb->setDisabled(true);
        keyexp = new KComboBox(bgroup);
        keyexp->insertItem(i18n("Never"),0);
        keyexp->insertItem(i18n("Days"),1);
        keyexp->insertItem(i18n("Weeks"),2);
        keyexp->insertItem(i18n("Months"),3);
        keyexp->insertItem(i18n("Years"),4);
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
        keykind->setMinimumSize(keykind->sizeHint());

        vbox->addWidget(bgroup1);
        page->show();
        page->resize(page->maximumSize());
        setMainWidget(page);
}

void keyGenerate::slotOk()
{
        if (QString(kname->text()).stripWhiteSpace().isEmpty()) {
                KMessageBox::sorry(this,i18n("You must give a name."));
                return;
        }
        QString vmail=mail->text();
	if (vmail.isEmpty())
	{
	if (KMessageBox::warningContinueCancel(this,i18n("You are about to create a key with no email address"))!=KMessageBox::Continue) return;
        }
	else if ((vmail.find(" ")!=-1) || (vmail.find(".")==-1) || (vmail.find("@")==-1)) {
                KMessageBox::sorry(this,i18n("Email address not valid"));
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
        if (state==0)
                numb->setDisabled(true);
        else
                numb->setDisabled(false);
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
        else
                return ("");
}

QString keyGenerate::getkeyname()
{
        if (kname->text()!=NULL)
                return(kname->text());
        else
                return ("");
}

QString keyGenerate::getkeymail()
{
        if (mail->text()!=NULL)
                return(mail->text());
        else
                return ("");
}

QString keyGenerate::getkeycomm()
{
        if (comment->text()!=NULL)
                return(comment->text());
        else
                return ("");
}

#include "keygener.moc"
