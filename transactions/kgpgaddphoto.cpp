/**
 * Copyright (C) 2008,2009 Rolf Eike Beer <kde@opensource.sf-tec.de>
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
	: KGpgTransaction(parent)
{
	addArgument("--status-fd=1");
	addArgument("--command-fd=0");
	addArgument("--edit-key");
	addArgument(keyid);
	addArgument("addphoto");

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

	if (line.contains("GOOD_PASSPHRASE")) {
		setSuccess(TS_MSG_SEQUENCE);
	} else if (line.contains("passphrase.enter")) {
		if (askPassphrase())
			setSuccess(TS_USER_ABORTED);
	} else if (line.contains("keyedit.prompt")) {
		setSuccess(TS_OK);
		write("save");
	} else if (line.endsWith(QLatin1String("photoid.jpeg.add"))) {
		write(m_photourl.toUtf8());
	} else if (line.contains("photoid.jpeg.size")) {
		if (KMessageBox::questionYesNo(0, i18n("This image is very large. Use it anyway?"), QString(), KGuiItem(i18n("Use Anyway")), KGuiItem(i18n("Do Not Use"))) == KMessageBox::Yes) {
			write("Yes");
		} else {
			setSuccess(TS_USER_ABORTED);
			return true;
		}
	} else if (line.contains("GET_")) {
		setSuccess(TS_MSG_SEQUENCE);
		return true;
	}

	return false;
}

void
KGpgAddPhoto::setImagePath(const QString &photourl)
{
	m_photourl = photourl;
}
