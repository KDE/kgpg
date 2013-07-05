/*
 * Copyright (C) 2010,2012,2013 Rolf Eike Beer <kde@opensource.sf-tec.de>
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

#include <KDebug>
#include <KLocale>
#include <QtCore/QFile>

KGpgGenerateRevoke::KGpgGenerateRevoke(QObject *parent, const QString &keyID, const KUrl &revokeUrl, const int reason, const QString &description)
	: KGpgTransaction(parent),
	m_keyid(keyID),
	m_revUrl(revokeUrl),
	m_reason(reason),
	m_description(description)
{
	addArgument(QLatin1String( "--status-fd=1" ));
	addArgument(QLatin1String( "--command-fd=0" ));
	addArgument(QLatin1String( "--no-verbose" ));

	if (!revokeUrl.isEmpty()) {
		addArgument(QLatin1String( "-o" ));
		addArgument(revokeUrl.toLocalFile());
	}
	addArgument(QLatin1String( "--gen-revoke" ));
	addArgument(keyID);
}

KGpgGenerateRevoke::~KGpgGenerateRevoke()
{
}

bool
KGpgGenerateRevoke::preStart()
{
	setSuccess(TS_MSG_SEQUENCE);

	setDescription(i18n("Generating Revocation Certificate for key %1", m_keyid));

	return true;
}

bool
KGpgGenerateRevoke::nextLine(const QString &line)
{
	if (!line.startsWith(QLatin1String("[GNUPG:] "))) {
		m_output.append(line + QLatin1Char( '\n' ));
		return false;
	}

	if (line.contains(QLatin1String( "NEED_PASSPHRASE" ))) {
		setSuccess(TS_USER_ABORTED);
	} else if (line.contains(QLatin1String( "ask_revocation_reason.code" ))) {
		write(QByteArray::number(m_reason));
	} else if (line.contains(QLatin1String( "ask_revocation_reason.text" ))) {
		write(m_description.toUtf8());
		// GnuPG stops asking if we pass an empty line
		m_description.clear();
	} else if (line.contains(QLatin1String( "GET_" ))) {
		setSuccess(TS_MSG_SEQUENCE);
		kDebug(2100) << line;
		return true;
	}

	return false;
}

KGpgTransaction::ts_boolanswer
KGpgGenerateRevoke::boolQuestion(const QString& line)
{
	if (line == QLatin1String("gen_revoke.okay")) {
		return BA_YES;
	} else if (line == QLatin1String("ask_revocation_reason.okay")) {
		return BA_YES;
	} else {
		return KGpgTransaction::boolQuestion(line);
	}
}

void
KGpgGenerateRevoke::finish()
{
	if (getSuccess() == TS_OK) {
		if (!m_revUrl.isEmpty()) {
			QFile of(m_revUrl.toLocalFile());
			if (of.open(QIODevice::ReadOnly)) {
				m_output = QLatin1String( of.readAll() );
				of.close();
			}
		}
		emit revokeCertificate(m_output);
	}
}

bool
KGpgGenerateRevoke::passphraseReceived()
{
	setSuccess(TS_OK);

	return false;
}

KGpgTransaction::ts_boolanswer
KGpgGenerateRevoke::confirmOverwrite(KUrl &currentFile)
{
	currentFile = m_revUrl;
	return BA_UNKNOWN;
}

const QString &
KGpgGenerateRevoke::getOutput() const
{
	return m_output;
}

#include "kgpggeneraterevoke.moc"
