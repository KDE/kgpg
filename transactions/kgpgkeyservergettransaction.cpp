/**
 * Copyright (C) 2009 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "kgpgkeyservergettransaction.h"

#include "gpgproc.h"

KGpgKeyserverGetTransaction::KGpgKeyserverGetTransaction(QObject *parent, const QString &keyserver, const QStringList &keys, const QString &proxy)
	: KGpgKeyserverTransaction(parent, keyserver, proxy)
{
	m_cmdpos = addArgument(QString());
	setKeyIds(keys);
}

KGpgKeyserverGetTransaction::~KGpgKeyserverGetTransaction()
{
}

bool
KGpgKeyserverGetTransaction::preStart()
{
	GPGProc *proc = getProcess();
	QStringList args(proc->program());

	int num = args.count();
	while (num > m_cmdpos)
		args.removeAt(--num);

	args << getGpgCommand() << m_keys;

	proc->setProgram(args);

	setSuccess(TS_MSG_SEQUENCE);

	return true;
}

bool
KGpgKeyserverGetTransaction::nextLine(const QString &line)
{
	if  (line.startsWith("gpgkeys: ")) {
		m_log.append(line.mid(9));
	} else {
		m_log.append(line);
	}
	setSuccess(TS_OK);

	return false;
}

const QStringList &
KGpgKeyserverGetTransaction::getLog() const
{
	return m_log;
}

void
KGpgKeyserverGetTransaction::setKeyIds(const QStringList &keys)
{
	m_keys = keys;
}

KGpgReceiveKeys::KGpgReceiveKeys(QObject *parent, const QString &keyserver, const QStringList &keys, const QString &proxy)
	: KGpgKeyserverGetTransaction(parent, keyserver, keys, proxy)
{
}

KGpgReceiveKeys::~KGpgReceiveKeys()
{
}

QString
KGpgReceiveKeys::getGpgCommand() const
{
	return "--recv-keys";
}

KGpgRefreshKeys::KGpgRefreshKeys(QObject *parent, const QString &keyserver, const QStringList &keys, const QString &proxy)
	: KGpgKeyserverGetTransaction(parent, keyserver, keys, proxy)
{
}

KGpgRefreshKeys::~KGpgRefreshKeys()
{
}

QString
KGpgRefreshKeys::getGpgCommand() const
{
	return "--refresh-keys";
}
