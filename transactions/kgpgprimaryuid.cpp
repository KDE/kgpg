/*
 * Copyright (C) 2009,2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgprimaryuid.h"

#include "model/kgpgitemnode.h"

KGpgPrimaryUid::KGpgPrimaryUid(QObject *parent, KGpgUidNode *uid)
	: KGpgUidTransaction(parent, uid->getParentKeyNode()->getId(), uid->getId())
{
	addArgument(QLatin1String( "primary" ));
}

KGpgPrimaryUid::~KGpgPrimaryUid()
{
}

bool
KGpgPrimaryUid::nextLine(const QString &line)
{
	return standardCommands(line);
}

#include "kgpgprimaryuid.moc"
