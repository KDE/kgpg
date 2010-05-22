/*
 * Copyright (C) 2008,2009,2010 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgtextorfiletransaction.h"

#include <KIO/NetAccess>
#include <KLocale>

#include "gpgproc.h"

KGpgTextOrFileTransaction::KGpgTextOrFileTransaction(QObject *parent, const QString &text, const bool allowChaining)
	: KGpgTransaction(parent, allowChaining),
	m_text(text)
{
}

KGpgTextOrFileTransaction::KGpgTextOrFileTransaction(QObject *parent, const KUrl::List &files, const bool allowChaining)
	: KGpgTransaction(parent, allowChaining)
{
	setUrls(files);
}

KGpgTextOrFileTransaction::~KGpgTextOrFileTransaction()
{
	cleanUrls();
}

void
KGpgTextOrFileTransaction::setText(const QString &text)
{
	m_text = text;
	cleanUrls();
}

void
KGpgTextOrFileTransaction::setUrls(const KUrl::List &files)
{
	m_text.clear();
	m_inpfiles = files;
}

bool
KGpgTextOrFileTransaction::preStart()
{
	m_messages.clear();

	QStringList locfiles;

	foreach (const KUrl &url, m_inpfiles) {
		if (url.isLocalFile()) {
			locfiles.append(url.toLocalFile());
		} else {
			QString tmpfile;

			if (KIO::NetAccess::download(url, tmpfile, 0)) {
				m_tempfiles.append(tmpfile);
			} else {
				m_messages.append(KIO::NetAccess::lastErrorString());
				cleanUrls();
				setSuccess(TS_KIO_FAILED);
				return false;
			}
		}
	}

	if (m_tempfiles.isEmpty() && m_text.isEmpty() && !hasInputTransaction()) {
		setSuccess(TS_MSG_SEQUENCE);
		return false;
	}

	GPGProc *proc = getProcess();
	QStringList args(proc->program().at(0));

	args << "--status-fd=1" << command() << locfiles << m_tempfiles;

	proc->setProgram(args);

	return true;
}

void
KGpgTextOrFileTransaction::postStart()
{
	if (!m_text.isEmpty()){
		GPGProc *proc = getProcess();
		proc->write(m_text.toAscii());
		proc->closeWriteChannel();
	}
}

bool
KGpgTextOrFileTransaction::nextLine(const QString &line)
{
	if (!line.startsWith(QLatin1String("[GNUPG:] SIGEXPIRED")) && !line.startsWith(QLatin1String("[GNUPG:] KEYEXPIRED ")))
		m_messages.append(line);

	return false;
}

void
KGpgTextOrFileTransaction::finish()
{
}

const QStringList &
KGpgTextOrFileTransaction::getMessages() const
{
	return m_messages;
}

void
KGpgTextOrFileTransaction::cleanUrls()
{
	foreach (const QString &u, m_tempfiles)
		KIO::NetAccess::removeTempFile(u);

	m_tempfiles.clear();
	m_locfiles.clear();
	m_inpfiles.clear();
}