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
#include <QWidget>

#include <KDebug>
#include <KLocale>

#include "gpgproc.h"
#include "kgpginterface.h"

// define this to get the commands and replies from all GnuPG transactions logged
#undef KGPG_DEBUG_TRANSACTIONS

class KGpgTransactionPrivate: QObject {
public:
	KGpgTransactionPrivate(KGpgTransaction *parent, bool allowChaining);
	~KGpgTransactionPrivate();

	KGpgTransaction *m_parent;
	GPGProc *m_process;
	KGpgTransaction *m_inputTransaction;
	int m_success;
	int m_tries;
	QString m_description;
	bool m_chainingAllowed;

	QStringList m_idhints;

	void slotReadReady();
	void slotProcessExited();
	void slotProcessStarted();
	void slotInputTransactionDone(int result);

	QList<int *> m_argRefs;
	bool m_inputProcessDone;
	int m_inputProcessResult;
	bool m_ownProcessFinished;

	/**
	 * terminate GnuPG session
	 */
	void sendQuit(void);

	void write(const QByteArray &a);

private:
	void processDone();

	unsigned int m_quitTries;	///< how many times we tried to quit
	QStringList m_quitLines;	///< what we received after we tried to quit
};

KGpgTransactionPrivate::KGpgTransactionPrivate(KGpgTransaction *parent, bool allowChaining)
	: QObject(parent),
	m_parent(parent),
	m_process(new GPGProc()),
	m_inputTransaction(NULL),
	m_success(KGpgTransaction::TS_OK),
	m_tries(3),
	m_chainingAllowed(allowChaining),
	m_inputProcessDone(false),
	m_inputProcessResult(KGpgTransaction::TS_OK),
	m_ownProcessFinished(false),
	m_quitTries(0)
{
}

KGpgTransactionPrivate::~KGpgTransactionPrivate()
{
	delete m_inputTransaction;
	delete m_process;
}

KGpgTransaction::KGpgTransaction(QObject *parent, const bool allowChaining)
	: QObject(parent),
	d(new KGpgTransactionPrivate(this, allowChaining))
{
	connect(d->m_process, SIGNAL(readReady()), SLOT(slotReadReady()));
	connect(d->m_process, SIGNAL(processExited()), SLOT(slotProcessExited()));
	connect(d->m_process, SIGNAL(started()), SLOT(slotProcessStarted()));
}

KGpgTransaction::~KGpgTransaction()
{
	delete d;
}

void
KGpgTransactionPrivate::slotReadReady()
{
	QString line;

	while (m_process->readln(line, true) >= 0) {
		if (m_quitTries)
			m_quitLines << line;
#ifdef KGPG_DEBUG_TRANSACTIONS
		kDebug(2100) << line;
#endif /* KGPG_DEBUG_TRANSACTIONS */

		if (line.startsWith(QLatin1String("[GNUPG:] USERID_HINT "))) {
			m_parent->addIdHint(line);
		} else if (line.startsWith(QLatin1String("[GNUPG:] BAD_PASSPHRASE "))) {
			m_success = KGpgTransaction::TS_BAD_PASSPHRASE;
		} else if (line.startsWith(QLatin1String("[GNUPG:] GET_BOOL "))) {
			switch (m_parent->boolQuestion(line.mid(18))) {
			case KGpgTransaction::BA_YES:
				write("YES\n");
				break;
			case KGpgTransaction::BA_NO:
				write("NO\n");
				break;
			case KGpgTransaction::BA_UNKNOWN:
				m_parent->setSuccess(KGpgTransaction::TS_MSG_SEQUENCE);
				kDebug(2100) << "unexpected GnuPG request" << line;
				sendQuit();
			}
		} else if (line.startsWith(QLatin1String("[GNUPG:] CARDCTRL "))) {
			// just ignore them, pinentry should handle that
		} else if (m_parent->nextLine(line)) {
			sendQuit();
		}
	}
}

void
KGpgTransactionPrivate::slotProcessExited()
{
	Q_ASSERT(m_parent->sender() == m_process);
	m_ownProcessFinished = true;

	if (m_inputProcessDone)
		processDone();
}

void
KGpgTransactionPrivate::slotProcessStarted()
{
	m_parent->postStart();
}

