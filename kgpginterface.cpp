/***************************************************************************
                          kgpginterface.cpp  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright          : (C) 2002 by Jean-Baptiste Mardelle
    email                : bj@altern.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpginterface.h"

#include <QDir>
#include <QTextStream>
#include <QFile>

#include <kio/netaccess.h>
#include <KMessageBox>
#include <KPasswordDialog>
#include <knewpassworddialog.h>
#include <KLocale>
#include <KProcess>
#include <KConfig>
#include <KDebug>
#include <KGlobal>
#include <KStandardDirs>
#include <KUrl>

#include "detailedconsole.h"
#include "kgpgsettings.h"
#include "convert.h"
#include "gpgproc.h"

using namespace KgpgCore;

KgpgInterface::KgpgInterface()
{
}

KgpgInterface::~KgpgInterface()
{
}

QString KgpgInterface::checkForUtf8(QString txt)
{
	/* The string is not in UTF-8 */
	if (!txt.contains("\\x"))
		return txt;

	// if (!strchr (txt.toAscii(), 0xc3) || (txt.find("\\x")!=-1)) {
	for (int idx = 0; (idx = txt.indexOf( "\\x", idx )) >= 0 ; ++idx) {
		char str[2] = "x";
		str[0] = (char) QString(txt.mid(idx + 2, 2)).toShort(0, 16);
		txt.replace(idx, 4, str);
	}

	return QString::fromUtf8(txt.toAscii());
}

QString KgpgInterface::checkForUtf8bis(QString txt)
{
	if (strchr(txt.toAscii(), 0xc3) || txt.contains("\\x")) {
		txt = checkForUtf8(txt);
	} else {
		txt = checkForUtf8(txt);
		txt = QString::fromUtf8(txt.toAscii());
	}
	return txt;
}

int KgpgInterface::gpgVersion(const QString &vstr)
{
	if (vstr.isEmpty())
		return -1;

	QStringList values(vstr.split('.'));
	if (values.count() < 3)
		return -2;

	return (0x10000 * values[0].toInt() + 0x100 * values[1].toInt() + values[2].toInt());
}

QString KgpgInterface::gpgVersionString(const QString &binary)
{
	GPGProc process(0, binary);
	process << "--version";
	process.start();
	process.waitForFinished(-1);

	if (process.exitCode() == 255)
		return QString();

	QString line;
	if (process.readln(line) != -1)
		return line.simplified().section(' ', -1);
	else
		return QString();
}

QString KgpgInterface::getGpgProcessHome(const QString &binary)
{
	GPGProc process(0, binary);
	process << "--version";
	process.start();
	process.waitForFinished(-1);

	if (process.exitCode() == 255) {
		return QString();
	}

	QString line;
	while (process.readln(line) != -1) {
		if (line.startsWith("Home: ")) {
			line.remove(0, 6);
			return line;
		}
	}

	return QString();
}

QString KgpgInterface::getGpgHome(const QString &binary)
{
	// First try: if environment is set GnuPG will use that directory
	// We can use this directly without starting a new process
	QByteArray env(qgetenv("GNUPGHOME"));
	QString gpgHome;
	if (!env.isEmpty()) {
		gpgHome = env;
	} else if (!binary.isEmpty()) {
		// Second try: start GnuPG and ask what it is
		gpgHome = getGpgProcessHome(binary);
	}

	// Third try: guess what it is.
	if (gpgHome.isEmpty()) {
#ifdef Q_OS_WIN32	//krazy:exclude=cpp
		gpgHome = qgetenv("APPDATA") + "/gnupg/";
		gpgHome.replace('\\', '/');
#else
		gpgHome = QDir::homePath() + "/.gnupg/";
#endif
	}

	gpgHome.replace("//", "/");

	if (!gpgHome.endsWith('/'))
		gpgHome.append('/');

	if (gpgHome.startsWith('~'))
		gpgHome.replace(0, 1, QDir::homePath());

	KStandardDirs::makeDir(gpgHome, 0700);
	return gpgHome;
}

