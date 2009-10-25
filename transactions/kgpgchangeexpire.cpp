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

#include "kgpgchangeexpire.h"

#include <KLocale>
#include <KMessageBox>

#include "detailedconsole.h"

KGpgChangeExpire::KGpgChangeExpire(QObject *parent, const QString &keyid, const QDate &date)
	: KGpgTransaction(parent)
{
	addArgument("--status-fd=1");
	addArgument("--command-fd=0");
	addArgument("--edit-key");
	addArgument(keyid);
	addArgument("expire");
	
	setDate(date);
}

KGpgChangeExpire::~KGpgChangeExpire()
{
}

bool
KGpgChangeExpire::preStart()
{
	m_step = 3;
	setSuccess(TS_MSG_SEQUENCE);

	return true;
}

bool
KGpgChangeExpire::nextLine(const QString &line)
{
	if (!line.startsWith("[GNUPG:]"))
		return false;

	if (getSuccess() == TS_USER_ABORTED) {
		if (line.contains("GET_" ))
			return true;
	} else if (line.contains("GOOD_PASSPHRASE")) {
		setSuccess(TS_OK);
	} else if (line.contains("keygen.valid")) {
		if (m_date.isNull())
			write("0");
		else
			write(QString::number(QDate::currentDate().daysTo(m_date)).toAscii());
	} else if (line.contains("passphrase.enter")) {
		QString passdlgmessage;
		if (m_step < 3)
			passdlgmessage = i18np("<p><b>Bad passphrase</b>. You have 1 try left.</p>", "<p><b>Bad passphrase</b>. You have %1 tries left.</p>", m_step);
		passdlgmessage += i18n("Enter passphrase for <b>%1</b>", getIdHints());

		if (sendPassphrase(passdlgmessage)) {
			setSuccess(TS_USER_ABORTED);
			return true;
		}
		--m_step;
	} else if ((getSuccess() == TS_OK) && line.contains("keyedit.prompt")) {
		write("save");
	} else if ((getSuccess() == TS_OK) && line.contains("keyedit.save.okay")) {
		write("YES");
	} else if (line.contains("GET_")) {
		if (getSuccess() != TS_BAD_PASSPHRASE)
			setSuccess(TS_MSG_SEQUENCE);
		return true;
	}

	return false;
}

void
KGpgChangeExpire::finish()
{
	if (getSuccess() == TS_MSG_SEQUENCE) {
		QString output;
		KgpgDetailedConsole *q = new KgpgDetailedConsole(0, i18n("<qt><b>Changing expiration failed.</b><br />"
					"Do you want to try changing the key expiration in console mode?</qt>"), output);
		if (q->exec() == QDialog::Accepted) {
			KMessageBox::sorry(0, i18n("Work in progress..."));
			setSuccess(TS_OK);
		} else
			setSuccess(TS_USER_ABORTED);
	}
}

void
KGpgChangeExpire::setDate(const QDate &date)
{
	m_date = date;
}
