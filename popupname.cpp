/***************************************************************************
                          popupname.cpp  -  description
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

popupName::popupName(const QString& caption,QWidget *parent, const char *name,KURL oldnam,bool email):KDialogBase( parent, name, true,name,Ok | Cancel)
{
        resize( 350, 180 );
        setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, sizePolicy().hasHeightForWidth() ) );
        setMinimumSize( QSize( 350, 180 ) );
        setMaximumSize( QSize( 350, 180 ) );
        QWidget *page = new QWidget(this);
        QVBoxLayout *vbox=new QVBoxLayout(page,2);

        bGroupSources = new QButtonGroup( page, "bGroupSources" );
        bGroupSources->setGeometry( QRect( 0, 0, 325, 120 ) );
        bGroupSources->setTitle(caption);
        bGroupSources->setColumnLayout(0, Qt::Vertical );
        bGroupSources->layout()->setSpacing( 6 );
        bGroupSources->layout()->setMargin( 11 );
        bGroupSourcesLayout = new QGridLayout( bGroupSources->layout() );
        bGroupSourcesLayout->setAlignment( Qt::AlignTop );

        checkClipboard = new QRadioButton( bGroupSources, "checkClipboard" );


        if (email) {
                checkClipboard->setText( i18n( "Clipboard" ) );
                checkMail = new QRadioButton( bGroupSources, "checkMail" );
                checkMail->setText( i18n( "Email" ) );
                exportAttributes = new QCheckBox(i18n("Export attributes (photo ID)"),page);
        } else
                checkClipboard->setText( i18n( "Editor" ) );

        bGroupSourcesLayout->addWidget( checkClipboard, 1, 0 );
        newFilename = new KLineEdit( bGroupSources, "newFilename" );

        path=oldnam;

        newFilename->setText(oldnam.path());


        if (email)
                bGroupSourcesLayout->addWidget( checkMail, 0, 0 );

        checkFile = new QRadioButton( bGroupSources, "checkFile" );
        checkFile->setText( i18n( "File:" ) );
        checkFile->setChecked( TRUE );

        buttonToolbar = new QPushButton( bGroupSources, "buttonToolbar" );
        buttonToolbar->setMinimumSize( QSize( 0, 0 ) );
        buttonToolbar->setMaximumSize( QSize( 32, 32767 ) );
        buttonToolbar->setText( trUtf8( "..." ) );

        bGroupSourcesLayout->addWidget( checkFile, 2, 0 );
        bGroupSourcesLayout->addWidget( newFilename, 2, 1 );
        bGroupSourcesLayout->addWidget( buttonToolbar, 2, 2 );

        // signals and slots connections

        connect( checkFile, SIGNAL( toggled(bool) ), this, SLOT( slotenable(bool) ) );
        connect( buttonToolbar, SIGNAL( clicked() ), this, SLOT( slotchooseurl()));
        // tab order



        setTabOrder( checkFile, newFilename );
        setTabOrder( newFilename,buttonToolbar);

        vbox->add
        (bGroupSources);
        if (email) {
                vbox->add
                (exportAttributes);
                setTabOrder(buttonToolbar,exportAttributes);
        }
        page->show();
        page->resize(page->maximumSize());
        setMainWidget(page);
}

void popupName::slotchooseurl()
{
        /////////  toggle editing of file name depending of the user choice
        KURL url=KFileDialog::getSaveURL(path.path(),i18n("*|All Files"), this, i18n("Save As"));
        if(!url.isEmpty())
                newFilename->setText(url.path());

}


void popupName::slotenable(bool on)
{
        /////////  toggle editing of file name depending of the user choice
        newFilename->setEnabled(on);
}
//#include "popupname.moc"
#include "popupname.moc"
