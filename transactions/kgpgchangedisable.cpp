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

#include "kgpgchangedisable.h"

KGpgChangeDisable::KGpgChangeDisable(QObject *parent, const QString &keyid, const bool &disable)
	: KGpgTransaction(parent)
{
	addArgument("--edit-key");
	addArgument(keyid);
	m_disablepos = addArgument(QString());
	addArgument("save");

	setDisable(disable);
}

KGpgChangeDisable::~KGpgChangeDisable()
{
}

bool
KGpgChangeDisable::nextLine(const QString &line)
{
	Q_UNUSED(line)

	return false;
}

void
KGpgChangeDisable::setDisable(const bool &disable)
{
	m_disable = disable;

	QString cmd;
	if (disable)
		cmd = "disable";
	else
		cmd = "enable";

	replaceArgument(m_disablepos, cmd);
}
