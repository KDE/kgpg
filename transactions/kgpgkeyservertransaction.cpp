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

#include "kgpgkeyservertransaction.h"

#include "gpgproc.h"

KGpgKeyserverTransaction::KGpgKeyserverTransaction(QObject *parent, const QString &keyserver, const QString &proxy)
	: KGpgTransaction(parent)
{
	addArgument("--status-fd=1");
	addArgument("--command-fd=0");
	addArgument("--keyserver-options");
	m_proxypos = addArgument(QString());
	addArgument("--keyserver");
	m_keyserverpos = addArgument(QString());

	setKeyserver(keyserver);
	setProxy(proxy);
}

KGpgKeyserverTransaction::~KGpgKeyserverTransaction()
{
}

void
KGpgKeyserverTransaction::setKeyserver(const QString &server)
{
	m_keyserver = server;

	replaceArgument(m_keyserverpos, server);
}

void
KGpgKeyserverTransaction::setProxy(const QString &proxy)
{
	m_proxy = proxy;

	replaceArgument(m_proxypos, proxy);
}

void
KGpgKeyserverTransaction::slotAbort()
{
	// no idea if this works on Windows
	getProcess()->terminate();
	setSuccess(TS_USER_ABORTED);
}
