/*
    SPDX-FileCopyrightText: 2008-2022 Rolf Eike Beer <kde@opensource.sf-tec.de>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kgpgaddphoto.h"

#include <KLocalizedString>
#include <KMessageBox>

KGpgAddPhoto::KGpgAddPhoto(QObject *parent, const QString &keyid, const QString &imagepath)
	: KGpgEditKeyTransaction(parent, keyid, QLatin1String( "addphoto" ), false)
{
	setImagePath(imagepath);
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
	} else {
		return KGpgEditKeyTransaction::nextLine(line);
	}

	return false;
}

KGpgTransaction::ts_boolanswer KGpgAddPhoto::boolQuestion(const QString &line)
{
	if (line == QLatin1String("photoid.jpeg.size")) {
        if (KMessageBox::questionTwoActions(nullptr, i18n("This image is very large. Use it anyway?"), QString(), KGuiItem(i18n("Use Anyway")), KGuiItem(i18n("Do Not Use"))) == KMessageBox::PrimaryAction) {
			return BA_YES;
		} else {
			setSuccess(TS_USER_ABORTED);
			return BA_NO;
		}
	}

	return BA_UNKNOWN;
}

void
KGpgAddPhoto::setImagePath(const QString &photourl)
{
	m_photourl = photourl;
}

#include "moc_kgpgaddphoto.cpp"
