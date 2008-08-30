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

	while (gpgProcess->readln(line, true) >= 0)
		m_parent->nextLine(line);
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

#include "kgpgtransaction.moc"