QStringList KgpgInterface::getGpgGroupNames(const QString &configfile)
{
	QStringList groups;
	QFile qfile(configfile);

	if (qfile.open(QIODevice::ReadOnly) && (qfile.exists())) {
		QTextStream t(&qfile);
		while (!t.atEnd()) {
			QString result(t.readLine().simplified());
			if (result.startsWith("group ")) {
				result.remove(0, 6);
				groups.append(result.section("=", 0, 0).simplified());
			}
		}
		qfile.close();
	}
	return groups;
}

QStringList KgpgInterface::getGpgGroupSetting(const QString &name, const QString &configfile)
{
	QFile qfile(configfile);
	if (qfile.open(QIODevice::ReadOnly) && (qfile.exists())) {
		QTextStream t(&qfile);
		while (!t.atEnd()) {
			QString result(t.readLine().simplified());

			if (result.startsWith("group ")) {
				kDebug(2100) << "Found 1 group";
				result.remove(0, 6);
				if (result.simplified().startsWith(name)) {
					kDebug(2100) << "Found group: " << name;
					result = result.section('=', 1);
					result = result.section('#', 0, 0);
					return result.split(' ');
				}
			}
		}
		qfile.close();
	}
	return QStringList();
}

void KgpgInterface::setGpgGroupSetting(const QString &name, const QStringList &values, const QString &configfile)
{
	QFile qfile(configfile);

	kDebug(2100) << "Changing group: " << name ;
	if (qfile.open(QIODevice::ReadOnly) && (qfile.exists())) {
		QTextStream t(&qfile);
		QString texttowrite;
		bool found = false;

		while (!t.atEnd()) {
			QString result(t.readLine());
			QString result2(result.simplified());

			if (result2.startsWith("group ")) {
				result2 = result2.remove(0, 6).simplified();
				if (result2.startsWith(name) && (result2.remove(0, name.length()).simplified().startsWith('='))) {
					result = QString("group %1=%2").arg(name).arg(values.join(QString(' ')));
					found = true;
				}
			}
			texttowrite += result + '\n';
		}
		qfile.close();

		if (!found)
			texttowrite += '\n' + QString("group %1=%2").arg(name).arg(values.join(" "));

		if (qfile.open(QIODevice::WriteOnly)) {
			QTextStream t(&qfile);
			t << texttowrite;
			qfile.close();
		}
	}
}

void KgpgInterface::delGpgGroup(const QString &name, const QString &configfile)
{
	QFile qfile(configfile);

	if (qfile.open(QIODevice::ReadOnly) && (qfile.exists())) {
		QTextStream t(&qfile);
		QString texttowrite;

		while (!t.atEnd()) {
			const QString result(t.readLine());
			QString result2(result.simplified());

			if (result2.startsWith("group ")) {
				result2 = result2.remove(0, 6).simplified();
				if (result2.startsWith(name) && (result2.remove(0, name.length()).simplified().startsWith('=')))
					continue;
			}

			texttowrite += result + '\n';
		}

		qfile.close();

		if (qfile.open(QIODevice::WriteOnly)) {
			QTextStream t(&qfile);
			t << texttowrite;
			qfile.close();
		}
	}
}

QString KgpgInterface::getGpgSetting(const QString &name, const QString &configfile)
{
	const QString tmp(name.simplified() + ' ');
	QFile qfile(configfile);

	if (qfile.open(QIODevice::ReadOnly) && (qfile.exists())) {
		QTextStream t(&qfile);
		while (!t.atEnd()) {
			QString result(t.readLine().simplified());
			if (result.startsWith(tmp)) {
				result = result.mid(tmp.length()).simplified();
				return result.section(" ", 0, 0);
			}
		}
		qfile.close();
	}

	return QString();
}

void KgpgInterface::setGpgSetting(const QString &name, const QString &value, const QString &url)
{
	QFile qfile(url);

	if (qfile.open(QIODevice::ReadOnly) && (qfile.exists())) {
		const QString temp(name + ' ');
		QString texttowrite;
		bool found = false;
		QTextStream t(&qfile);

		while (!t.atEnd()) {
			QString result = t.readLine();
			if (result.simplified().startsWith(temp)) {
				if (!value.isEmpty())
					result = temp + ' ' + value;
				else
					result.clear();
				found = true;
			}

			texttowrite += result + '\n';
		}

		qfile.close();
		if ((!found) && (!value.isEmpty()))
			texttowrite += '\n' + temp + ' ' + value;

		if (qfile.open(QIODevice::WriteOnly)) {
			QTextStream t(&qfile);
			t << texttowrite;
			qfile.close();
		}
	}
}

