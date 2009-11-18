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

#include "kgpgchangepass.h"

#include <KLocale>
#include <KMessageBox>

#include "detailedconsole.h"

KGpgChangePass::KGpgChangePass(QObject *parent, const QString &keyid)
	: KGpgTransaction(parent)
{
	addArgument("--status-fd=1");
	addArgument("--command-fd=0");
	addArgument("--edit-key");
	addArgument(keyid);
	addArgument("passwd");
}

KGpgChangePass::~KGpgChangePass()
{
}

bool
KGpgChangePass::preStart()
{
	setSuccess(TS_MSG_SEQUENCE);

	m_seenold = false;

	return true;
}

bool
KGpgChangePass::nextLine(const QString &line)
{
	if (!line.startsWith(QLatin1String("[GNUPG:] ")))
		return false;

	if (m_seenold && line.contains("keyedit.prompt")) {
		setSuccess(TS_OK);
		write("save");
	} else if (line.contains("GOOD_PASSPHRASE")) {
		setSuccess(TS_MSG_SEQUENCE);
		m_seenold = true;
	} else if (line.contains("passphrase.enter")) {
		QString userIDs(getIdHints());

		if (!m_seenold) {
			if (askPassphrase(i18n("Enter old passphrase for <b>%1</b>", userIDs)))
				setSuccess(TS_USER_ABORTED);
		} else {
			if (sendPassphrase(i18n("<qt>Enter new passphrase for <b>%1</b><br />If you forget this passphrase all your encrypted files and messages will be inaccessible.</qt>", userIDs), true)) {
				setSuccess(TS_USER_ABORTED);
			}
		}
	} else if (line.contains("GET_")) {
		setSuccess(TS_MSG_SEQUENCE);
		return true;
	}

	return false;
}
