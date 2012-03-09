/*
 * Copyright (C) 2008,2009,2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgaddphoto.h"

#include <KMessageBox>
#include <KLocale>

KGpgAddPhoto::KGpgAddPhoto(QObject *parent, const QString &keyid, const QString &imagepath)
	: KGpgEditKeyTransaction(parent, keyid, QLatin1String( "addphoto" ), false)
{
	setImagePath(imagepath);
}

KGpgAddPhoto::~KGpgAddPhoto()
{
}

bool
KGpgAddPhoto::nextLine(const QString &line)
{
	if (!line.startsWith(QLatin1String("[GNUPG:] ")))
		return false;

	if (line.contains(QLatin1String( "GOOD_PASSPHRASE" ))) {
		setSuccess(TS_MSG_SEQUENCE);
	} else if (line.endsWith(QLatin1String("photoid.jpeg.add"))) {
		write(m_photourl.toUtf8());
		setSuccess(TS_OK);
	} else if (line.contains(QLatin1String( "photoid.jpeg.size" ))) {
		if (KMessageBox::questionYesNo(0, i18n("This image is very large. Use it anyway?"), QString(), KGuiItem(i18n("Use Anyway")), KGuiItem(i18n("Do Not Use"))) == KMessageBox::Yes) {
			write("YES");
		} else {
			setSuccess(TS_USER_ABORTED);
			return true;
		}
	} else {
		return KGpgEditKeyTransaction::nextLine(line);
	}

	return false;
}

void
KGpgAddPhoto::setImagePath(const QString &photourl)
{
	m_photourl = photourl;
}

#include "kgpgaddphoto.moc"
