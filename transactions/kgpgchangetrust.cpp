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

#include "kgpgchangetrust.h"

KGpgChangeTrust::KGpgChangeTrust(QObject *parent, const QString &keyid, const gpgme_validity_t trust)
	: KGpgEditKeyTransaction(parent, keyid, QLatin1String( "trust" ), false)
{
	setTrust(trust);
}

KGpgChangeTrust::~KGpgChangeTrust()
{
}

bool
KGpgChangeTrust::preStart()
{
	setSuccess(TS_MSG_SEQUENCE);

	return true;
}

bool
KGpgChangeTrust::nextLine(const QString &line)
{
	if (line.contains(QLatin1String( "edit_ownertrust.value" ))) {
		write(QByteArray::number(m_trust));
		setSuccess(TS_OK);
	} else {
		return KGpgEditKeyTransaction::nextLine(line);
	}

	return false;
}

KGpgTransaction::ts_boolanswer
KGpgChangeTrust::boolQuestion(const QString& line)
{
	if (line == QLatin1String("edit_ownertrust.set_ultimate.okay")) {
		return BA_YES;
	} else {
		return KGpgTransaction::boolQuestion(line);
	}
}

void
KGpgChangeTrust::setTrust(const gpgme_validity_t trust)
{
	m_trust = trust;
}

#include "kgpgchangetrust.moc"
