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
		const KgpgCore::KgpgKeyAlgo &algorithm, const uint &size, const unsigned int &expire,
		const unsigned int &expireunit)
	: KGpgTransaction(parent)
{
	addArgument("--status-fd=1");
	addArgument("--command-fd=0");
	addArgument("--no-verbose");
	addArgument("--no-greeting");
	addArgument("--gen-key");

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

	return true;
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
	} else if (line.contains("keygen.algo")) {
		if (m_algorithm == KgpgCore::ALGO_RSA)
			write("5");
		else
			write("1");
	} else if (line.contains("keygen.size")) {
		write(QByteArray::number(m_size));
	} else if (line.contains("keygen.valid")) {
		if (m_expire != 0) {
			QByteArray output(QByteArray::number(m_expire));

			switch (m_expireunit) {
			case 1:
				output.append("d");
				break;
			case 2:
				output.append("w");
				break;
			case 3:
				output.append("m");
				break;
			case 4:
				output.append("y");
				break;
			}
			write(output);
		} else {
			write("0");
		}
	} else if (line.contains("keygen.name")) {
		if (m_namesent) {
			setSuccess(TS_INVALID_NAME);
			return true;
//			p->kill();
		} else {
			m_namesent = true;
			write(m_name.toUtf8());
		}
	} else if (line.contains("keygen.email")) {
		write(m_email.toAscii());
	} else if (line.contains("keygen.comment")) {
		write(m_comment.toUtf8());
	} else if (line.contains("passphrase.enter")) {
		QString passdlgmessage;

		if (!m_email.isEmpty()) {
			passdlgmessage = i18n("<p><b>Enter passphrase for %1 &lt;%2&gt;</b>:<br />Passphrase should include non alphanumeric characters and random sequences.</p>", m_name, m_email);
		} else {
			passdlgmessage = i18n("<p><b>Enter passphrase for %1</b>:<br />Passphrase should include non alphanumeric characters and random sequences.</p>", m_name);
		}

		if (sendPassphrase(passdlgmessage, true)) {
			setSuccess(TS_USER_ABORTED);
		} else {
			setSuccess(TS_MSG_SEQUENCE);
		}
	} else if (line.contains("GOOD_PASSPHRASE")) {
		setSuccess(TS_MSG_SEQUENCE);
	} else if (line.contains("KEY_CREATED")) {
		m_fingerprint = line.right(40);
		setSuccess(TS_OK);
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
KGpgGenerateKey::setSize(const unsigned int &size)
{
	m_size = size;
}

void
KGpgGenerateKey::setExpire(const unsigned int &expire, const unsigned int &expireunit)
{
	m_expire = expire;
	m_expireunit = expireunit;
}

QString
KGpgGenerateKey::getFingerprint() const
{
	return m_fingerprint;
}
