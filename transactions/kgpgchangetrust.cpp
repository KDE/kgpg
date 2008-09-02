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

#include "kgpgchangetrust.h"

KGpgChangeTrust::KGpgChangeTrust(QObject *parent, const QString &keyid, const KgpgCore::KgpgKeyOwnerTrust &trust)
	: KGpgTransaction(parent)
{
	addArgument("--status-fd=1");
	addArgument("--command-fd=0");
	addArgument("--edit-key");
	addArgument(keyid);
	addArgument("trust");

	setTrust(trust);
}

KGpgChangeTrust::~KGpgChangeTrust()
{
}

bool
KGpgChangeTrust::nextLine(const QString &line)
{
	if (line.contains("edit_ownertrust.set_ultimate.okay")) {
		write("YES");
	} else if (line.contains("edit_ownertrust.value")) {
		write(QString::number(m_trust).toAscii());
	} else if (line.contains("keyedit.prompt")) {
		write("save");
	} else if (line.contains("GET_")) {
		// gpg asks for something unusal
		setSuccess(1);
		return true;
	}

	return false;
}

void
KGpgChangeTrust::setTrust(const KgpgCore::KgpgKeyOwnerTrust &trust)
{
	m_trust = trust;
}
