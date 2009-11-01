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

#include "kgpggeneratekey.h"

#include <KMessageBox>
#include <KLocale>

#include "kpimutils/email.h"

KGpgGenerateKey::KGpgGenerateKey(QObject *parent, const QString &name, const QString &email, const QString &comment,
		const KgpgCore::KgpgKeyAlgo &algorithm, const uint size, const unsigned int expire,
		const char expireunit)
	: KGpgTransaction(parent)
{
	addArgument("--status-fd=1");
	addArgument("--command-fd=0");
	addArgument("--no-verbose");
	addArgument("--no-greeting");
	addArgument("--gen-key");
	addArgument("--batch");

	setName(name);
	setEmail(email);
	setComment(comment);
	setAlgorithm(algorithm);
	setSize(size);
	setExpire(expire, expireunit);
}

KGpgGenerateKey::~KGpgGenerateKey()
{
}

bool
KGpgGenerateKey::preStart()
{
	if (!m_email.isEmpty() && !KPIMUtils::isValidSimpleAddress(m_email)) {
		setSuccess(TS_INVALID_EMAIL);
		return false;
	}

	m_fingerprint.clear();
	m_started = false;
	m_namesent = false;

	setSuccess(TS_MSG_SEQUENCE);

	setDescription("Generating key for " + m_name);

	return true;
}

void
KGpgGenerateKey::postStart()
{
	QByteArray keymessage("Key-Type: ");
	if (m_algorithm == KgpgCore::ALGO_RSA)
		keymessage.append("RSA");
	else
		keymessage.append("DSA");

	keymessage.append("\nKey-Length: ");
	keymessage.append(QByteArray::number(m_size));
	keymessage.append("\nName-Real: ");
	keymessage.append(m_name.toUtf8());
	if (!m_email.isEmpty()) {
		keymessage.append("\nName-Email: ");
		keymessage.append(m_email.toAscii());
	}
	if (!m_comment.isEmpty()) {
		keymessage.append("\nName-Comment: ");
		keymessage.append(m_comment.toUtf8());
	}
	if (m_expire != 0) {
		keymessage.append("\nExpire-Date: ");
		keymessage.append(QByteArray::number(m_expire));
		keymessage.append(m_expireunit);
	}
	keymessage.append("\nPassphrase: ");
	write(keymessage, false);

	QString passdlgmessage;
	if (!m_email.isEmpty()) {
		passdlgmessage = i18n("<p><b>Enter passphrase for %1 &lt;%2&gt;</b>:<br />Passphrase should include non alphanumeric characters and random sequences.</p>", m_name, m_email);
	} else {
		passdlgmessage = i18n("<p><b>Enter passphrase for %1</b>:<br />Passphrase should include non alphanumeric characters and random sequences.</p>", m_name);
	}

	if (sendPassphrase(passdlgmessage, true)) {
		setSuccess(TS_USER_ABORTED);
	}
	keymessage.append("%commit");

	write(keymessage);
}

bool
KGpgGenerateKey::nextLine(const QString &line)
{
	if (!line.startsWith("[GNUPG:] "))
		return false;

	if (line.contains("PROGRESS")) {
		if (!m_started) {
			emit generateKeyStarted();
			m_started = true;
		}
	} else if (line.contains("GOOD_PASSPHRASE")) {
		setSuccess(TS_MSG_SEQUENCE);
	} else if (line.contains("KEY_CREATED")) {
		m_fingerprint = line.right(40);
		setSuccess(TS_OK);
		return true;
	} else if (line.contains("NEED_PASSPHRASE")) {
		setSuccess(TS_USER_ABORTED);
	} else if (line.contains("GET_")) {
		setSuccess(TS_MSG_SEQUENCE);
		return true;
	}

	return false;
}

void
KGpgGenerateKey::setName(const QString &name)
{
	m_name = name;
}

QString
KGpgGenerateKey::getName() const
{
	return m_name;
}

void
KGpgGenerateKey::setEmail(const QString &email)
{
	m_email = email;
}

QString
KGpgGenerateKey::getEmail() const
{
	return m_email;
}

void
KGpgGenerateKey::setComment(const QString &comment)
{
	m_comment = comment;
}

void
KGpgGenerateKey::setAlgorithm(const KgpgCore::KgpgKeyAlgo &algorithm)
{
	m_algorithm = algorithm;
}

void
KGpgGenerateKey::setSize(const unsigned int size)
{
	m_size = size;
}

void
KGpgGenerateKey::setExpire(const unsigned int expire, const char expireunit)
{
	Q_ASSERT((expireunit == 'd') || (expireunit == 'w') ||
			(expireunit == 'm') || (expireunit == 'y'));
	m_expire = expire;
	m_expireunit = expireunit;
}

QString
KGpgGenerateKey::getFingerprint() const
{
	return m_fingerprint;
}
