/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010,2011,2012
 *               Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgtextinterface.h"

#include "kgpginterface.h"
#include "gpgproc.h"
#include "kgpgsettings.h"

#include <KLocale>
#include <QFile>
#include <QString>
#include <QTextCodec>

class KGpgTextInterfacePrivate
{
public:
	KGpgTextInterfacePrivate(QObject *parent);

	GPGProc * const m_process;
	bool m_badpassword;
	bool m_anonymous;
	int m_step;
	QString m_message;
	QString m_signID;
	QStringList m_userIDs;
	QStringList m_gpgopts;
	KUrl m_file;
	KUrl::List m_files;
	KUrl::List m_errfiles;

	void updateIDs(QByteArray txt);
	bool gpgPassphrase();
	void signFile(const KUrl &);
};

KGpgTextInterfacePrivate::KGpgTextInterfacePrivate(QObject *parent)
	: m_process(new GPGProc(parent)),
	m_badpassword(false),
	m_anonymous(false),
	m_step(3)
{
}

void
KGpgTextInterfacePrivate::updateIDs(QByteArray txt)
{
	int cut = txt.indexOf(' ', 22);
	txt.remove(0, cut);

	int pos = txt.indexOf('(');
	if (pos >= 0)
		txt.remove(pos, txt.indexOf(')', pos));

	txt.replace('<', "&lt;");
	QString s = GPGProc::recode(txt);

	if (!m_userIDs.contains(s))
		m_userIDs << s;
}

bool
KGpgTextInterfacePrivate::gpgPassphrase()
{
	QString s;

	if (m_userIDs.isEmpty())
		s = i18n("[No user id found]");
	else
		s = m_userIDs.join( i18n(" or " ));

	QString passdlgmessage;
	if (m_anonymous)
		passdlgmessage = i18n("<p><b>No user id found</b>. Trying all secret keys.</p>");
	if ((m_step < 3) && !m_anonymous)
		passdlgmessage = i18np("<p><b>Bad passphrase</b>. You have 1 try left.</p>",
		                       "<p><b>Bad passphrase</b>. You have %1 tries left.</p>", m_step);
	if (m_userIDs.isEmpty())
		passdlgmessage += i18n("Enter passphrase");
	else
		passdlgmessage += i18n("Enter passphrase for <b>%1</b>", s);

	if (KgpgInterface::sendPassphrase(passdlgmessage, m_process)) {
		m_process->kill();
		return true;
	}

	m_step--;

	return false;
}

void
KGpgTextInterfacePrivate::signFile(const KUrl &file)
{
	*m_process << QLatin1String( "--command-fd=0" ) << QLatin1String( "-u" ) << m_signID;

	*m_process << m_gpgopts;

	if (m_gpgopts.contains(QLatin1String( "--detach-sign" )) && !m_gpgopts.contains( QLatin1String( "--output" )))
		*m_process << QLatin1String( "--output" ) << file.path() + QLatin1String( ".sig" );
	*m_process << file.path();

	m_process->start();
}

KGpgTextInterface::KGpgTextInterface(QObject *parent)
	: QObject(parent),
	d(new KGpgTextInterfacePrivate(parent))
{
}

KGpgTextInterface::~KGpgTextInterface()
{
	delete d->m_process;
	delete d;
}

// signatures
void
KGpgTextInterface::signFiles(const QString &keyID, const KUrl::List &srcUrls, const QStringList &options)
{
	d->m_files = srcUrls;
	d->m_step = 0;
	d->m_signID = keyID;
	d->m_gpgopts = options;

	slotSignFile(0);
}

void
KGpgTextInterface::slotSignFile(int err)
{
	if (err != 0)
		d->m_errfiles << d->m_files.at(d->m_step);

	d->m_process->resetProcess();
	d->signFile(d->m_files.at(d->m_step));

	if (++d->m_step == d->m_files.count()) {
		connect(d->m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotSignFinished(int)));
	} else {
		connect(d->m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotSignFile(int)));
	}
}

void
KGpgTextInterface::slotSignFinished(int err)
{
	if (err != 0)
		d->m_errfiles << d->m_files.at(d->m_step - 1);

	emit fileSignFinished(d->m_errfiles);
}

#include "kgpgtextinterface.moc"