void
KGpgTransactionPrivate::sendQuit(void)
{
	write("quit\n");

	if (m_quitTries++ >= 3) {
		kDebug(2100) << "tried" << m_quitTries << "times to quit the GnuPG session";
		kDebug(2100) << "last input was" << m_quitLines;
		kDebug(2100) << "please file a bug report at https://bugs.kde.org";
		m_process->kill();
		m_success = KGpgTransaction::TS_MSG_SEQUENCE;
	}
}

void
KGpgTransactionPrivate::slotInputTransactionDone(int result)
{
	Q_ASSERT(m_parent->sender() == m_inputTransaction);

	m_inputProcessDone = true;
	m_inputProcessResult = result;

	if (m_ownProcessFinished)
		processDone();
}

void
KGpgTransactionPrivate::write(const QByteArray &a)
{
	m_process->write(a);
#ifdef KGPG_DEBUG_TRANSACTIONS
	kDebug(2100) << a;
#endif /* KGPG_DEBUG_TRANSACTIONS */
}

void
KGpgTransactionPrivate::processDone()
{
	m_parent->finish();
	emit m_parent->infoProgress(100, 100);
	emit m_parent->done(m_success);
}

void
KGpgTransaction::start()
{
	d->m_inputProcessResult = false;
	d->m_inputProcessDone = (d->m_inputTransaction == NULL);

	setSuccess(TS_OK);
	d->m_idhints.clear();
	d->m_tries = 3;
	if (preStart()) {
		d->m_ownProcessFinished = false;
		if (d->m_inputTransaction != NULL)
			d->m_inputTransaction->start();
		d->m_process->start();
		emit infoProgress(0, 1);
	} else {
		emit done(d->m_success);
	}
}

void
KGpgTransaction::write(const QByteArray &a, const bool lf)
{
	if (lf)
		d->write(a + '\n');
	else
		d->write(a);
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
	return KgpgInterface::sendPassphrase(text, d->m_process, isnew, qobject_cast<QWidget *>(parent()));
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

KGpgTransaction::ts_boolanswer
KGpgTransaction::boolQuestion(const QString& line)
{
	Q_UNUSED(line);

	return BA_UNKNOWN;
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

void
KGpgTransaction::waitForInputTransaction()
{
	Q_ASSERT(d->m_inputTransaction != NULL);

	if (d->m_inputProcessDone)
		return;

	d->m_inputTransaction->waitForFinished();
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
	int cut = txt.indexOf(QLatin1Char( ' ' ), 22, Qt::CaseInsensitive);
	txt.remove(0, cut);

	if (txt.contains(QLatin1Char( '(' ), Qt::CaseInsensitive))
		txt = txt.section(QLatin1Char( '(' ), 0, 0) + txt.section(QLatin1Char( ')' ), -1);

	txt.replace(QLatin1Char( '<' ), QLatin1String( "&lt;" ));

	if (!d->m_idhints.contains(txt))
		d->m_idhints << txt;
}

QString
KGpgTransaction::getIdHints() const
{
	return d->m_idhints.join( i18n(" or " ));
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
		userIDs.replace(QLatin1Char( '<' ), QLatin1String( "&lt;" ));

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
	d->m_process->setEnv(QLatin1String( "GNUPGHOME" ), home);
}

int
KGpgTransaction::waitForFinished(const int msecs)
{
	int ret = TS_OK;

	if (d->m_inputTransaction != NULL) {
		int ret = d->m_inputTransaction->waitForFinished(msecs);
		if ((ret != TS_OK) && (msecs != -1))
			return ret;
	}

	bool b = d->m_process->waitForFinished(msecs);

	if (ret != TS_OK)
		return ret;

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

void
KGpgTransaction::setInputTransaction(KGpgTransaction *ta)
{
	Q_ASSERT(d->m_chainingAllowed);

	if (d->m_inputTransaction != NULL)
		clearInputTransaction();
	d->m_inputTransaction = ta;

	GPGProc *proc = ta->getProcess();
	proc->setStandardOutputProcess(d->m_process);
	connect(ta, SIGNAL(done(int)), SLOT(slotInputTransactionDone(int)));
}

void
KGpgTransaction::clearInputTransaction()
{
	disconnect(d->m_inputTransaction, SIGNAL(done(int)), this, SLOT(slotInputTransactionDone(int)));
	d->m_inputTransaction = NULL;
}

bool
KGpgTransaction::hasInputTransaction() const
{
	return (d->m_inputTransaction != NULL);
}

void KGpgTransaction::kill()
{
	d->m_process->kill();
}

#include "kgpgtransaction.moc"
