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

#include "kgpgtransaction.h"

#include "gpgproc.h"
#include "kgpginterface.h"

#include <KDebug>
#include <kio/renamedialog.h>
#include <KPasswordDialog>
#include <knewpassworddialog.h>
#include <KLocale>
#include <KPushButton>
#include <KUrl>
#include <QByteArray>
#include <QPointer>
#include <QStringList>
#include <QWeakPointer>
#include <QWidget>

class KGpgTransactionPrivate {
public:
	KGpgTransactionPrivate(KGpgTransaction *parent, bool allowChaining);
	~KGpgTransactionPrivate();

	KGpgTransaction *m_parent;
	GPGProc *m_process;
	KGpgTransaction *m_inputTransaction;
	KNewPasswordDialog *m_newPasswordDialog;
	KPasswordDialog *m_passwordDialog;
	int m_success;
	int m_tries;
	QString m_description;
	bool m_chainingAllowed;

	QStringList m_idhints;

	KUrl m_overwriteUrl;	///< the file to overwrite or it's new name

	void slotReadReady();
	void slotProcessExited();
	void slotProcessStarted();
	void slotInputTransactionDone(int result);
	void slotPassphraseEntered(const QString &passphrase);
	/**
	 * @brief a slot to handle the case that the passphrase entry was aborted by the user
	 *
	 * This will delete the sender as well as do the internal passphrase aborted handling.
	 */
	void slotPassphraseAborted();
	/**
	 * @brief do the internal passphrase aborted handling
	 */
	void handlePassphraseAborted();

	QList<int *> m_argRefs;
	bool m_inputProcessDone;
	int m_inputProcessResult;
	bool m_ownProcessFinished;

	/**
	 * terminate GnuPG session
	 */
	void sendQuit(void);

	void write(const QByteArray &a);

	static const QStringList &hintNames(void);

private:
	void processDone();

	unsigned int m_quitTries;	///< how many times we tried to quit
	QStringList m_quitLines;	///< what we received after we tried to quit
};

