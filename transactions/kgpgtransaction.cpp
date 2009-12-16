/*
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

#include "kgpgtransaction.h"

#include <QByteArray>
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
	QString m_description;

	QStringList m_idhints;

	void slotReadReady(GPGProc *gpgProcess);
	void slotProcessExited(GPGProc *gpgProcess);
	void slotProcessStarted();

	QList<int *> m_argRefs;
};

KGpgTransactionPrivate::KGpgTransactionPrivate(KGpgTransaction *parent)
	: QObject(parent)
{
	m_parent = parent;

	m_process = new GPGProc;
	m_success = KGpgTransaction::TS_OK;
}

KGpgTransaction::KGpgTransaction(QObject *parent)
	: QObject(parent), d(new KGpgTransactionPrivate(this))
{
	connect(d->m_process, SIGNAL(readReady(GPGProc *)), SLOT(slotReadReady(GPGProc *)));
	connect(d->m_process, SIGNAL(processExited(GPGProc *)), SLOT(slotProcessExited(GPGProc *)));
	connect(d->m_process, SIGNAL(started()), SLOT(slotProcessStarted()));
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
		if (line.startsWith(QLatin1String("[GNUPG:] USERID_HINT "))) {
			m_parent->addIdHint(line);
		} else if (line.startsWith(QLatin1String("[GNUPG:] BAD_PASSPHRASE "))) {
			m_success = KGpgTransaction::TS_BAD_PASSPHRASE;
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
	emit m_parent->infoProgress(100, 100);
	emit m_parent->done(m_success);
}

void
KGpgTransactionPrivate::slotProcessStarted()
{
	m_parent->postStart();
}

void
KGpgTransaction::start()
{
	setSuccess(TS_OK);
	d->m_idhints.clear();
	d->m_tries = 3;
	if (preStart()) {
		d->m_process->start();
		emit infoProgress(0, 1);
	} else {
		emit done(d->m_success);
	}
}

void
KGpgTransaction::write(const QByteArray &a, const bool lf)
{
	d->m_process->write(a);
	if (lf)
		d->m_process->write("\n");
}

void
KGpgTransaction::write(const int i)
{
	write(QByteArray::number(i));
}

int
KGpgTransaction::sendPassphrase(const QString &text, const bool isnew)
{
	emit statusMessage(i18n("Requesting Passphrase"));
	return KgpgInterface::sendPassphrase(text, d->m_process, isnew);
}

int
KGpgTransaction::getSuccess() const
{
	return d->m_success;
}

void
KGpgTransaction::setSuccess(const int v)
{
	d->m_success = v;
}

void
KGpgTransaction::finish()
{
}

void
KGpgTransaction::setDescription(const QString &description)
{
	d->m_description = description;
}

bool
KGpgTransaction::preStart()
{
	return true;
}

void
KGpgTransaction::postStart()
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

int
KGpgTransaction::addArgument(const QString &arg)
{
	int r = d->m_process->program().count();

	*d->m_process << arg;

	return r;
}

int
KGpgTransaction::addArguments(const QStringList &args)
{
	int r = d->m_process->program().count();

	*d->m_process << args;

	return r;
}

void
KGpgTransaction::replaceArgument(const int pos, const QString &arg)
{
	QStringList args(d->m_process->program());
	d->m_process->clearProgram();

	args.replace(pos, arg);

	d->m_process->setProgram(args);
}

void
KGpgTransaction::insertArgument(const int pos, const QString &arg)
{
	insertArguments(pos, QStringList(arg));
}

void
KGpgTransaction::insertArguments(const int pos, const QStringList &args)
{
	QStringList tmp(d->m_process->program());

	int tmppos = pos;
	foreach (const QString &s, args) {
		tmp.insert(tmppos++, s);
	}
	d->m_process->setProgram(tmp);

	int move = args.count();
	foreach (int *ref, d->m_argRefs) {
		if (*ref >= pos)
			*ref += move;
	}
}

void
KGpgTransaction::addArgumentRef(int *ref)
{
	d->m_argRefs.append(ref);
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

void
KGpgTransaction::setGnuPGHome(const QString &home)
{
	d->m_process->setEnv("GNUPGHOME", home);
}

int
KGpgTransaction::waitForFinished(const int msecs)
{
	bool b = d->m_process->waitForFinished(msecs);

	if (!b)
		return TS_USER_ABORTED;
	else
		return getSuccess();
}

const QString &
KGpgTransaction::getDescription() const
{
	return d->m_description;
}

#include "kgpgtransaction.moc"