bool KgpgInterface::getGpgBoolSetting(const QString &name, const QString &configfile)
{
	QFile qfile(configfile);
	if (qfile.open(QIODevice::ReadOnly) && (qfile.exists())) {
		QTextStream t(&qfile);
		while (!t.atEnd()) {
			if (t.readLine().simplified().startsWith(name))
				return true;
		}
		qfile.close();
	}
	return false;
}

void KgpgInterface::setGpgBoolSetting(const QString &name, const bool enable, const QString &url)
{
	QFile qfile(url);

	if (qfile.open(QIODevice::ReadOnly) && (qfile.exists())) {
		QString texttowrite;
		bool found = false;
		QTextStream t(&qfile);

		while (!t.atEnd()) {
			QString result(t.readLine());

			if (result.simplified().startsWith(name)) {
				if (enable)
					result = name;
				else
					result.clear();

				found = true;
			}

			texttowrite += result + '\n';
		}
		qfile.close();

		if ((!found) && (enable))
			texttowrite += name;

		if (qfile.open(QIODevice::WriteOnly)) {
			QTextStream t(&qfile);
			t << texttowrite;
			qfile.close();
		}
	}
}

int KgpgInterface::sendPassphrase(const QString &text, KProcess *process, const bool isnew)
{
	QByteArray passphrase;
	int code;
	if (isnew) {
		KNewPasswordDialog dlg;
		dlg.setPrompt(text);
		code = dlg.exec();
		passphrase = dlg.password().toLocal8Bit();
	} else {
		KPasswordDialog dlg;
		dlg.setPrompt(text);
		code = dlg.exec();
		passphrase = dlg.password().toLocal8Bit();
	}

	if (code != KPasswordDialog::Accepted)
		return 1;

	process->write(passphrase + '\n');

	return 0;
}

void KgpgInterface::updateIDs(QString txt)
{
	int cut = txt.indexOf(' ', 22, Qt::CaseInsensitive);
	txt.remove(0, cut);

	if (txt.contains('(', Qt::CaseInsensitive))
		txt = txt.section('(', 0, 0) + txt.section(')', -1);

	txt.replace('<', "&lt;");

	if (!userIDs.contains(txt)) {
		if (!userIDs.isEmpty())
			userIDs += i18n(" or ");
		userIDs += txt;
	}
}

KgpgKeyList KgpgInterface::readPublicKeys(const bool block, const QStringList &ids, const bool withsigs)
{
	m_publiclistkeys = KgpgKeyList();
	m_publickey = KgpgKey();
	m_numberid = 0;
	cycle = "none";

	GPGProc *process = new GPGProc(this);
	*process << "--with-colons" << "--with-fingerprint" << "--fixed-list-mode";
	if (withsigs)
		*process << "--list-sigs";
	else
		*process << "--list-keys";

	*process << ids;
	process->setOutputChannelMode(KProcess::MergedChannels);

	if (!block) {
		connect(process, SIGNAL(readReady(GPGProc *)), SLOT(readPublicKeysProcess(GPGProc *)));
		connect(process, SIGNAL(processExited(GPGProc *)), SLOT(readPublicKeysFin(GPGProc *)));
		process->start();
		return KgpgKeyList();
	} else {
		process->start();
		process->waitForFinished(-1);
		readPublicKeysProcess(process);
		readPublicKeysFin(process, true);
		return m_publiclistkeys;
	}
}

