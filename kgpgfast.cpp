/***************************************************************************
                          kgpgfast.cpp  -  description
                             -------------------
    begin                : Sat Jun 29 2002
    copyright            : (C) 2002 by 
    email                : 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

////////////////////////////////////////////////////////   code  for file renaming

#include <qlineedit.h>

#include "kgpgfast.h"


KgpgOverwrite::KgpgOverwrite( QWidget *parent, const char *name,KURL ofile):KDialogBase( parent, name, true,i18n("File already exists"),0)
{
QWidget *page = new QWidget(this);
direc=ofile.directory(0,0);
QVBoxLayout *vbox=new QVBoxLayout(page,3);

QLabel *label=new QLabel(i18n("File <b>%1</b> already exists").arg(ofile.filename()),page);
lineedit=new KLineEdit(page);
lineedit->setText(ofile.filename());

KButtonBox *boutonbox=new KButtonBox(page,KButtonBox::Horizontal,15,12);
  boutonbox->addStretch(1);
  bouton1=boutonbox->addButton(i18n("&Overwrite"),TRUE);
  bouton2=boutonbox->addButton(i18n("&Rename"),TRUE);
  bouton3=boutonbox->addButton(i18n("&Cancel"),TRUE);
  
  QObject::connect(lineedit,SIGNAL(textChanged(const QString &)),this,SLOT(enablerename()));
  QObject::connect(lineedit,SIGNAL(returnPressed(const QString &)),this,SLOT(slotcheck()));
  QObject::connect(bouton1,SIGNAL(clicked()),this,SLOT(slotok()));
  QObject::connect(bouton2,SIGNAL(clicked()),this,SLOT(slotcheck()));
  QObject::connect(bouton3,SIGNAL(clicked()),this,SLOT(annule()));
  
bouton2->setDisabled(true);

  vbox->addWidget(label);
    vbox->addWidget(lineedit);
  vbox->addWidget(boutonbox);
  //page->resize(page->minimumSize());
  //resize(this->minimumSize());
  page->show();
  setMainWidget(page);
}


void KgpgOverwrite::enablerename()
{
bouton2->setDisabled(false);
bouton1->setDisabled(true);
}

void KgpgOverwrite::annule()
{
reject();
}

void KgpgOverwrite::slotok()
{
accept();
}


void KgpgOverwrite::slotcheck()
{
QFile nname(direc+lineedit->text());
if (nname.exists())
{
int result=KMessageBox::warningContinueCancel(this,i18n("File %1 already exists").arg(lineedit->text()),i18n("Warning"),i18n("Overwrite"));
if (result==KMessageBox::Cancel) return;
else accept();
}
else accept();
}

QString KgpgOverwrite::getfname()
{
return (lineedit->text());
}
    



#include "kgpgfast.moc"
