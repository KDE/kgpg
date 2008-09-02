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

#include "kgpgtransaction.h"

#include <QStringList>

#include <KLocale>

#include "gpgproc.h"
#include "kgpginterface.h"

class KGpgTransactionPrivate: QObject {
public:
	KGpgTransactionPrivate(KGpgTransaction *parent);

	KGpgTransaction *m_parent;
	GPGProc *m_process;
	int m_success;
	int m_tries;

	QStringList m_idhints;

	void slotReadReady(GPGProc *gpgProcess);
	void slotProcessExited(GPGProc *gpgProcess);
};

KGpgTransactionPrivate::KGpgTransactionPrivate(KGpgTransaction *parent)
	: QObject(parent)
{
	m_parent = parent;

	m_process = new GPGProc;
	m_success = 0;
}

KGpgTransaction::KGpgTransaction(QObject *parent)
	: QObject(parent), d(new KGpgTransactionPrivate(this))
{
	connect(d->m_process, SIGNAL(readReady(GPGProc *)), SLOT(slotReadReady(GPGProc *)));
	connect(d->m_process, SIGNAL(processExited(GPGProc *)), SLOT(slotProcessExited(GPGProc *)));
}

KGpgTransaction::~KGpgTransaction()
{
	delete d;
}

void
KGpgTransactionPrivate::slotReadReady(GPGProc *gpgProcess)
{
	QString line;

	while (gpgProcess->readln(line, true) >= 0) {
		if (line.startsWith("[GNUPG:] USERID_HINT ")) {
			m_parent->addIdHint(line);
		} else if (m_parent->nextLine(line)) {
			m_process->write("quit\n");
		}
	}
}

void
KGpgTransactionPrivate::slotProcessExited(GPGProc *gpgProcess)
{
	Q_UNUSED(gpgProcess)

	m_parent->finish();
	emit m_parent->done(m_success);
}

void
KGpgTransaction::start()
{
	setSuccess(0);
	d->m_idhints.clear();
	d->m_tries = 3;
	preStart();
	d->m_process->start();
}

void
KGpgTransaction::write(const QByteArray &a)
{
	d->m_process->write(a + '\n');
}

int
KGpgTransaction::sendPassphrase(const QString &text, const bool isnew)
{
	return KgpgInterface::sendPassphrase(text, d->m_process, isnew);
}

int
KGpgTransaction::getSuccess() const
{
	return d->m_success;
}

void
KGpgTransaction::setSuccess(const int &v)
{
	d->m_success = v;
}

void
KGpgTransaction::finish()
{
}

void
KGpgTransaction::preStart()
{
}

void
KGpgTransaction::addIdHint(QString txt)
{
	int cut = txt.indexOf(' ', 22, Qt::CaseInsensitive);
	txt.remove(0, cut);

	if (txt.contains('(', Qt::CaseInsensitive))
		txt = txt.section('(', 0, 0) + txt.section(')', -1);

	txt.replace('<', "&lt;");

	if (!d->m_idhints.contains(txt))
		d->m_idhints << txt;
}

QString
KGpgTransaction::getIdHints() const
{
	return d->m_idhints.join(i18n(" or "));
}

GPGProc *
KGpgTransaction::getProcess()
{
	return d->m_process;
}

void
KGpgTransaction::addArgument(const QString &arg)
{
	*d->m_process << arg;
}

bool
KGpgTransaction::askPassphrase(const QString &message)
{
	QString passdlgmessage;
	QString userIDs(getIdHints());
	if (userIDs.isEmpty())
		userIDs = i18n("[No user id found]");
	else
		userIDs.replace('<', "&lt;");

	if (d->m_tries < 3)
		passdlgmessage = i18np("<p><b>Bad passphrase</b>. You have 1 try left.</p>", "<p><b>Bad passphrase</b>. You have %1 tries left.</p>", d->m_tries);
	if (message.isEmpty()) {
		passdlgmessage += i18n("Enter passphrase for <b>%1</b>", userIDs);
	} else {
		passdlgmessage += message;
	}

	--d->m_tries;

	return sendPassphrase(passdlgmessage, false);
}

#include "kgpgtransaction.moc"