void KgpgInterface::readPublicKeysProcess(GPGProc *p)
{
	QStringList lsp;
	int items;

	while ((items = p->readln(lsp)) >= 0) {
		if ((lsp.at(0) == "pub") && (items >= 10)) {
			if (cycle != "none") {
				cycle = "none";
				m_publiclistkeys << m_publickey;
			}

			m_publickey = KgpgKey();

			m_publickey.setTrust(Convert::toTrust(lsp.at(1)));
			m_publickey.setSize(lsp.at(2).toUInt());
			m_publickey.setAlgorithm(Convert::toAlgo(lsp.at(3).toInt()));
			m_publickey.setFingerprint(lsp.at(4));
			m_publickey.setCreation(QDateTime::fromTime_t(lsp.at(5).toUInt()).date());
			m_publickey.setOwnerTrust(Convert::toOwnerTrust(lsp.at(8)));

			if (lsp.at(6).isEmpty())
				m_publickey.setExpiration(QDate());
			else
				m_publickey.setExpiration(QDateTime::fromTime_t(lsp.at(6).toUInt()).date());

			m_publickey.setValid((items <= 11) || !lsp.at(11).contains('D', Qt::CaseSensitive));  // disabled key

			cycle = "pub";

			m_numberid = 0;
		} else if ((lsp.at(0) == "fpr") && (items >= 10)) {
			const QString fingervalue(lsp.at(9));

			m_publickey.setFingerprint(fingervalue);
		} else if ((lsp.at(0) == "sub") && (items >= 7)) {
			KgpgKeySub sub;

			sub.setId(lsp.at(4).right(8));
			sub.setTrust(Convert::toTrust(lsp.at(1)));
			sub.setSize(lsp.at(2).toUInt());
			sub.setAlgorithm(Convert::toAlgo(lsp.at(3).toInt()));
			sub.setCreation(QDateTime::fromTime_t(lsp.at(5).toUInt()).date());

			// FIXME: Please see kgpgkey.h, KgpgSubKey class
			if (items <= 11) {
				sub.setValid(true);
			} else {
				sub.setValid(!lsp.at(11).contains('D'));

				if (lsp.at(11).contains('s'))
					sub.setType(SKT_SIGNATURE);
				else
				if (lsp.at(11).contains('e'))
					sub.setType(SKT_ENCRYPTION);
			}

			if (lsp.at(6).isEmpty())
				sub.setExpiration(QDate());
			else
				sub.setExpiration(QDateTime::fromTime_t(lsp.at(6).toUInt()).date());

			m_publickey.subList()->append(sub);
			cycle = "sub";
		} else if (lsp.at(0) == "uat") {
			m_numberid++;
			KgpgKeyUat uat;
			uat.setId(QString::number(m_numberid));
			uat.setCreation(QDateTime::fromTime_t(lsp.at(5).toUInt()).date());
			m_publickey.uatList()->append(uat);

			cycle = "uat";
		} else if ((lsp.at(0) == "uid") && (items >= 10)) {
			QString fullname(lsp.at(9));
			QString kmail;
			if (fullname.contains('<') ) {
				kmail = fullname;

				if (fullname.contains(')') )
				kmail = kmail.section(')', 1);

				kmail = kmail.section('<', 1);
				kmail.truncate(kmail.length() - 1);

				if ( kmail.contains('<') ) // several email addresses in the same key
				{
				kmail = kmail.replace('>', ';');
				kmail.remove('<');
				}
			}

			QString kname(fullname.section(" <", 0, 0));
			QString comment;
			if (fullname.contains('(') ) {
				kname = kname.section(" (", 0, 0);
				comment = fullname.section('(', 1, 1);
				comment = comment.section(')', 0, 0);
			}

			if (m_numberid == 0) {
				m_numberid++;
				m_publickey.setEmail(kmail);
				m_publickey.setComment(comment);
				m_publickey.setName(kname);
			} else {
				KgpgKeyUid uid;

				uid.setEmail(kmail);
				uid.setComment(comment);
				uid.setName(kname);
				uid.setTrust(Convert::toTrust(lsp.at(1)));
				uid.setValid((items <= 11) || !lsp.at(11).contains('D'));

				uid.setIndex(++m_numberid);

				m_publickey.uidList()->append(uid);

				cycle = "uid";
			}
		} else if (((lsp.at(0) == "sig") || (lsp.at(0) == "rev")) && (items >= 11)) {
			KgpgKeySign signature;

			signature.setId(lsp.at(4));
			signature.setCreation(QDateTime::fromTime_t(lsp.at(5).toUInt()).date());

			if (lsp.at(6).isEmpty())
				signature.setExpiration(QDate());
			else
				signature.setExpiration(QDateTime::fromTime_t(lsp.at(6).toUInt()).date());

			if (lsp.at(10).endsWith('l'))
				signature.setLocal(true);

			if (lsp.at(0) == "rev")
				signature.setRevocation(true);

			if (cycle == "pub")
				m_publickey.addSign(signature);
			else if (cycle == "uat")
				m_publickey.uatList()->last().addSign(signature);
			else if (cycle == "uid")
				m_publickey.uidList()->last().addSign(signature);
			else if (cycle == "sub")
				m_publickey.subList()->last().addSign(signature);
		} else {
			log += lsp.join(QString(':')) + '\n';
		}
	}
}

