/***************************************************************************
                          kgpgshredwidget.cpp  -  description
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



#include "kgpgshredwidget.h"

kgpgShredWidget::kgpgShredWidget(QWidget *parent, const char *name):KgpgShred( parent, name)
{
        update();
        //fileSize=QFile(sfile.path()).size();
        //kgpgShredFile(sfile);
}

kgpgShredWidget::~kgpgShredWidget()
{}

void kgpgShredWidget::kgpgShredFile(KURL sfile)
{
if (!sfile.isLocalFile())
{
KMessageBox::sorry(0,i18n("<qt>File <b>%1</b> is a remote file.<br>You cannot shred it.</qt>").arg(sfile.filename()));
return;
}
        fileSize=QFile(sfile.path()).size();
        kProgress2->setTotalSteps(QFile(sfile.path()).size());
        shredres=new KShred(sfile.path());
        connect(shredres,SIGNAL(processedSize(KIO::filesize_t)),this,SLOT(setValue(KIO::filesize_t)));
        //connect(shredres,SIGNAL(infoMessage(const QString &)),this,SLOT(setValue(const QString &)));
        if (shredres->shred())
                accept();
        else
                KMessageBox::sorry(0,i18n("<qt><b>ERROR</b> during file shredding.<br>File was not securely deleted.<qt>"));
}

void kgpgShredWidget::setValue(KIO::filesize_t byte)
//void kgpgShredWidget::setValue(const QString & txt)
{
        //textLabel2->setText(txt);
        //if ((byte-(((int)(byte/1000))*1000))<100)
        kProgress2->setValue((int)byte);
        repaint();
}

#include "kgpgshredwidget.moc"
