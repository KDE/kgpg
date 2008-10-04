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

#include "kgpgadduid.h"

#include "kpimutils/email.h"

KGpgAddUid::KGpgAddUid(QObject *parent, const QString &keyid, const QString &name, const QString &email, const QString &comment)
	: KGpgTransaction(parent)
{
	addArgument("--status-fd=1");
	addArgument("--command-fd=0");
	addArgument("--edit-key");
	addArgument(keyid);
	addArgument("adduid");

	m_keyid = keyid;

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
	if (!m_email.isEmpty() && !KPIMUtils::isValidSimpleAddress(m_email)) {
		setSuccess(4);
		return false;
	}

	return true;
}

bool
KGpgAddUid::nextLine(const QString &line)
{
	if (!line.startsWith("[GNUPG:] "))
		return false;

	if (line.contains("GOOD_PASSPHRASE")) {
		setSuccess(2);
	} else if (line.contains("passphrase.enter")) {
		if (askPassphrase())
			setSuccess(3);
	} else if (line.contains("keyedit.prompt")) {
		setSuccess(0);
		write("save");
	} else if (line.contains("keygen.name")) {
		write(m_name.toAscii());
	} else if (line.contains("keygen.email")) {
		write(m_email.toAscii());
	} else if (line.contains("keygen.comment")) {
		write(m_comment.toAscii());
	} else if (line.contains("GET_")) {
		// gpg asks for something unusal, turn to konsole mode
		return true;
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

QString
KGpgAddUid::getKeyid() const
{
	return m_keyid;
}