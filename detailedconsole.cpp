/***************************************************************************
                          detailledconsole.cpp  -  description
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


#include "detailedconsole.h"
#include <qlayout.h>
#include <qlabel.h>
#include <qvgroupbox.h>

#include <klocale.h>
#include <klistbox.h>
#include <kglobal.h>

#include "kgpgoptions.h"


KDetailedConsole::KDetailedConsole(QWidget *parent, const char *name,const QString &boxLabel,const QString &errormessage)
    : KDialogBase(parent,name,true,i18n("Sorry"),KDialogBase::Details|KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok)
{
        QWidget *page = new QWidget( this );
        setMainWidget(page);
        QVBoxLayout *vbox=new QVBoxLayout(page,0, spacingHint() );

        QLabel *lab1=new QLabel(page);
        lab1->setText(boxLabel);

        QVGroupBox *detailsGroup = new QVGroupBox( i18n("Details"), page);
        (void) new QLabel(errormessage,detailsGroup);
        //labdetails->setMinimumSize(labdetails->sizeHint());
        setDetailsWidget(detailsGroup);
        vbox->addWidget(lab1);

}

KDetailedConsole::~KDetailedConsole()
{}

KDetailedInfo::KDetailedInfo(QWidget *parent, const char *name , const QString &boxLabel,const QString &errormessage,QStringList keysList)
    : KDialogBase(parent,name,true,i18n("Info"),KDialogBase::Details|KDialogBase::Ok)
{
        QWidget *page = new QWidget( this );

        QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );

        QLabel *label = new QLabel( boxLabel, page, "caption" );
	KListBox *list=new KListBox(page);
	list->insertStringList(keysList);

        topLayout->addWidget( label );
	topLayout->addWidget( list );
	if (keysList.isEmpty()) list->hide();

        QVGroupBox *detailsGroup = new QVGroupBox( i18n("Details"), this);
        QLabel *labdetails = new QLabel(errormessage,detailsGroup);
        labdetails->setMinimumSize(labdetails->sizeHint());
        setDetailsWidget(detailsGroup);

	setMainWidget(page);
}

KDetailedInfo::~KDetailedInfo()
{}

//#include "detailedconsole.moc"
