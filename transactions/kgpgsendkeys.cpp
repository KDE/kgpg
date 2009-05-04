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

#include "kgpgsendkeys.h"

#include "gpgproc.h"

KGpgSendKeys::KGpgSendKeys(QObject *parent, const QString &keyserver, const QStringList &keys, const QString &attropt, const bool withProgress, const QString &proxy)
	: KGpgKeyserverTransaction(parent, keyserver, withProgress, proxy)
{
	addArgument("--export-options");
	m_attrpos = addArgument(QString());
	addArgument("--send-keys");
	setAttributeOptions(attropt);
	setKeyIds(keys);

	getProcess()->setOutputChannelMode(KProcess::MergedChannels);
}

KGpgSendKeys::~KGpgSendKeys()
{
}

bool
KGpgSendKeys::preStart()
{
	GPGProc *proc = getProcess();
	QStringList args(proc->program());

	int num = args.count();
	while (num > m_attrpos + 2)
		args.removeAt(--num);

	args << m_keys;

	proc->setProgram(args);

	setSuccess(TS_MSG_SEQUENCE);

	return KGpgKeyserverTransaction::preStart();
}

bool
KGpgSendKeys::nextLine(const QString &line)
{
	m_log.append(line);
	setSuccess(TS_OK);

	return false;
}

const QStringList &
KGpgSendKeys::getLog() const
{
	return m_log;
}

void
KGpgSendKeys::setKeyIds(const QStringList &keys)
{
	m_keys = keys;
}

void
KGpgSendKeys::setAttributeOptions(const QString &opt)
{
	if (opt.isEmpty())
		m_attributeopt = "no-export-attributes";
	else
		m_attributeopt = opt;

	replaceArgument(m_attrpos, m_attributeopt);
}
