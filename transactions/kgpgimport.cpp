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

#include "kgpgimport.h"

#include <KIO/NetAccess>
#include <KDebug>
#include <KLocale>

#include "gpgproc.h"

KGpgImport::KGpgImport(QObject *parent, const QString &text)
	: KGpgTransaction(parent), m_text(text)
{
}

KGpgImport::KGpgImport(QObject *parent, const KUrl::List &keys)
	: KGpgTransaction(parent)
{
	setUrls(keys);
}

KGpgImport::~KGpgImport()
{
	cleanUrls();
}

void
KGpgImport::setText(const QString &text)
{
	m_text = text;
	cleanUrls();
}

void
KGpgImport::setUrls(const KUrl::List &keys)
{
	m_text.clear();
	m_inpfiles = keys;
}

bool
KGpgImport::preStart()
{
	m_messages.clear();

	QStringList locfiles;

	foreach (KUrl url, m_inpfiles) {
		if (url.isLocalFile()) {
			locfiles.append(url.toLocalFile());
		} else {
			QString tmpfile;

			if (KIO::NetAccess::download(url, tmpfile, 0)) {
				m_tempfiles.append(tmpfile);
			} else {
				m_messages.append(KIO::NetAccess::lastErrorString());
				cleanUrls();
				setSuccess(TS_IMPORT_KIO_FAILED);
				return false;
			}
		}
	}

	GPGProc *proc = getProcess();
	QStringList args(proc->program().at(0));

	args << "--status-fd=1" << "--import" << "--allow-secret-key-import" << locfiles << m_tempfiles;

	proc->setProgram(args);

	return true;
}

bool
KGpgImport::nextLine(const QString &line)
{
	if (!line.isEmpty())
		m_messages.append(line);

	return false;
}

void
KGpgImport::finish()
{
}

const QStringList &
KGpgImport::getMessages() const
{
	return m_messages;
}

void
KGpgImport::cleanUrls()
{
	foreach (QString u, m_tempfiles)
		KIO::NetAccess::removeTempFile(u);

	m_tempfiles.clear();
	m_locfiles.clear();
	m_inpfiles.clear();
}

QStringList
KGpgImport::getImportedKeys() const
{
	QStringList res;

	foreach (QString str, m_messages)
		if (str.startsWith("[GNUPG:] IMPORTED "))
			res << str.mid(18);

	return res;
}

QStringList
KGpgImport::getImportedIds(const int reason) const
{
	QStringList res;

	foreach (QString str, m_messages) {
		if (!str.startsWith("[GNUPG:] IMPORT_OK "))
			continue;

		QString tmpstr(str.mid(19).simplified());

		int space = tmpstr.indexOf(' ');
		if (space <= 0) {
			kDebug(3125) << __LINE__ << "invalid format:" << str;
			continue;
		}

		bool ok;
		unsigned char code = tmpstr.left(space).toUInt(&ok);
		if (!ok) {
			kDebug(3125) << __LINE__ << "invalid format:" << str << space << tmpstr.left(space - 1);
			continue;
		}

		if ((reason == -1) || ((reason == 0) && (code == 0)) || ((reason & code) != 0))
			res << str.mid(space + 1);
	}

	return res;
}

QString
KGpgImport::getImportMessage() const
{
	foreach (QString str, m_messages) {
		if (!str.startsWith("[GNUPG:] IMPORT_RES "))
			continue;

		const QStringList rstr(str.mid(20).simplified().split(' '));

		bool fine = (rstr.count() == 14);

		unsigned int i = 0;
		unsigned long rcode[14];

		while (fine && (i < 14)) {
			rcode[i] = rstr.at(i).toULong(&fine);
			i++;
		}

		if (!fine)
			return i18n("The import result string has an unsupported format.<br />Please see the detailed log for more information.");

		QString resultMessage(i18np("<qt>%1 key processed.</qt>", "<qt>%1 keys processed.</qt>", rcode[0]));

		if (rcode[1])
			resultMessage += i18np("<qt><br />One key without ID.</qt>", "<qt><br />%1 keys without ID.</qt>", rcode[1]);
		if (rcode[2])
			resultMessage += i18np("<qt><br /><b>One key imported:</b></qt>", "<qt><br /><b>%1 keys imported:</b></qt>", rcode[2]);
		if (rcode[3])
			resultMessage += i18np("<qt><br />One RSA key imported.</qt>", "<qt><br />%1 RSA keys imported.</qt>", rcode[3]);
		if (rcode[4])
			resultMessage += i18np("<qt><br />One key unchanged.</qt>", "<qt><br />%1 keys unchanged.</qt>", rcode[4]);
		if (rcode[5])
			resultMessage += i18np("<qt><br />One user ID imported.</qt>", "<qt><br />%1 user IDs imported.</qt>", rcode[5]);
		if (rcode[6])
			resultMessage += i18np("<qt><br />One subkey imported.</qt>", "<qt><br />%1 subkeys imported.</qt>", rcode[6]);
		if (rcode[7])
			resultMessage += i18np("<qt><br />One signature imported.</qt>", "<qt><br />%1 signatures imported.</qt>", rcode[7]);
		if (rcode[8])
			resultMessage += i18np("<qt><br />One revocation certificate imported.</qt>", "<qt><br />%1 revocation certificates imported.</qt>", rcode[8]);
		if (rcode[9])
			resultMessage += i18np("<qt><br />One secret key processed.</qt>", "<qt><br />%1 secret keys processed.</qt>", rcode[9]);
		if (rcode[10])
			resultMessage += i18np("<qt><br /><b>One secret key imported.</b></qt>", "<qt><br /><b>%1 secret keys imported.</b></qt>", rcode[10]);
		if (rcode[11])
			resultMessage += i18np("<qt><br />One secret key unchanged.</qt>", "<qt><br />%1 secret keys unchanged.</qt>", rcode[11]);
		if (rcode[12])
			resultMessage += i18np("<qt><br />One secret key not imported.</qt>", "<qt><br />%1 secret keys not imported.</qt>", rcode[12]);

		if (rcode[9])
			resultMessage += i18n("<qt><br /><b>You have imported a secret key.</b> <br />"
			"Please note that imported secret keys are not trusted by default.<br />"
			"To fully use this secret key for signing and encryption, you must edit the key (double click on it) and set its trust to Full or Ultimate.</qt>");

		return resultMessage;
	}

	return i18n("No key imported...<br />Check detailed log for more infos");
}
