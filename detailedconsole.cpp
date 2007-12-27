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

#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>

#include <KMessageBox>
#include <KLocale>


KgpgDetailedConsole::KgpgDetailedConsole(QWidget *parent, const QString &boxLabel, const QString &errormessage)
    : KDialog(parent)
{
    setCaption( i18nc("see kdeui/dialogs/kmessagebox.cpp", "Sorry") );
    setButtons( Details | Yes | No);
    setDefaultButton( No );
    setModal(true);
    setDefaultButton(Yes);

    QWidget *page = new QWidget(this);
    setMainWidget(page);

    QVBoxLayout *vbox = new QVBoxLayout(page);
    vbox->setMargin(0);
    vbox->setSpacing(spacingHint());

    QLabel *lab1 = new QLabel(page);
    lab1->setText(boxLabel);

    QGroupBox *detailsGroup = new QGroupBox(i18n("Details"), page);
    (void) new QLabel(errormessage, detailsGroup);
    //labdetails->setMinimumSize(labdetails->sizeHint());

    setDetailsWidget(detailsGroup);
    vbox->addWidget(lab1);
}

KgpgDetailedInfo::KgpgDetailedInfo(QWidget *parent, const QString &boxLabel, const QString &errormessage, const QStringList &keysList)
                : KDialog(parent)
{
    setCaption( i18n("Info") );
    setButtons( Details | Ok );
    setDefaultButton( Ok );
    setModal( true );
    bool checkboxResult;
    KMessageBox::createKMessageBox(this, QMessageBox::Information, boxLabel, keysList, QString(), &checkboxResult, 0, errormessage); // krazy:exclude=qtclasses
}
