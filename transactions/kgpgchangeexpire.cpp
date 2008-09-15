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
	setSuccess(2);

	return true;
}

/**
 * 0 = success
 * 1 = Bad Passphrase
 * 2 = Unknown error
 * 3 = Aborted
 */
bool
KGpgChangeExpire::nextLine(const QString &line)
{
	if (!line.startsWith("[GNUPG:]"))
		return false;

	if (getSuccess() == 3) {
		if (line.contains("GET_" ))
			return true;
	} else if (line.contains("GOOD_PASSPHRASE")) {
		setSuccess(0);
	} else if (line.contains("keygen.valid")) {
		if (m_date.isNull())
			write("0");
		else
			write(QString::number(QDate::currentDate().daysTo(m_date)).toAscii());
	} else if (line.contains("passphrase.enter")) {
		QString passdlgmessage;
		if (m_step < 3)
			passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", m_step);
		passdlgmessage += i18n("Enter passphrase for <b>%1</b>", getIdHints());

		if (sendPassphrase(passdlgmessage)) {
			setSuccess(3);	// aborted by user mode
			return true;
		}
		--m_step;
	} else if ((getSuccess() == 0) && line.contains("keyedit.prompt")) {
		write("save");
	} else if ((getSuccess() == 0) && line.contains("keyedit.save.okay")) {
		write("YES");
	} else if (line.contains("GET_")) {
		// gpg asks for something unusal, turn to konsole mode
		if (getSuccess() != 1)
			setSuccess(4);	// switching to console mode
		return true;
	}

	return false;
}

void
KGpgChangeExpire::finish()
{
	if (getSuccess() == 4) {
		QString output;
		KgpgDetailedConsole *q = new KgpgDetailedConsole(0, i18n("<qt><b>Changing expiration failed.</b><br />"
					"Do you want to try changing the key expiration in console mode?</qt>"), output);
		if (q->exec() == QDialog::Accepted) {
			KMessageBox::sorry(0, i18n("work in progress..."));
			setSuccess(0);
		} else
			setSuccess(3);
	}
}

void
KGpgChangeExpire::setDate(const QDate &date)
{
	m_date = date;
}