KGpgTransactionPrivate::KGpgTransactionPrivate(KGpgTransaction *parent, bool allowChaining)
	: m_parent(parent),
	m_process(new GPGProc()),
	m_inputTransaction(NULL),
	m_newPasswordDialog(NULL),
	m_passwordDialog(NULL),
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
	if (m_newPasswordDialog) {
		m_newPasswordDialog->close();
		m_newPasswordDialog->deleteLater();
	}
	if (m_process->state() == QProcess::Running) {
		m_process->closeWriteChannel();
		m_process->terminate();
	}
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
	QWeakPointer<GPGProc> process(m_process);
	QWeakPointer<KGpgTransaction> par(m_parent);

	while (!process.isNull() && (m_process->readln(line, true) >= 0)) {
		if (m_quitTries)
			m_quitLines << line;
#ifdef KGPG_DEBUG_TRANSACTIONS
		kDebug(2100) << m_parent << line;
#endif /* KGPG_DEBUG_TRANSACTIONS */

		static const QString getBool = QLatin1String("[GNUPG:] GET_BOOL ");

		if (line.startsWith(QLatin1String("[GNUPG:] USERID_HINT "))) {
			m_parent->addIdHint(line);
		} else if (line.startsWith(QLatin1String("[GNUPG:] BAD_PASSPHRASE "))) {
			// the MISSING_PASSPHRASE line comes first, in that case ignore a
			// following BAD_PASSPHRASE
			if (m_success != KGpgTransaction::TS_USER_ABORTED)
				m_success = KGpgTransaction::TS_BAD_PASSPHRASE;
		} else if (line.startsWith(QLatin1String("[GNUPG:] GET_HIDDEN passphrase.enter"))) {
			const bool goOn = m_parent->passphraseRequested();

			// Check if the object was deleted while waiting for the result
			if (!goOn || par.isNull())
				return;

		} else if (line.startsWith(QLatin1String("[GNUPG:] GOOD_PASSPHRASE"))) {
			emit m_parent->statusMessage(i18n("Got Passphrase"));

			if (m_passwordDialog != NULL) {
				m_passwordDialog->close();
				m_passwordDialog->deleteLater();
				m_passwordDialog = NULL;
			}

			if (m_parent->passphraseReceived()) {
				// signal GnuPG that there will be no further input and it can
				// begin sending output.
				m_process->closeWriteChannel();
			}

		} else if (line.startsWith(getBool)) {
			static const QString overwrite = QLatin1String("openfile.overwrite.okay");
			const QString question = line.mid(getBool.length());

			KGpgTransaction::ts_boolanswer answer;

			if (question.startsWith(overwrite)) {
				m_overwriteUrl.clear();
				answer = m_parent->confirmOverwrite(m_overwriteUrl);

				if ((answer == KGpgTransaction::BA_UNKNOWN) && !m_overwriteUrl.isEmpty()) {
					QPointer<KIO::RenameDialog> over = new KIO::RenameDialog(qobject_cast<QWidget *>(m_parent->parent()),
							i18n("File Already Exists"), KUrl(),
							m_overwriteUrl, KIO::M_OVERWRITE);

					m_overwriteUrl.clear();

					switch (over->exec()) {
					case KIO::R_OVERWRITE:
						answer = KGpgTransaction::BA_YES;
						break;
					case KIO::R_RENAME:
						answer = KGpgTransaction::BA_NO;
						m_overwriteUrl = over->newDestUrl();
						break;
					default:
						answer = KGpgTransaction::BA_UNKNOWN;
						m_parent->setSuccess(KGpgTransaction::TS_USER_ABORTED);
						// Close the pipes, otherwise GnuPG will try to answer
						// further questions about this file.
						m_process->closeWriteChannel();
						m_process->closeReadChannel(QProcess::StandardOutput);
						break;
					}

					delete over;

					if (answer == KGpgTransaction::BA_UNKNOWN)
						continue;
				}
			} else {
				answer = m_parent->boolQuestion(question);
			}

			switch (answer) {
			case KGpgTransaction::BA_YES:
				write("YES\n");
				break;
			case KGpgTransaction::BA_NO:
				write("NO\n");
				break;
			case KGpgTransaction::BA_UNKNOWN:
				m_parent->setSuccess(KGpgTransaction::TS_MSG_SEQUENCE);
				m_parent->unexpectedLine(line);
				sendQuit();
			}
		} else if (!m_overwriteUrl.isEmpty() && line.startsWith(QLatin1String("[GNUPG:] GET_LINE openfile.askoutname"))) {
			write(m_overwriteUrl.toLocalFile().toUtf8() + '\n');
			m_overwriteUrl.clear();
		} else if (line.startsWith(QLatin1String("[GNUPG:] MISSING_PASSPHRASE"))) {
			m_success = KGpgTransaction::TS_USER_ABORTED;
		} else if (line.startsWith(QLatin1String("[GNUPG:] CARDCTRL "))) {
			// just ignore them, pinentry should handle that
		} else {
			// all known hints
			int i = 0;
			bool matched = false;
			foreach (const QString &hintName, hintNames()) {
				const KGpgTransaction::ts_hintType h = static_cast<KGpgTransaction::ts_hintType>(i++);
				if (!line.startsWith(hintName))
					continue;

				matched = true;

				bool r;
				const int skip = hintName.length();
				if (line.length() == skip) {
					r = m_parent->hintLine(h, QString());
				} else {
					r = m_parent->hintLine(h, line.mid(skip + 1).trimmed());
				}

				if (!r) {
					m_parent->setSuccess(KGpgTransaction::TS_MSG_SEQUENCE);
					sendQuit();
				}

				break;
			}

			if (!matched) {
				if (m_parent->nextLine(line))
					sendQuit();
			}
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

#ifdef KGPG_DEBUG_TRANSACTIONS
	if (m_quitTries == 0)
		kDebug(2100) << "sending quit";
#endif /* KGPG_DEBUG_TRANSACTIONS */

	if (m_quitTries++ >= 3) {
		kDebug(2100) << "tried" << m_quitTries << "times to quit the GnuPG session";
		kDebug(2100) << "last input was" << m_quitLines;
		kDebug(2100) << "please file a bug report at https://bugs.kde.org";
		m_process->closeWriteChannel();
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
KGpgTransactionPrivate::slotPassphraseEntered(const QString &passphrase)
{
	// not calling KGpgTransactionPrivate::write() here for obvious privacy reasons
	m_process->write(passphrase.toUtf8() + '\n');
	if (m_parent->sender() == m_newPasswordDialog) {
		m_newPasswordDialog->deleteLater();
		m_newPasswordDialog = NULL;
		m_parent->newPassphraseEntered();
	} else {
		Q_ASSERT(m_parent->sender() == m_passwordDialog);
	}
}

void
KGpgTransactionPrivate::slotPassphraseAborted()
{
	Q_ASSERT((m_parent->sender() == m_passwordDialog) ^ (m_parent->sender() == m_newPasswordDialog));
	m_parent->sender()->deleteLater();
	m_newPasswordDialog = NULL;
	m_passwordDialog = NULL;
	handlePassphraseAborted();
}

void
KGpgTransactionPrivate::handlePassphraseAborted()
{
	// sending "quit" here is useless as it would be interpreted as the passphrase
	m_process->closeWriteChannel();
	m_success = KGpgTransaction::TS_USER_ABORTED;
}

void
KGpgTransactionPrivate::write(const QByteArray &a)
{
	m_process->write(a);
#ifdef KGPG_DEBUG_TRANSACTIONS
	kDebug(2100) << m_parent << a;
#endif /* KGPG_DEBUG_TRANSACTIONS */
}

const QStringList &
KGpgTransactionPrivate::hintNames (void)
{
	static QStringList hints;

	if (hints.isEmpty()) {
		hints.insert(KGpgTransaction::HT_KEYEXPIRED, QLatin1String("[GNUPG:] KEYEXPIRED"));
		hints.insert(KGpgTransaction::HT_SIGEXPIRED, QLatin1String("[GNUPG:] SIGEXPIRED"));
		hints.insert(KGpgTransaction::HT_NOSECKEY,   QLatin1String("[GNUPG:] NO_SECKEY"));
		hints.insert(KGpgTransaction::HT_ENCTO,      QLatin1String("[GNUPG:] ENC_TO"));
	}

	return hints;
}

void
KGpgTransactionPrivate::processDone()
{
	m_parent->finish();
	emit m_parent->infoProgress(100, 100);
	emit m_parent->done(m_success);
#ifdef KGPG_DEBUG_TRANSACTIONS
	kDebug(2100) << this << "result:" << m_success;
#endif /* KGPG_DEBUG_TRANSACTIONS */
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
#ifdef KGPG_DEBUG_TRANSACTIONS
		kDebug(2100) << this << d->m_process->program();
#endif /* KGPG_DEBUG_TRANSACTIONS */
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

void
KGpgTransaction::askNewPassphrase(const QString& text)
{
	emit statusMessage(i18n("Requesting Passphrase"));

	d->m_newPasswordDialog = new KNewPasswordDialog(qobject_cast<QWidget *>(parent()));
	d->m_newPasswordDialog->setPrompt(text);
	d->m_newPasswordDialog->setAllowEmptyPasswords(false);
	connect(d->m_newPasswordDialog, SIGNAL(newPassword(QString)), SLOT(slotPassphraseEntered(QString)));
	connect(d->m_newPasswordDialog, SIGNAL(rejected()), SLOT(slotPassphraseAborted()));
	connect(d->m_process, SIGNAL(processExited()), d->m_newPasswordDialog->button(KDialog::Cancel), SLOT(click()));
	d->m_newPasswordDialog->show();
}

int
KGpgTransaction::getSuccess() const
{
	return d->m_success;
}

void
KGpgTransaction::setSuccess(const int v)
{
#ifdef KGPG_DEBUG_TRANSACTIONS
	kDebug(2100) << "old" << d->m_success << "new" << v;
#endif /* KGPG_DEBUG_TRANSACTIONS */
	d->m_success = v;
}

KGpgTransaction::ts_boolanswer
KGpgTransaction::boolQuestion(const QString& line)
{
	Q_UNUSED(line)

	return BA_UNKNOWN;
}

KGpgTransaction::ts_boolanswer
KGpgTransaction::confirmOverwrite(KUrl &currentFile)
{
	Q_UNUSED(currentFile)

	return BA_UNKNOWN;
}

bool
KGpgTransaction::hintLine(const ts_hintType hint, const QString &args)
{
	switch (hint) {
	case HT_KEYEXPIRED:
		return !args.isEmpty();
	default:
		return true;
	}
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

void
KGpgTransaction::unexpectedLine(const QString &line)
{
	kDebug(2100) << this << "unexpected input line" << line << "for command" << d->m_process->program();
}

bool
KGpgTransaction::passphraseRequested()
{
	return askPassphrase();
}

bool
KGpgTransaction::passphraseReceived()
{
	return true;
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
	emit statusMessage(i18n("Requesting Passphrase"));

	if (d->m_passwordDialog == NULL) {
		d->m_passwordDialog = new KPasswordDialog(qobject_cast<QWidget *>(parent()));

		QString passdlgmessage;
		if (message.isEmpty()) {
			QString userIDs(getIdHints());
			if (userIDs.isEmpty())
				userIDs = i18n("[No user id found]");
			else
				userIDs.replace(QLatin1Char( '<' ), QLatin1String( "&lt;" ));

			passdlgmessage = i18n("Enter passphrase for <b>%1</b>", userIDs);
		} else {
			passdlgmessage = message;
		}

		d->m_passwordDialog->setPrompt(passdlgmessage);

		connect(d->m_passwordDialog, SIGNAL(gotPassword(QString,bool)), SLOT(slotPassphraseEntered(QString)));
		connect(d->m_passwordDialog, SIGNAL(rejected()), SLOT(slotPassphraseAborted()));
		connect(d->m_process, SIGNAL(processExited()), d->m_passwordDialog->button(KDialog::Cancel), SLOT(click()));
	} else {
		// we already have a dialog, so this is a "bad passphrase" situation
		--d->m_tries;

		d->m_passwordDialog->showErrorMessage(i18np("<p><b>Bad passphrase</b>. You have 1 try left.</p>",
				"<p><b>Bad passphrase</b>. You have %1 tries left.</p>", d->m_tries),
				KPasswordDialog::PasswordError);
	}

	d->m_passwordDialog->show();

	return true;
}

void
KGpgTransaction::setGnuPGHome(const QString &home)
{
	QStringList tmp(d->m_process->program());

	Q_ASSERT(tmp.count() > 3);
	int homepos = tmp.indexOf(QLatin1String("--options"), 1);
	if (homepos == -1)
		homepos = tmp.indexOf(QLatin1String("--homedir"), 1);
	Q_ASSERT(homepos != -1);
	Q_ASSERT(homepos + 1 < tmp.count());

	tmp[homepos] = QLatin1String("--homedir");
	tmp[homepos + 1] = home;

	d->m_process->setProgram(tmp);
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

void
KGpgTransaction::kill()
{
	d->m_process->kill();
}

void
KGpgTransaction::newPassphraseEntered()
{
}

#include "kgpgtransaction.moc"
