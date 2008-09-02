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

void
KGpgChangePass::preStart()
{
	setSuccess(2);
}

/**
 * 0 = success
 * 1 = Bad Passphrase
 * 2 = Unknown error
 * 3 = Aborted
 */
bool
KGpgChangePass::nextLine(const QString &line)
{
	if (!line.startsWith("[GNUPG:] "))
		return false;

	if ((getSuccess() == 4) && line.contains("keyedit.prompt")) {
		setSuccess(0);
		write("save");
	} else if (line.contains("GOOD_PASSPHRASE")) {
		setSuccess(4);
	} else if (line.contains("passphrase.enter")) {
		QString userIDs(getIdHints());

		if (getSuccess() == 1) {
			if (askPassphrase(i18n("Enter old passphrase for <b>%1</b>", userIDs)))
				setSuccess(3);
		} else if (getSuccess() == 4) {
			if (sendPassphrase(i18n("<qt>Enter new passphrase for <b>%1</b><br />If you forget this passphrase all your encrypted files and messages will be inaccessible!</qt>", userIDs))) {
				setSuccess(3);
			}
		}
	} else if (line.contains("GET_")) {
		// gpg asks for something unusal, turn to konsole mode
		return true;
	}

	return false;
}
