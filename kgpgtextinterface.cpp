/**
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008 Rolf Eike Beer <kde@opensource.sf-tec.de>
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

#include <QTextCodec>
#include <QFile>

#include <KLocale>
#include <KDebug>
#include <KMessageBox>

#include "kgpginterface.h"
#include "gpgproc.h"
#include "detailedconsole.h"

class KGpgTextInterfacePrivate
{
public:
	KGpgTextInterfacePrivate();

	GPGProc *m_process;
	bool m_ok;
	bool m_badmdc;
	bool m_badpassword;
	bool m_anonymous;
	bool m_signmiss;
	bool m_forcefail;
	int m_step;
	int m_textlength;
	QString m_message;
	QString m_signID;
	QString m_log;
	QStringList m_userIDs;
	QStringList m_gpgopts;
	QByteArray m_tmpmessage;
	QByteArray m_readin;
	KUrl m_file;
	KUrl m_dest;
	KUrl::List m_files;
	KUrl::List m_errfiles;

	void updateIDs(QByteArray txt);
	bool symPassphrase();
	bool gpgPassphrase();
	void signFile(const KUrl &);
};

KGpgTextInterfacePrivate::KGpgTextInterfacePrivate()
{
	m_anonymous = false;
	m_badmdc = false;
	m_ok = false;
	m_textlength = -1;
	m_step = 3;
	m_badpassword = false;
	m_signmiss = false;
	m_forcefail = false;
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
KGpgTextInterfacePrivate::symPassphrase()
{
	if (KgpgInterface::sendPassphrase(i18n("Enter passphrase (symmetrical encryption)"), m_process)) {
		m_process->kill();
		return true;
	} else {
		return false;
	}
}

bool
KGpgTextInterfacePrivate::gpgPassphrase()
{
	QString s;

	if (m_userIDs.isEmpty())
		s = i18n("[No user id found]");
	else
		s = m_userIDs.join(i18n(" or "));

	QString passdlgmessage;
	if (m_anonymous)
		passdlgmessage = i18n("<p><b>No user id found</b>. Trying all secret keys.</p>");
	if ((m_step < 3) && !m_anonymous)
		passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", m_step);
	if (m_userIDs.isEmpty())
		passdlgmessage += i18n("Enter passphrase");
	else
		passdlgmessage += i18n("Enter passphrase for <b>%1</b>", s);

	if (KgpgInterface::sendPassphrase(passdlgmessage, m_process, false)) {
		m_process->kill();
		return true;
	}

	m_step--;

	return false;
}

void
KGpgTextInterfacePrivate::signFile(const KUrl &file)
{
	*m_process << "--command-fd=0" << "-u" << m_signID.simplified().toLocal8Bit();

	for (QStringList::ConstIterator it = m_gpgopts.begin(); it != m_gpgopts.end(); ++it)
		*m_process << QFile::encodeName(*it);

	*m_process << "--output" << QFile::encodeName(file.path() + ".sig");
	*m_process << "--detach-sig" << QFile::encodeName(file.path());

	m_process->start();
}

KGpgTextInterface::KGpgTextInterface(QObject *parent)
	: QObject(parent), d(new KGpgTextInterfacePrivate)
{
	d->m_process = new GPGProc(this);
	*d->m_process << "--status-fd=1" << "--command-fd=0";

}

KGpgTextInterface::~KGpgTextInterface()
{
	delete d->m_process;
	delete d;
}

void
KGpgTextInterface::encryptText(const QString &text, const QStringList &userids, const QStringList &options)
{
	QTextCodec *codec = QTextCodec::codecForLocale();
	if (codec->canEncode(text))
		d->m_message = text;
	else
		d->m_message = text.toUtf8();

	for (QStringList::ConstIterator it = options.begin(); it != options.end(); ++it)
		*d->m_process << QFile::encodeName(*it);

	if (userids.isEmpty()) {
		*d->m_process << "-c";
	} else {
		*d->m_process << "-e";
		for (QStringList::ConstIterator it = userids.begin(); it != userids.end(); ++it)
			*d->m_process << "--recipient" << *it;
	}

	connect(d->m_process, SIGNAL(readReady(GPGProc *)), this, SLOT(encryptTextProcess()));
	connect(d->m_process, SIGNAL(processExited(GPGProc *)), this, SLOT(encryptTextFin()));
	d->m_process->start();
}

void
KGpgTextInterface::encryptTextProcess()
{
	int items;
	QString line;

	while ( (items = d->m_process->readln(line)) >= 0 ) {
		if (line.startsWith("[GNUPG:] ")) {
			line.remove(0, 9);
			if (line.startsWith("BEGIN_ENCRYPTION")) {
				d->m_process->write(d->m_message.toAscii());
				d->m_process->closeWriteChannel();
				d->m_message.clear();
			} else if (line.contains("passphrase.enter")) {
				if (d->symPassphrase()) {
					d->m_message.clear();
					return;
				}
			}
		} else {
			d->m_message += line + '\n';
		}
	}
}

void
KGpgTextInterface::encryptTextFin()
{
	emit txtEncryptionFinished(QString::fromUtf8(d->m_message.toAscii()).trimmed(), this);
}

void
KGpgTextInterface::decryptText(const QString &text, const QStringList &options)
{
	for (QStringList::ConstIterator it = options.begin(); it != options.end(); ++it)
		*d->m_process << QFile::encodeName(*it);

	*d->m_process << "-d";

	d->m_process->setOutputChannelMode(KProcess::SeparateChannels);
	connect(d->m_process, SIGNAL(readyReadStandardError()), this, SLOT(decryptTextStdErr()));
	connect(d->m_process, SIGNAL(lineReadyStandardOutput()), this, SLOT(decryptTextStdOut()));
	connect(d->m_process, SIGNAL(processExited(GPGProc *)), this, SLOT(decryptTextFin()));
	d->m_process->start();

	d->m_process->write(text.toAscii());
}

void
KGpgTextInterface::decryptTextStdErr()
{
	QByteArray a;

	while (d->m_process->readLineStandardError(&a))
		d->m_log.append(a + '\n');
}

void
KGpgTextInterface::decryptTextStdOut()
{
	QByteArray line;

	while (!line.isEmpty() || d->m_process->readLineStandardOutput(&line)) {
		if (line.startsWith("[GNUPG:] ")) {
			line.remove(0, 9);
			if (line.startsWith("USERID_HINT")) {
				d->updateIDs(line);
			} else if (line.startsWith("ENC_TO")) {
				if (line.contains("0000000000000000"))
					d->m_anonymous = true;
			} else if (line.startsWith("GET_")) {
				if ((line.contains("passphrase.enter"))) {
					if (d->gpgPassphrase())
						return;
				} else {
					d->m_process->write("quit\n");
				}
			} else if (line.startsWith("BEGIN_DECRYPTION")) {
				d->m_process->closeWriteChannel();
			} else if (line.startsWith("PLAINTEXT_LENGTH")) {
				d->m_textlength = line.mid(line.indexOf("PLAINTEXT_LENGTH") + 16).toInt();
			} else if (line.startsWith("DECRYPTION_OKAY")) {
				d->m_ok = true;
			} else if (line.startsWith("BADMDC")) {
				d->m_badmdc = true;
			} else if (line.startsWith("BAD_PASSPHRASE")) {
				d->m_step--;
			} else if (line.startsWith("END_DECRYPTION")) {
				// workaround for GnuPG weirdness: DECRYPTION_OKAY is
				// not written if message is signed *and* encrypted
				if (!d->m_forcefail)
					d->m_ok = true;
			} else if (line.startsWith("DECRYPTION_FAILED")) {
				d->m_ok = false;
				d->m_forcefail = true;
			}
			line.clear();
		} else {
			if (d->m_textlength == -1) {
				d->m_tmpmessage.append(line + '\n');
				line.clear();
			} else if (d->m_textlength < line.length()) {
				d->m_tmpmessage.append(line.left(d->m_textlength));
				line.remove(0, d->m_textlength);
				d->m_textlength = 0;
			} else {
				d->m_tmpmessage.append(line + '\n');
				d->m_textlength -= line.length() + 1;
				line.clear();
			}
		}
	}
}

void
KGpgTextInterface::decryptTextFin()
{
	if ((d->m_ok) && (!d->m_badmdc)) {
		emit txtDecryptionFinished(d->m_tmpmessage, this);
	} else if (d->m_badmdc) {
		KMessageBox::sorry(0, i18n("Bad MDC detected. The encrypted text has been manipulated."));
		emit txtDecryptionFailed(d->m_log, this);
	} else
		emit txtDecryptionFailed(d->m_log, this);
}

void
KGpgTextInterface::signText(const QString &text, const QString &userid, const QStringList &options)
{
	QTextCodec *codec = QTextCodec::codecForLocale();
	if (codec->canEncode(text))
		d->m_message = text;
	else
		d->m_message = text.toUtf8();

	for (QStringList::ConstIterator it = options.begin(); it != options.end(); ++it)
		*d->m_process << QFile::encodeName(*it);

	*d->m_process << "--clearsign" << "-u" << userid;

	connect(d->m_process, SIGNAL(lineReadyStandardOutput()), this, SLOT(signTextProcess()));
	connect(d->m_process, SIGNAL(processExited(GPGProc *)), this, SLOT(signTextFin()));
	d->m_process->start();
}

void
KGpgTextInterface::signTextProcess()
{
	QByteArray line;

	while (d->m_process->readLineStandardOutput(&line)) {
		if (line.startsWith("[GNUPG:]")) {
			if (line.contains("USERID_HINT")) {
				d->updateIDs(line);
			} else if (line.contains("BAD_PASSPHRASE")) {
				d->m_message.fill('x');
				d->m_message.clear();
				d->m_badpassword = true;
			} else if (line.contains("GOOD_PASSPHRASE")) {
				d->m_process->write(d->m_message.toAscii());
				d->m_process->closeWriteChannel();
				d->m_message.clear();
			} else if (line.contains("passphrase.enter")) {
				if (d->gpgPassphrase())
					return;
			}
		} else 
			d->m_message += line + '\n';
	}
}

void
KGpgTextInterface::signTextFin()
{
	if (d->m_badpassword) {
		emit txtSigningFailed(d->m_message, this);
		d->m_message.clear();
	} else if (!d->m_message.isEmpty()) {
		emit txtSigningFinished(d->m_message.trimmed(), this);
	} else {
		emit txtSigningFinished(QString(), this);
	}
}

void
KGpgTextInterface::verifyText(const QString &text)
{
	QTextCodec *codec = QTextCodec::codecForLocale();
	if (codec->canEncode(text))
		d->m_message = text;
	else
		d->m_message = text.toUtf8();

	*d->m_process << "--verify";

	connect(d->m_process, SIGNAL(readReady(GPGProc *)), this, SLOT(readVerify()));
	connect(d->m_process, SIGNAL(processExited(GPGProc *)), this, SLOT(verifyTextFin()));
	d->m_process->start();

	d->m_process->write(d->m_message.toAscii());
	d->m_message.clear();
	d->m_process->closeWriteChannel();
}

void
KGpgTextInterface::verifyTextFin()
{
	if (d->m_signmiss) {
		emit txtVerifyMissingSignature(d->m_signID, this);
	} else {
		if (d->m_signID.isEmpty())
		d->m_signID = i18n("No signature found.");

		emit txtVerifyFinished(d->m_signID, d->m_message, this);
	}
}

void
KGpgTextInterface::encryptFile(const QStringList &encryptkeys, const KUrl &srcurl, const KUrl &desturl, const QStringList &options, const bool &symetrical)
{
	d->m_file = srcurl;

	for (QStringList::ConstIterator it = options.begin(); it != options.end(); ++it)
		*d->m_process << QFile::encodeName(*it);

	*d->m_process << "--output" << QFile::encodeName(desturl.path());

	if (!symetrical) {
		*d->m_process << "-e";
		for (QStringList::ConstIterator it = encryptkeys.begin(); it != encryptkeys.end(); ++it)
		*d->m_process << "--recipient" << *it;
	} else
		*d->m_process << "-c";

	*d->m_process << QFile::encodeName(srcurl.path());

	connect(d->m_process, SIGNAL(readReady(GPGProc *)), this, SLOT(fileReadEncProcess()));
	connect(d->m_process, SIGNAL(processExited(GPGProc *)), this, SLOT(fileEncryptFin()));
	d->m_process->start();
}

// TODO : there is a bug when we want to encrypt a file
void
KGpgTextInterface::fileReadEncProcess()
{
	QString line;

	while (d->m_process->readln(line) >= 0) {
		kDebug(2100) << line ;
		if (line.startsWith("[GNUPG:]")) {
			if (line.contains("BEGIN_ENCRYPTION")) {
				emit processstarted(d->m_file.path());
			} else if (line.contains("GET_" )) {
				if (line.contains("openfile.overwrite.okay")) {
					d->m_process->write("Yes\n");
				} else if (line.contains("passphrase.enter")) {
					if (d->symPassphrase())
						return;
				} else {
					d->m_process->write("quit\n");
				}
			} else if (line.contains("END_ENCRYPTION")) {
				d->m_ok = true;
			} else {
				d->m_message += line + '\n';
			}
		} else {
			d->m_message += line + '\n';
		}
	}
}

void
KGpgTextInterface::fileEncryptFin()
{
	if (d->m_ok)
		emit fileEncryptionFinished(d->m_file, this);
	else
		emit errorMessage(d->m_message, this);
}

// decrypt file to text
void
KGpgTextInterface::KgpgDecryptFileToText(const KUrl &srcUrl, const QStringList &options)
{
	*d->m_process << "--no-batch" << "-o" << "-";

	for (QStringList::ConstIterator it = options.begin(); it != options.end(); ++it)
		*d->m_process << QFile::encodeName(*it);

	*d->m_process << "-d" << QFile::encodeName(srcUrl.path());

	d->m_process->setOutputChannelMode(KProcess::SeparateChannels);
	connect(d->m_process, SIGNAL(readyReadStandardError()), this, SLOT(decryptTextStdErr()));
	connect(d->m_process, SIGNAL(readReady(GPGProc *)), this, SLOT(decryptTextStdOut()));
	connect(d->m_process, SIGNAL(processExited(GPGProc *)), this, SLOT(decryptTextFin()));
	d->m_process->start();
}

void
KGpgTextInterface::decryptFile(const KUrl &src, const KUrl &dest, const QStringList &options)
{
	d->m_file = src;

	*d->m_process << "--no-verbose" << "--no-greeting";

	*d->m_process << options;

	d->m_dest = dest;
	if (!dest.fileName().isEmpty())
		*d->m_process << "-o" << dest.path();
	*d->m_process << "-d" << src.path();

	connect(d->m_process, SIGNAL(lineReadyStandardOutput()), this, SLOT(decryptFileProcess()));
	connect(d->m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(decryptFileFin(int, QProcess::ExitStatus)));
	d->m_process->start();
}

void
KGpgTextInterface::decryptFileProcess()
{
	QByteArray line;

	while (d->m_process->readLineStandardOutput(&line)) {
		if (line.startsWith("[GNUPG:] ")) {
			line.remove(0, 9);
			if (line.startsWith("BEGIN_DECRYPTION")) {
				emit decryptFileStarted(d->m_file);
			} else if (line.startsWith("USERID_HINT")) {
				d->updateIDs(line);
			} else if (line.startsWith("ENC_TO")) {
				if (line.contains("0000000000000000"))
					d->m_anonymous = true;
			} else if (line.startsWith("GET_")) {
				if (line.contains("openfile.overwrite.okay")) {
					d->m_process->write("Yes\n");
				} else if ((line.contains("passphrase.enter"))) {
					if (d->gpgPassphrase()) {
						emit decryptFileFinished(1, this);
						return;
					}
				} else {
					d->m_process->write("quit\n");
				}
			} else {
				d->m_message += line + '\n';
			}
		}
	}
}

void
KGpgTextInterface::decryptFileFin(int res, QProcess::ExitStatus status)
{
	if (status == QProcess::CrashExit) {
		if (d->m_dest.isLocalFile())
			QFile(d->m_dest.toLocalFile()).remove();
		emit decryptFileFinished(2, this);
		return;
	}

	if (res != 0) {
		if (d->m_dest.isLocalFile())
			QFile(d->m_dest.toLocalFile()).remove();
		emit decryptFileFinished(3, this);
		return;
	}

	if (d->m_message.contains("DECRYPTION_OKAY") && d->m_message.contains("END_DECRYPTION"))
		emit decryptFileFinished(0, this);
	else
		emit decryptFileFinished(4, this);
}

void
KGpgTextInterface::KgpgVerifyFile(const KUrl &sigUrl, const KUrl &srcUrl)
{
	d->m_file = sigUrl;

	*d->m_process << "--verify";
	if (!srcUrl.isEmpty())
		*d->m_process << QFile::encodeName(srcUrl.path());
	*d->m_process << QFile::encodeName(sigUrl.path());

	connect(d->m_process, SIGNAL(lineReadyStandardOutput()), this, SLOT(readVerify()));
	connect(d->m_process, SIGNAL(processExited(GPGProc *)), this, SLOT(verifyfin()));
	d->m_process->start();
}

void
KGpgTextInterface::readVerify()
{
	QByteArray line;

	while (d->m_process->readLineStandardOutput(&line)) {
		d->m_message += GPGProc::recode(line) + '\n';
		if (line.contains("GET_"))
			d->m_process->write("quit\n");

		if (!line.startsWith("[GNUPG:] "))
			continue;
		line.remove(0, 9);
		if (line.startsWith("UNEXPECTED") || line.startsWith("NODATA")) {
			d->m_signID = i18n("No signature found.");
		} else if (line.startsWith("GOODSIG")) {
			int sigpos = line.indexOf(' ', 8);
			d->m_signID = i18n("<qt>Good signature from:<br /><b>%1</b><br />Key ID: %2<br /></qt>",
					GPGProc::recode(line.mid(sigpos + 1).replace('<', "&lt;")),
					QString::fromAscii(line.mid(8, sigpos - 8)));
		} else if (line.startsWith("BADSIG")) {
			int sigpos = line.indexOf(' ', 7);
			d->m_signID = i18n("<qt><b>BAD signature</b> from:<br /> %1<br />Key id: %2<br /><br /><b>The file is corrupted</b><br /></qt>",
					GPGProc::recode(line.mid(sigpos + 1).replace('<', "&lt;")),
					QString::fromAscii(line.mid(7, sigpos - 7)));
		} else if (line.startsWith("NO_PUBKEY")) {
			d->m_signmiss = true;
			d->m_signID = line.remove(0, line.indexOf(' '));
		} else  if (line.startsWith("TRUST_UNDEFINED")) {
			d->m_signID += i18n("<qt>The signature is valid, but the key is untrusted<br /></qt>");
		} else if (line.startsWith("TRUST_ULTIMATE")) {
			d->m_signID += i18n("<qt>The signature is valid, and the key is ultimately trusted<br /></qt>");
		}
	}
}

void
KGpgTextInterface::verifyfin()
{
	if (!d->m_signmiss) {
		if (d->m_signID.isEmpty())
			d->m_signID = i18n("No signature found.");

		(void) new KgpgDetailedInfo(0, d->m_signID, d->m_message);
	} else {
		if (KMessageBox::questionYesNo(0,
					i18n("<qt><b>Missing signature:</b><br />Key id: %1<br /><br />Do you want to import this key from a keyserver?</qt>", d->m_signID),
					d->m_file.fileName(), KGuiItem(i18n("Import")), KGuiItem(i18n("Do Not Import"))) == KMessageBox::Yes)
			emit verifyquerykey(d->m_signID);
	}
	emit verifyfinished();
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
KGpgTextInterface::signFilesBlocking(const QString &keyID, const KUrl::List &srcUrls, const QStringList &options)
{
	d->m_signID = keyID;
	d->m_gpgopts = options;

	for (int i = 0; i < srcUrls.count(); i++) {
		d->signFile(srcUrls.at(i));
		d->m_process->waitForFinished(-1);
		d->m_process->resetProcess();
	}
}
void
KGpgTextInterface::slotSignFile(int err)
{
	if (err != 0)
		d->m_errfiles << d->m_files.at(d->m_step);

	d->m_process->resetProcess();
	d->signFile(d->m_files.at(d->m_step));

	if (++d->m_step == d->m_files.count()) {
		connect(d->m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotSignFinished(int)));
	} else {
		connect(d->m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotSignFile(int)));
	}

	d->m_process->start();
}

void
KGpgTextInterface::slotSignFinished(int err)
{
	if (err != 0)
		d->m_errfiles << d->m_files.at(d->m_step - 1);

	emit fileSignFinished(this, d->m_errfiles);
}
