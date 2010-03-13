/*
 * Copyright (C) 2010 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpggeneraterevoke.h"

#include <KMessageBox>
#include <KLocale>
#include <QtCore/QFile>

KGpgGenerateRevoke::KGpgGenerateRevoke(QObject *parent, const QString &keyID, const KUrl &revokeUrl, const int reason, const QString &description)
	: KGpgTransaction(parent),
	m_keyid(keyID),
	m_revUrl(revokeUrl),
	m_reason(reason),
	m_description(description)
{
	addArgument("--status-fd=1");
	addArgument("--command-fd=0");
	addArgument("--no-verbose");
	addArgument("--no-greeting");

	if (!revokeUrl.isEmpty()) {
		addArgument("-o");
		addArgument(revokeUrl.toLocalFile());
	}
	addArgument("--gen-revoke");
	addArgument(keyID);
}

KGpgGenerateRevoke::~KGpgGenerateRevoke()
{
}

bool
KGpgGenerateRevoke::preStart()
{
	setSuccess(TS_MSG_SEQUENCE);

	setDescription(i18n("Generating Revokation Certificate for key %1", m_keyid));

	return true;
}

bool
KGpgGenerateRevoke::nextLine(const QString &line)
{
	if (!line.startsWith(QLatin1String("[GNUPG:] "))) {
		m_output.append(line + '\n');
		return false;
	}

	if (line.contains("GOOD_PASSPHRASE")) {
		setSuccess(TS_OK);
	} else if (line.contains("passphrase.enter")) {
		if (!askPassphrase()) {
			setSuccess(TS_USER_ABORTED);
			return true;
		}
	} else if (line.contains("NEED_PASSPHRASE")) {
		setSuccess(TS_USER_ABORTED);
	} else if (line.contains("gen_revoke.okay") ||
			line.contains("ask_revocation_reason.okay") ||
			line.contains("openfile.overwrite.okay")) {
		write("YES");
	} else if (line.contains("ask_revocation_reason.code")) {
		write(QByteArray::number(m_reason));
	} else if (line.contains("ask_revocation_reason.text")) {
		write(m_description.toUtf8());
		// GnuPG stops asking if we pass an empty line
		m_description.clear();
	} else if (line.contains("GET_")) {
		setSuccess(TS_MSG_SEQUENCE);
		return true;
	}

	return false;
}

void
KGpgGenerateRevoke::finish()
{
	if (getSuccess() == TS_OK) {
		if (!m_revUrl.isEmpty()) {
			QFile of(m_revUrl.toLocalFile());
			if (of.open(QIODevice::ReadOnly)) {
				m_output = of.readAll();
				of.close();
			}
		}
		emit revokeCertificate(m_output);
	}
}

const QString &
KGpgGenerateRevoke::getOutput() const
{
	return m_output;
}
