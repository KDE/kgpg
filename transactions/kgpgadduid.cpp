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

#include "kgpgadduid.h"

#include <kpimutils/email.h>

KGpgAddUid::KGpgAddUid(QObject *parent, const QString &keyid, const QString &name, const QString &email, const QString &comment)
	: KGpgEditKeyTransaction(parent, keyid, QLatin1String("adduid"), false, true)
{
	setName(name);
	setEmail(email);
	setComment(comment);
}

KGpgAddUid::~KGpgAddUid()
{
}

bool
KGpgAddUid::preStart()
{
	if (!KGpgEditKeyTransaction::preStart())
		return false;

	if (!m_email.isEmpty() && !KPIMUtils::isValidSimpleAddress(m_email)) {
		setSuccess(TS_INVALID_EMAIL);
		return false;
	}

	return true;
}

bool
KGpgAddUid::nextLine(const QString &line)
{
	if (!line.startsWith(QLatin1String("[GNUPG:] ")))
		return false;

	if (line.contains(QLatin1String( "GOOD_PASSPHRASE" ))) {
		setSuccess(TS_OK);
	} else if (line.contains(QLatin1String( "keygen.name" ))) {
		write(m_name.toUtf8());
	} else if (line.contains(QLatin1String( "keygen.email" ))) {
		write(m_email.toAscii());
	} else if (line.contains(QLatin1String( "keygen.comment" ))) {
		write(m_comment.toUtf8());
	} else {
		return KGpgEditKeyTransaction::nextLine(line);
	}

	return false;
}

void
KGpgAddUid::setName(const QString &name)
{
	m_name = name;
}

void
KGpgAddUid::setEmail(const QString &email)
{
	m_email = email;
}

void
KGpgAddUid::setComment(const QString &comment)
{
	m_comment = comment;
}

#include "kgpgadduid.moc"