void KgpgInterface::readPublicKeysFin(GPGProc *p, const bool block)
{
	// insert the last key
	if (cycle != "none")
		m_publiclistkeys << m_publickey;

	if (p->exitCode() != 0) {
		KMessageBox::detailedError(NULL, i18n("An error occurred while scanning your keyring"), log);
		log.clear();
	}

	p->deleteLater();
	if (!block)
		emit readPublicKeysFinished(m_publiclistkeys, this);
}


KgpgKeyList KgpgInterface::readSecretKeys(const QStringList &ids)
{
	m_partialline.clear();
	m_ispartial = false;
	m_secretlistkeys = KgpgKeyList();
	m_secretkey = KgpgKey();
	m_secretactivate = false;

	GPGProc *process = new GPGProc(this);
	*process << "--with-colons" << "--list-secret-keys" << "--with-fingerprint" << "--fixed-list-mode";

	*process << ids;

	process->start();
	process->waitForFinished(-1);
	readSecretKeysProcess(process);

	if (m_secretactivate)
		m_secretlistkeys << m_secretkey;

	delete process;

	return m_secretlistkeys;
}

void KgpgInterface::readSecretKeysProcess(GPGProc *p)
{
	QStringList lsp;
	int items;
	bool hasuid = true;

	while ( (items = p->readln(lsp)) >= 0 ) {
		if ((lsp.at(0) == "sec") && (items >= 10)) {
			if (m_secretactivate)
			m_secretlistkeys << m_secretkey;

			m_secretactivate = true;
			m_secretkey = KgpgKey();

			m_secretkey.setTrust(Convert::toTrust(lsp.at(1)));
			m_secretkey.setSize(lsp.at(2).toUInt());
			m_secretkey.setAlgorithm(Convert::toAlgo(lsp.at(3).toInt()));
			m_secretkey.setFingerprint(lsp.at(4));
			m_secretkey.setCreation(QDateTime::fromTime_t(lsp.at(5).toUInt()).date());
			m_secretkey.setSecret(true);

			if (lsp.at(6).isEmpty())
				m_secretkey.setExpiration(QDate());
			else
				m_secretkey.setExpiration(QDateTime::fromTime_t(lsp.at(6).toUInt()).date());
			hasuid = true;
		} else if ((lsp.at(0) == "uid") && (items >= 10)) {
			if (hasuid)
				continue;

			hasuid = true;

			const QString fullname(lsp.at(9));
			if (fullname.contains('<' )) {
				QString kmail(fullname);

				if (fullname.contains(')' ))
					kmail = kmail.section(')', 1);

				kmail = kmail.section('<', 1);
				kmail.truncate(kmail.length() - 1);

				if (kmail.contains('<' )) { // several email addresses in the same key
					kmail = kmail.replace('>', ';');
					kmail.remove('<');
				}

				m_secretkey.setEmail(kmail);
			} else {
				m_secretkey.setEmail(QString());
			}

			QString kname(fullname.section(" <", 0, 0));
			if (fullname.contains('(' )) {
				kname = kname.section(" (", 0, 0);
				QString comment = fullname.section('(', 1, 1);
				comment = comment.section(')', 0, 0);

				m_secretkey.setComment(comment);
			} else {
				m_secretkey.setComment(QString());
			}
			m_secretkey.setName(kname);
		} else if ((lsp.at(0) == "fpr") && (items >= 10)) {
			const QString fingervalue(lsp.at(9));

			m_secretkey.setFingerprint(fingervalue);
		}
	}
}

