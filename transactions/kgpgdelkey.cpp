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

#include "kgpgdelkey.h"
#include <QString>
#include <QStringList>
#include "gpgproc.h"
#include <KDebug>

KGpgDelKey::KGpgDelKey(QObject *parent, const QString &keyid)
	: KGpgTransaction(parent)
{
	setCmdLine();

	setDelKey(keyid);
}

KGpgDelKey::KGpgDelKey(QObject *parent, const QStringList &keyids)
: KGpgTransaction(parent)
{
	setCmdLine();
	
	setDelKeys(keyids);
}

KGpgDelKey::~KGpgDelKey()
{
}

void
KGpgDelKey::setDelKey(const QString &keyid)
{
	m_keyids << keyid;
}

void
KGpgDelKey::setDelKeys(const QStringList &keyids)
{
	m_keyids << keyids;
}

bool
KGpgDelKey::nextLine(const QString &line)
{
	Q_UNUSED(line);

	return false;
}

bool
KGpgDelKey::preStart()
{
	GPGProc *proc = getProcess();
	QStringList args = proc->program();

	int num = args.count();
	while (num > m_argscount)
		args.removeAt(--num);

	args << m_keyids;

	proc->setProgram(args);

	return true;
}

void
KGpgDelKey::finish()
{
	m_keyids.clear();
}

void
KGpgDelKey::setCmdLine()
{
	addArgument("--status-fd=1");
	addArgument("--command-fd=0");
	addArgument("--yes");
	addArgument("--batch");
	addArgument("--delete-secret-and-public-key");

	m_argscount = getProcess()->program().count();
}
