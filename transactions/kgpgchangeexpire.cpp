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

#include "kgpgchangeexpire.h"

#include "detailedconsole.h"

#include <KLocale>
#include <KMessageBox>

KGpgChangeExpire::KGpgChangeExpire(QObject *parent, const QString &keyid, const QDateTime &date)
	: KGpgEditKeyTransaction(parent, keyid, QLatin1String( "expire" ), false)
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

	if (line.contains(QLatin1String( "GOOD_PASSPHRASE" ))) {
		setSuccess(TS_OK);

		return false;
	} else if (line.contains(QLatin1String( "keygen.valid" ))) {
		if (m_date.isNull())
			write("0");
		else
			write(QByteArray::number(QDate::currentDate().daysTo(m_date.date())));

		return false;
	} else {
		return KGpgEditKeyTransaction::nextLine(line);
	}
}

void
KGpgChangeExpire::setDate(const QDateTime &date)
{
	m_date = date;
}

#include "kgpgchangeexpire.moc"