KgpgCore::KgpgKeyList
KgpgInterface::readJoinedKeys(const KgpgKeyTrust &trust, const QStringList &ids)
{
	KgpgKeyList secretkeys = readSecretKeys(ids);
	KgpgKeyList publickeys = readPublicKeys(true, ids, false);
	int i, j;

	for (i = publickeys.size() - 1; i >= 0; i--)
		if (publickeys.at(i).trust() < trust)
			publickeys.removeAt(i);

	for (i = 0; i < secretkeys.size(); i++) {
		for (j = 0; j < publickeys.size(); j++)
			if (secretkeys.at(i).fullId() == publickeys.at(j).fullId()) {
				publickeys[j].setSecret(true);
				break;
			}
	}

	// move the secret keys to the top of the list
	for (j = 1; j < publickeys.size(); j++)
		if (publickeys.at(j).secret())
			publickeys.move(j, 0);

	return publickeys;
}

void KgpgInterface::signKey(const QString &keyid, const QString &signkeyid, const bool local, const int checking, const bool terminal, const QString &uid)
{
	m_partialline.clear();
	m_ispartial = false;
	log.clear();
	m_signkey = signkeyid;
	m_keyid = keyid;
	m_checking = checking;
	m_local = local;
	step = 3;

	if (terminal) {
		signKeyOpenConsole();
		return;
	}

	m_success = 0;

	m_workProcess = new KProcess(this);
	m_workProcess->setOutputChannelMode(KProcess::OnlyStdoutChannel);
	*m_workProcess << KGpgSettings::gpgBinaryPath();
	*m_workProcess << "--no-secmem-warning" << "--no-tty" << "--status-fd=1" << "--command-fd=0";

	*m_workProcess << "-u" << signkeyid;
	*m_workProcess << "--edit-key" << keyid;

	if (!uid.isEmpty())
		*m_workProcess << "uid" << uid;

	if (local)
		*m_workProcess << "lsign";
	else
		*m_workProcess << "sign";

	kDebug(3125) << "Signing key" << keyid << "uid" << uid << "with key" << signkeyid;
	connect(m_workProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(signKeyProcess()));
	connect(m_workProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(signKeyFin()));

	m_workProcess->start();
}

