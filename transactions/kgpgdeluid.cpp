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

#include "kgpgdeluid.h"

KGpgDelUid::KGpgDelUid(QObject *parent, const QString &keyid, const QString &uid)
	: KGpgUidTransaction(parent, keyid, uid)
{
	addArgument("deluid");
}

KGpgDelUid::~KGpgDelUid()
{
}

bool
KGpgDelUid::nextLine(const QString &line)
{
	if (line.contains("keyedit.remove.uid.okay")) {
		write("YES");
	} else {
		return standardCommands(line);
	}

	return false;
}
