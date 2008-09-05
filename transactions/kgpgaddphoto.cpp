/**
 * Copyright (C) 2008 Rolf Eike Beer <kde@opensource.sf-tec.de>
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
	if (!line.startsWith("[GNUPG:] "))
		return false;

	if (line.contains("BAD_PASSPHRASE")) {
		setSuccess(1);
	} else if (line.contains("GOOD_PASSPHRASE")) {
		setSuccess(2);
	} else if (line.contains("passphrase.enter")) {
		if (askPassphrase())
			setSuccess(3);
	} else if (line.contains("keyedit.prompt")) {
		write("save");
	} else if (line.endsWith("photoid.jpeg.add")) {
		write(m_photourl.toAscii());
	} else if (line.contains("photoid.jpeg.size")) {
		if (KMessageBox::questionYesNo(0, i18n("This image is very large. Use it anyway?"), QString(), KGuiItem(i18n("Use Anyway")), KGuiItem(i18n("Do Not Use"))) == KMessageBox::Yes) {
			write("Yes");
		} else {
			setSuccess(4);
			return true;
		}
	} else if (line.contains("GET_")) {
		// gpg asks for something unusal, turn to konsole mode
		return true;
	}

	return false;
}

void
KGpgAddPhoto::setImagePath(const QString &photourl)
{
	m_photourl = photourl;
}
