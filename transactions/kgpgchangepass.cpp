/*
 * Copyright (C) 2008,2009,2010,2011,2012,2013 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgchangepass.h"

#include <KLocale>

KGpgChangePass::KGpgChangePass(QObject *parent, const QString &keyid)
	: KGpgTransaction(parent),
	m_seenold(false)
{
	addArgument(QLatin1String( "--status-fd=1" ));
	addArgument(QLatin1String( "--command-fd=0" ));
	addArgument(QLatin1String( "--edit-key" ));
	addArgument(keyid);
	addArgument(QLatin1String( "passwd" ));
}

KGpgChangePass::~KGpgChangePass()
{
}

bool
KGpgChangePass::preStart()
{
	setSuccess(TS_MSG_SEQUENCE);

	return true;
}

bool
KGpgChangePass::nextLine(const QString &line)
{
	if (!line.startsWith(QLatin1String("[GNUPG:] ")))
		return false;

	if (line.contains(QLatin1String( "keyedit.prompt" ))) {
		if (m_seenold && (getSuccess() != TS_USER_ABORTED)) {
			setSuccess(TS_OK);
			write("save");
		} else {
			// some sort of error, we already set the error code
			return true;
		}
	} else if (line.contains(QLatin1String( "GET_" ))) {
		setSuccess(TS_MSG_SEQUENCE);
		return true;
	}

	return false;
}

bool
KGpgChangePass::passphraseRequested()
{
	const QString userIDs = getIdHints();

	if (!m_seenold) {
		return askPassphrase(i18n("Enter old passphrase for <b>%1</b>", userIDs));
	} else {
		askNewPassphrase(i18n("<qt>Enter new passphrase for <b>%1</b><br />If you forget this passphrase all your encrypted files and messages will be inaccessible.</qt>", userIDs));
	}

	return true;
}

bool
KGpgChangePass::passphraseReceived()
{
	m_seenold = true;
	setSuccess(TS_MSG_SEQUENCE);
	return false;
}

#include "kgpgchangepass.moc"
