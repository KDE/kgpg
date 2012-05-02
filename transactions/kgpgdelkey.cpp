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

#include "kgpgdelkey.h"

#include "gpgproc.h"

#include <QString>
#include <QStringList>

KGpgDelKey::KGpgDelKey(QObject *parent, KGpgKeyNode *key)
	: KGpgTransaction(parent)
{
	m_keys << key;
	setCmdLine();
}

KGpgDelKey::KGpgDelKey(QObject *parent, const KGpgKeyNode::List &keys)
	: KGpgTransaction(parent),
	m_keys(keys)
{
	setCmdLine();
}

KGpgDelKey::~KGpgDelKey()
{
}

KGpgKeyNode::List
KGpgDelKey::keys() const
{
	return m_keys;
}

bool
KGpgDelKey::nextLine(const QString &line)
{
	if (!line.startsWith(QLatin1String("[GNUPG:] GOT_IT")))
		setSuccess(KGpgTransaction::TS_MSG_SEQUENCE);

	return false;
}

KGpgTransaction::ts_boolanswer
KGpgDelKey::boolQuestion(const QString &line)
{
	if (line.startsWith(QLatin1String("delete_key.okay")))
		return KGpgTransaction::BA_YES;

	if (line.startsWith(QLatin1String("delete_key.secret.okay")))
		return KGpgTransaction::BA_YES;

	return KGpgTransaction::boolQuestion(line);
}

bool
KGpgDelKey::preStart()
{
	GPGProc *proc = getProcess();
	QStringList args = proc->program();

	foreach (const KGpgKeyNode *key, m_keys)
		args << key->getFingerprint();

	proc->setProgram(args);

	setSuccess(KGpgTransaction::TS_OK);

	return true;
}

void
KGpgDelKey::setCmdLine()
{
	addArgument(QLatin1String( "--status-fd=1" ));
	addArgument(QLatin1String( "--command-fd=0" ));
	addArgument(QLatin1String( "--delete-secret-and-public-key" ));

	m_argscount = getProcess()->program().count();
}

#include "kgpgdelkey.moc"
