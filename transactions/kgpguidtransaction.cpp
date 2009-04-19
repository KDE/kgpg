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

#include "kgpguidtransaction.h"

#include "gpgproc.h"

KGpgUidTransaction::KGpgUidTransaction(QObject *parent)
	: KGpgTransaction(parent)
{
	Q_ASSERT(false);
}

KGpgUidTransaction::KGpgUidTransaction(QObject *parent, const QString &keyid, const QString &uid)
	: KGpgTransaction(parent)
{
	addArgument("--status-fd=1");
	addArgument("--command-fd=0");
	addArgument("--edit-key");
	addArgument(keyid);
	addArgument("uid");
	addArgument("-1");

	GPGProc *proc = getProcess();

	QStringList args = proc->program();
	m_uidpos = args.count() - 1;

	setUid(uid);
}

KGpgUidTransaction::~KGpgUidTransaction()
{
}

bool
KGpgUidTransaction::preStart()
{
	setSuccess(TS_MSG_SEQUENCE);

	return true;
}

bool
KGpgUidTransaction::standardCommands(const QString &line)
{
	if (!line.startsWith("[GNUPG:] "))
		return false;

	if (line.contains("GOOD_PASSPHRASE")) {
		setSuccess(TS_MSG_SEQUENCE);
	} else if (line.contains("passphrase.enter")) {
		if (askPassphrase())
			setSuccess(TS_USER_ABORTED);
	} else if (line.contains("keyedit.prompt")) {
		write("save");
		setSuccess(TS_OK);
	} else if (line.contains("GET_")) {
		setSuccess(TS_MSG_SEQUENCE);
		// gpg asks for something unusal, turn to konsole mode
		return true;
	}

	return false;
}

void
KGpgUidTransaction::setUid(const QString &uid)
{
	m_uid = uid;

	GPGProc *proc = getProcess();

	QStringList args = proc->program();
	proc->clearProgram();

	args.replace(m_uidpos, uid);

	proc->setProgram(args);
}

void
KGpgUidTransaction::setUid(const unsigned int uid)
{
	setUid(QString::number(uid));
}
