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

///////////////////////////////////////////////////////////////////// code for the decryption dialog box

#include <kmessagebox.h>
#include <kfiledialog.h>
#include <qlistview.h>
#include <qfile.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qvbox.h>

#include "popupname.h"
#include "kgpg.h"

////////////////////   main window

popupName::popupName(QWidget *parent, const char *name,KURL oldnam,bool email):KDialogBase( parent, name, true,name,Ok | Cancel)
{
  resize( 350, 180 );

  setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, sizePolicy().hasHeightForWidth() ) );
  setMinimumSize( QSize( 350, 180 ) );
  setMaximumSize( QSize( 350, 180 ) );
  QWidget *page = new QWidget(this);
  bGroupSources = new QButtonGroup( page, "bGroupSources" );
  bGroupSources->setGeometry( QRect( 0, 0, 325, 120 ) );
  bGroupSources->setTitle(name);
  bGroupSources->setColumnLayout(0, Qt::Vertical );
  bGroupSources->layout()->setSpacing( 6 );
  bGroupSources->layout()->setMargin( 11 );
  bGroupSourcesLayout = new QGridLayout( bGroupSources->layout() );
  bGroupSourcesLayout->setAlignment( Qt::AlignTop );

  choix1 = new QRadioButton( bGroupSources, "choix1" );
  choix1->setText( i18n( "Editor" ) );
  
  if (email==true)
  {
  choix2 = new QRadioButton( bGroupSources, "choix2" );
  choix2->setText( i18n( "E-Mail" ) );
  }

  bGroupSourcesLayout->addMultiCellWidget( choix1, 0, 0, 0, 2 );

  lineedit = new KLineEdit( bGroupSources, "lineedit" );

  path=oldnam;

  lineedit->setText(oldnam.path());

  bGroupSourcesLayout->addWidget( lineedit, 2, 1 );
 if (email==true) bGroupSourcesLayout->addWidget( choix2, 1, 0 );
 
  choix0 = new QRadioButton( bGroupSources, "choix0" );
  choix0->setText( i18n( "File" ) );
  choix0->setChecked( TRUE );



  bGroupSourcesLayout->addWidget( choix0, 2, 0 );

  buttonToolbar = new QPushButton( bGroupSources, "buttonToolbar" );
  buttonToolbar->setMinimumSize( QSize( 0, 0 ) );
  buttonToolbar->setMaximumSize( QSize( 32, 32767 ) );
  buttonToolbar->setText( trUtf8( "..." ) );

  bGroupSourcesLayout->addWidget( buttonToolbar, 2, 2 );

  // signals and slots connections

  connect( choix0, SIGNAL( toggled(bool) ), this, SLOT( slotenable(bool) ) );
  connect( buttonToolbar, SIGNAL( clicked() ), this, SLOT( slotchooseurl()));
  // tab order
  setTabOrder( lineedit, buttonToolbar );
  setTabOrder( buttonToolbar, choix0 );
  setTabOrder( choix0, choix1 );
  page->show();
  page->resize(page->maximumSize());
  setMainWidget(page);


}

void popupName::slotchooseurl()
{
  /////////  toggle editing of file name depending of the user choice
  KURL url=KFileDialog::getSaveURL(path.path(),i18n("*|All files"), this, i18n("Save as..."));
  if(!url.isEmpty()) lineedit->setText(url.path());

}


void popupName::slotenable(bool on)
{
  /////////  toggle editing of file name depending of the user choice
  lineedit->setEnabled(on);
}


bool popupName::getfmode()
{
  return(choix0->isChecked());
}

bool popupName::getmailmode()
{
  return(choix2->isChecked());
}

QString popupName::getfname()
{
  return(lineedit->text());
}
