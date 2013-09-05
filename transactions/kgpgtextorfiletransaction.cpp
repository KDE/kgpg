/*
 * Copyright (C) 2008,2009,2010,2011,2012,2013 Rolf Eike Beer <kde@opensource.sf-tec.de>
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

#include "gpgproc.h"

#include <KDebug>
#include <KIO/NetAccess>
#include <KLocale>
#include <QRegExp>

KGpgTextOrFileTransaction::KGpgTextOrFileTransaction(QObject *parent, const QString &text, const bool allowChaining)
	: KGpgTransaction(parent, allowChaining)
{
	setText(text);
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

	int begin = text.indexOf(QRegExp(QLatin1String("^(.*\n)?-----BEGIN PGP [A-Z ]*-----\r?\n")));
	if (begin < 0)
		return;

	// find the end of the BEGIN PGP ... line
	static const QChar lf = QLatin1Char('\n');
	begin = text.indexOf(lf, begin);
	Q_ASSERT(begin > 0);

	// now loop until either an empty line is found (end of header) or
	// a line beginning with Charset is found. If the latter, use the
	// charset found there as hint for the following operation
	int nextlf;
	begin++;
	while ((nextlf = text.indexOf(lf, begin)) > 0) {
		static const QChar cr = QLatin1Char('\r');
		if ((nextlf == begin) || ((nextlf == begin + 1) && (text[begin] == cr)))
			break;

		const QString charset = QLatin1String("Charset: ");
		if (text.mid(begin, charset.length()) == charset) {
			QString cs = text.mid(begin + charset.length(), nextlf - begin - charset.length());
			if (!getProcess()->setCodec(cs.toAscii()))
				kDebug(2100) << "unsupported charset found in header" << cs;
			break;
		}
		begin = nextlf + 1;
	}


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

	if (locfiles.isEmpty() && m_tempfiles.isEmpty() && m_text.isEmpty() && !hasInputTransaction()) {
		setSuccess(TS_MSG_SEQUENCE);
		return false;
	}

	QStringList args(QLatin1String("--status-fd=1"));

	args << command();
	// if the input is not stdin set command-fd so GnuPG
	// can ask if e.g. the file already exists
	if (!locfiles.isEmpty() || !m_tempfiles.isEmpty()) {
		args << QLatin1String("--command-fd=0");
		m_closeInput = false;
	} else {
		m_closeInput = !args.contains(QLatin1String("--command-fd=0"));
	}
	if (locfiles.count() + m_tempfiles.count() > 1)
		args << QLatin1String("--multifile");
	args << locfiles << m_tempfiles;
	addArguments(args);

	return true;
}

void
KGpgTextOrFileTransaction::postStart()
{
	if (!m_text.isEmpty()){
		GPGProc *proc = getProcess();
		proc->write(m_text.toUtf8());
		if (m_closeInput)
			proc->closeWriteChannel();
	}
}

bool
KGpgTextOrFileTransaction::nextLine(const QString &line)
{
	m_messages.append(line);

	return false;
}

void
KGpgTextOrFileTransaction::finish()
{
	if (getProcess()->exitCode() != 0) {
		setSuccess(TS_MSG_SEQUENCE);
	}
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

const KUrl::List &
KGpgTextOrFileTransaction::getInputFiles() const
{
	return m_inpfiles;
}

#include "kgpgtextorfiletransaction.moc"
