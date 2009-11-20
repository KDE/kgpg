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

#include "kgpgchangeexpire.h"

#include <KLocale>
#include <KMessageBox>

#include "detailedconsole.h"

KGpgChangeExpire::KGpgChangeExpire(QObject *parent, const QString &keyid, const QDate &date)
	: KGpgEditKeyTransaction(parent, keyid, "expire", false)
{
	setDate(date);
}

KGpgChangeExpire::~KGpgChangeExpire()
{
}

bool
KGpgChangeExpire::nextLine(const QString &line)
{
	if (!line.startsWith(QLatin1String("[GNUPG:]")))
		return false;

	if (line.contains("GOOD_PASSPHRASE")) {
		setSuccess(TS_OK);

		return false;
	} else if (line.contains("keygen.valid")) {
		if (m_date.isNull())
			write("0");
		else
			write(QByteArray::number(QDate::currentDate().daysTo(m_date)));

		return false;
	} else {
		return KGpgEditKeyTransaction::nextLine(line);
	}
}

void
KGpgChangeExpire::finish()
{
	if (getSuccess() == TS_MSG_SEQUENCE) {
		QString output;
		KgpgDetailedConsole *q = new KgpgDetailedConsole(0, i18n("<qt><b>Changing expiration failed.</b><br />"
					"Do you want to try changing the key expiration in console mode?</qt>"), output);
		if (q->exec() == QDialog::Accepted) {
			KMessageBox::sorry(0, i18n("Work in progress..."));
			setSuccess(TS_OK);
		} else
			setSuccess(TS_USER_ABORTED);
	}
}

void
KGpgChangeExpire::setDate(const QDate &date)
{
	m_date = date;
}