void KgpgInterface::signKeyProcess()
{
	QString buffer(m_partialline + m_workProcess->readAllStandardOutput());

	while (buffer.contains('\n')) {
		int pos = buffer.indexOf('\n');
		QString line(buffer.left(pos));
		buffer.remove(0, pos + 1);

		if (line.startsWith("[GNUPG:]")) {
			if (line.contains("USERID_HINT")) {
				updateIDs(line);
			} else if (m_success == 3) {
				// user has aborted the process and don't want to sign the key
				if (line.contains("GET_"))
					m_workProcess->write("quit\n");
				return;
			} else if (line.contains("ALREADY_SIGNED")) {
				m_success = 4;
			} else  if (line.contains("GOOD_PASSPHRASE")) {
				m_success = 2;
			} else if (line.contains("sign_uid.expire")) {
				m_workProcess->write("Never\n");
			} else if (line.contains("sign_uid.class")) {
				m_workProcess->write(QString::number(m_checking).toAscii() + '\n');
			} else if (line.contains("sign_uid.okay")) {
				m_workProcess->write("Y\n");
			} else if (line.contains("sign_all.okay")) {
				m_workProcess->write("Y\n");
			} else if (line.contains("passphrase.enter")) {
				QString passdlgmessage;
				if (step < 3)
					passdlgmessage = i18np("<p><b>Bad passphrase</b>. You have 1 try left.</p>",
							"<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
				passdlgmessage += i18n("Enter passphrase for <b>%1</b>", checkForUtf8bis(userIDs));

				if (sendPassphrase(passdlgmessage, m_workProcess, false)) {
					m_success = 3;
					m_workProcess->write("quit\n");
					return;
				}

				if (step > 1)
					step--;
				else
					step = 3;
			} else if ((m_success != 1) && line.contains("keyedit.prompt")) {
				m_workProcess->write("save\n");
			} else if (line.contains("BAD_PASSPHRASE")) {
				m_success = 1;
			} else if (line.contains("GET_")) {
				// gpg asks for something unusal, turn to konsole mode
				if (m_success != 1)
					m_success = 5; // switching to console mode
				m_workProcess->write("quit\n");
			}
		} else {
			log += line + '\n';
		}
	}
	m_partialline = buffer;
}

void KgpgInterface::signKeyFin()
{
	m_workProcess->deleteLater();
	if ((m_success != 0) && (m_success != 5)) {
		// signature successful or bad passphrase or aborted or already signed
		emit signKeyFinished(m_success, m_keyid, this);
	} else {
		KgpgDetailedConsole *q = new KgpgDetailedConsole(0,
				i18n("<qt>Signing key <b>%1</b> with key <b>%2</b> failed.<br />Do you want to try signing the key in console mode?</qt>", m_keyid, m_signkey),
				log);
		if (q->exec() == QDialog::Accepted)
			signKeyOpenConsole();
		else
			emit signKeyFinished(3, m_keyid, this);
	}
}

void KgpgInterface::signKeyOpenConsole()
{
	KConfigGroup config(KGlobal::config(), "General");

	KProcess process;
	process << config.readPathEntry("TerminalApplication", "konsole");
	process << "-e" << KGpgSettings::gpgBinaryPath() << "--no-secmem-warning" << "-u" << m_signkey;
	process << "--default-cert-level" << QString(m_checking);

	if (!m_local)
		process << "--sign-key" << m_keyid;
	else
		process << "--lsign-key" << m_keyid;

	process.execute();
	emit signKeyFinished(2, m_keyid, this);
}

QPixmap KgpgInterface::loadPhoto(const QString &keyid, const QString &uid, const bool block)
{
#ifdef Q_OS_WIN32	//krazy:exclude=cpp
	const QString pgpgoutput("cmd /C \"echo %I\"");
#else
	const QString pgpgoutput("echo %I");
#endif

	m_workProcess = new KProcess(this);
	m_workProcess->setOutputChannelMode(KProcess::OnlyStdoutChannel);
	*m_workProcess << KGpgSettings::gpgBinaryPath();
	*m_workProcess << "--no-secmem-warning" << "--no-tty" << "--status-fd=2";
	*m_workProcess << "--photo-viewer" << pgpgoutput << "--edit-key" << keyid << "uid" << uid << "showphoto" << "quit";

	connect(m_workProcess, SIGNAL(finished(int)), SLOT(loadPhotoFin(int)));
	m_workProcess->start();
	if (!block) {
		return QPixmap();
	} else {
		m_workProcess->waitForFinished();
		return m_pixmap;
	}
}

void KgpgInterface::loadPhotoFin(int exitCode)
{
	m_pixmap = QPixmap();

	if (exitCode == 0) {
		QByteArray pic(m_workProcess->readAllStandardOutput());

		while (pic.endsWith('\r') || pic.endsWith('\n'))
			pic.chop(1);

		KUrl url(pic);
		m_pixmap.load(url.path());
		QFile::remove(url.path());
		QDir dir;
		dir.rmdir(url.directory());
	}

	emit loadPhotoFinished(m_pixmap, this);
}

// delete signature
void KgpgInterface::KgpgDelSignature(const QString &keyID, const QString &uid, QString signKeyID)
{
	deleteSuccess = false;
	step = 0;

	signb = 0;
	sigsearch = 0;
	QList<int> signoff;

	findSigns(keyID, QStringList(signKeyID), uid, &signoff);

	if (signoff.count() == 0)
		return;

	signb = signoff.at(0);

	GPGProc *conprocess = new GPGProc(this);
	*conprocess << "--command-fd=0" << "--status-fd=1" << "--edit-key" << keyID << "uid" << uid << "delsig";
	connect(conprocess, SIGNAL(readReady(GPGProc *)), SLOT(delsigprocess(GPGProc *)));
	connect(conprocess, SIGNAL(processExited(GPGProc *)), SLOT(delsignover(GPGProc *)));
	conprocess->start();
}

void KgpgInterface::delsigprocess(GPGProc *p)
{
	QString required;

	while (p->readln(required, true) >= 0) {
		if (required.contains("keyedit.delsig")){
			if ((sigsearch == signb) && (step == 0)) {
				p->write("Y\n");
				step = 1;
			} else
				p->write("n\n");
			sigsearch++;
			required.clear();
		} else if ((step == 1) && required.contains("keyedit.prompt")) {
			p->write("save\n");
			required.clear();
			deleteSuccess = true;
		} else if (required.contains("GET_LINE")) {
			p->write("quit\n");
			deleteSuccess = false;
		}
	}
}

void KgpgInterface::delsignover(GPGProc *p)
{
	p->deleteLater();
	emit delsigfinished(deleteSuccess);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  key revocation

void KgpgInterface::KgpgRevokeKey(const QString &keyID, const KUrl &revokeUrl, const int reason, const QString &description)
{
	revokeReason = reason;
	revokeSuccess = false;
	revokeDescription = description;
	certificateUrl = revokeUrl;
	output.clear();

	GPGProc *process = new GPGProc(this);
	*process << "--status-fd=1" << "--command-fd=0";

	if (!revokeUrl.isEmpty())
		*process << "-o" << revokeUrl.toLocalFile();
	*process << "--gen-revoke" << keyID;
	QObject::connect(process, SIGNAL(processExited(GPGProc *)), SLOT(revokeover(GPGProc *)));
	QObject::connect(process, SIGNAL(readReady(GPGProc *)), SLOT(revokeprocess(GPGProc *)));
	process->start();
}

void KgpgInterface::revokeover(GPGProc *)
{
	if (!revokeSuccess) {
		KMessageBox::detailedSorry(0,i18n("Creation of the revocation certificate failed..."), output);
	} else {
		output = output.section("-----BEGIN", 1);
		output.prepend("-----BEGIN");
		output = output.section("BLOCK-----", 0);
		emit revokecertificate(output);
		if (!certificateUrl.isEmpty())
			emit revokeurl(certificateUrl);
	}
}

void KgpgInterface::revokeprocess(GPGProc *p)
{
	QString required;

	while (p->readln(required) >= 0) {
		output += required + '\n';

		if (required.contains("USERID_HINT",Qt::CaseInsensitive)) {
			updateIDs(required);

		} else if (required.contains("GOOD_PASSPHRASE")) {
			revokeSuccess=true;

		} else if (required.contains("gen_revoke.okay") ||
					required.contains("ask_revocation_reason.okay") ||
					required.contains("openfile.overwrite.okay")) {
			p->write("YES\n");
		} else if (required.contains("ask_revocation_reason.code")) {
			p->write(QString::number(revokeReason).toAscii() + '\n');
		} else if (required.contains("passphrase.enter")) {
			if (sendPassphrase(i18n("<qt>Enter passphrase for <b>%1</b>:</qt>", checkForUtf8bis(userIDs)), p)) {
				expSuccess=3;  //  aborted by user mode
				p->write("quit\n");
				return;
			}
		} else if (required.contains("ask_revocation_reason.text")) {
			p->write(revokeDescription.toAscii() + '\n');
			revokeDescription.clear();
		} else if ((required.contains("GET_"))) {
			// gpg asks for something unusal, turn to konsole mode
			kDebug(2100) << "unknown request:" << required;
			expSuccess=1;  /////  switching to console mode
			p->write("quit\n");
		}
	}
}

void KgpgInterface::findSigns(const QString &keyID, const QStringList &ids, const QString &uid, QList<int> *res)
{
	GPGProc *listproc = new GPGProc(this);
	*listproc << "--status-fd=1";
	*listproc << "--with-colons" << "--list-sigs" << "--fixed-list-mode" << keyID;
	listproc->start();
	listproc->waitForFinished(-1);

	QString line;
	int curuid = 0;
	int signs = 0;
	int tgtuid = uid.toInt();

	while (listproc->readln(line, true) >= 0) {
		if (line.startsWith("sig:") && (tgtuid == curuid)) {
			if (ids.contains(line.section(':', 4, 4), Qt::CaseInsensitive))
				*res << signs;
			signs++;
		} else if (line.startsWith("uid:")) {
			curuid++;
		}
	}

	delete listproc;
}

#include "kgpginterface.moc"
