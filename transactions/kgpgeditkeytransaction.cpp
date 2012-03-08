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

#include "kgpgeditkeytransaction.h"

KGpgEditKeyTransaction::KGpgEditKeyTransaction(QObject *parent, const QString &keyid,
		const QString &command, const bool hasValue, const bool autoSave)
	: KGpgTransaction(parent),
	m_autosave(autoSave),
	m_keyid(keyid)
{
	addArgument(QLatin1String( "--status-fd=1" ));
	addArgument(QLatin1String( "--command-fd=0" ));
	addArgument(QLatin1String( "--edit-key" ));
	addArgument(keyid);

	m_cmdpos = addArgument(command);
	addArgumentRef(&m_cmdpos);

	if (hasValue) {
		m_argpos = addArgument(QString());
		addArgumentRef(&m_argpos);
	} else {
		m_argpos = -1;
	}

	if (autoSave)
		addArgument(QLatin1String( "save" ));
}

KGpgEditKeyTransaction::~KGpgEditKeyTransaction()
{
}

QString
KGpgEditKeyTransaction::getKeyid() const
{
	return m_keyid;
}

bool
KGpgEditKeyTransaction::preStart()
{
	setSuccess(TS_MSG_SEQUENCE);

	return true;
}

bool
KGpgEditKeyTransaction::nextLine(const QString &line)
{
	if (line == QLatin1String("[GNUPG:] GOT_IT")) {
		return false;
	} else if (getSuccess() == TS_USER_ABORTED) {
		if (line.contains(QLatin1String( "GET_" ) ))
			return true;
	} else if ((getSuccess() == TS_OK) && line.contains(QLatin1String( "keyedit.prompt" ))) {
		return true;
	} else if (line.contains(QLatin1String( "NEED_PASSPHRASE" ))) {
		// nothing for now
		// we could use the id from NEED_PASSPHRASE as user id hint ...
	} else {
		if (getSuccess() != TS_BAD_PASSPHRASE)
			setSuccess(TS_MSG_SEQUENCE);
		return true;
	}

	return false;
}

KGpgTransaction::ts_boolanswer
KGpgEditKeyTransaction::boolQuestion(const QString& line)
{
	if ((getSuccess() == TS_OK) && (line == QLatin1String("keyedit.save.okay")) && !m_autosave) {
		return BA_YES;
	} else {
		return KGpgTransaction::boolQuestion(line);
	}
}

void
KGpgEditKeyTransaction::replaceValue(const QString &arg)
{
	Q_ASSERT(m_argpos >= 0);

	replaceArgument(m_argpos, arg);
}

void
KGpgEditKeyTransaction::replaceCommand(const QString &cmd)
{
	replaceArgument(m_cmdpos, cmd);
}

#include "kgpgeditkeytransaction.moc"
