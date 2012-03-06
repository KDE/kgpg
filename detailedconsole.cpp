/***************************************************************************
                          detailedconsole.cpp  -  description
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

#include <KLocale>
#include <KMessageBox>

KgpgDetailedInfo::KgpgDetailedInfo(QWidget *parent, const QString &boxLabel, const QString &errormessage,
		const QStringList &keysList, const QString &caption)
	: KDialog(parent)
{
	if (!caption.isEmpty())
		setCaption(caption);
	else
		setCaption(i18n("Info"));
	setButtons(Details | Ok);
	setDefaultButton(Ok);
	setModal(true);
	KMessageBox::createKMessageBox(this, QMessageBox::Information, // krazy:exclude=qtclasses
				   boxLabel, keysList, QString(), NULL, 0, errormessage);
}

#include "detailedconsole.moc"
