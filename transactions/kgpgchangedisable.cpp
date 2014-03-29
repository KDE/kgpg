/*
 * Copyright (C) 2008,2009,2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgchangedisable.h"

KGpgChangeDisable::KGpgChangeDisable(QObject *parent, const QString &keyid, const bool disable)
	: KGpgEditKeyTransaction(parent, keyid, QString(), false)
{
	setDisable(disable);
}

KGpgChangeDisable::~KGpgChangeDisable()
{
}

void
KGpgChangeDisable::setDisable(bool disable)
{
	QString cmd;
	if (disable)
		cmd = QLatin1String( "disable" );
	else
		cmd = QLatin1String( "enable" );

	replaceCommand(cmd);
}

bool
KGpgChangeDisable::preStart()
{
	if (!KGpgEditKeyTransaction::preStart())
		return false;

	setSuccess(TS_OK);

	return true;
}

bool
KGpgChangeDisable::nextLine(const QString &line)
{
	return KGpgEditKeyTransaction::nextLine(line);
}

#include "kgpgchangedisable.moc"
