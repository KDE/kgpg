/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */
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
#include <QFile>
#include <QPixmap>
#include <QString>
#include <QTextStream>

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
#include "core/KGpgKeyNode.h"
#include "core/KGpgSignNode.h"
#include "core/KGpgSubkeyNode.h"
#include "core/KGpgUatNode.h"
#include "core/KGpgUidNode.h"

using namespace KgpgCore;

KgpgInterface::KgpgInterface()
	: m_readNode(NULL),
	m_currentSNode(NULL)
{
}

KgpgInterface::~KgpgInterface()
{
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
		if (line.startsWith(QLatin1String("Home: "))) {
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
			if (result.startsWith(QLatin1String("group "))) {
				result.remove(0, 6);
				groups.append(result.section('=', 0, 0).simplified());
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

			if (result.startsWith(QLatin1String("group "))) {
				kDebug(2100) << "Found 1 group";
				result.remove(0, 6);
				if (result.simplified().startsWith(name)) {
					kDebug(2100) << "Found group: " << name;
					result = result.section('=', 1);
					result = result.section('#', 0, 0);
					return result.split(' ', QString::SkipEmptyParts);
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

	kDebug(2100) << "Changing group: " << name;
	if (qfile.open(QIODevice::ReadOnly) && (qfile.exists())) {
		QTextStream t(&qfile);
		QString texttowrite;
		bool found = false;

		while (!t.atEnd()) {
			QString result(t.readLine());
			QString result2(result.simplified());

			if (result2.startsWith(QLatin1String("group "))) {
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

bool KgpgInterface::renameGroup(const QString &oldName, const QString &newName, const QString &configfile)
{
	QFile qfile(configfile);

	kDebug(2100) << "Renaming group " << oldName << " to " << newName;
	if (qfile.open(QIODevice::ReadOnly) && (qfile.exists())) {
		QTextStream t(&qfile);
		QString texttowrite;
		bool found = false;

		while (!t.atEnd()) {
			QString result = t.readLine();
			QString result2 = result.simplified();

			if (result2.startsWith(QLatin1String("group "))) {
				result2 = result2.remove(0, 6).simplified();
				if (result2.startsWith(oldName)) {
					QString values = result2.remove(0, oldName.length()).simplified();
					found = values.startsWith('=');
					if (found) {
						result = QLatin1String("group ") + newName + ' ' + values;
					}
				}
			}
			texttowrite += result + '\n';
		}
		qfile.close();

		if (!found) {
			kDebug(2100) << "Group " << oldName << " not renamed, group does not exist";
			return false;
		}

		if (qfile.open(QIODevice::WriteOnly)) {
			QTextStream t(&qfile);
			t << texttowrite;
			qfile.close();
			return true;
		}
	}
	return false;
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

			if (result2.startsWith(QLatin1String("group "))) {
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
				return result.section(' ', 0, 0);
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
		passphrase = dlg.password().toUtf8();
	} else {
		KPasswordDialog dlg;
		dlg.setPrompt(text);
		code = dlg.exec();
		passphrase = dlg.password().toUtf8();
	}

	if (code != KPasswordDialog::Accepted)
		return 1;

	process->write(passphrase + '\n');

	return 0;
}

KgpgKeyList KgpgInterface::readPublicKeys(const bool block, const QStringList &ids)
{
	m_publiclistkeys.clear();
	m_publickey = KgpgKey();
	m_numberid = 0;

	GPGProc *process = new GPGProc(this);
	*process << "--with-colons" << "--with-fingerprint" << "--fixed-list-mode" << "--list-keys";

	*process << ids;
	process->setOutputChannelMode(KProcess::MergedChannels);

	if (!block) {
		connect(process, SIGNAL(readReady()), SLOT(readPublicKeysProcess()));
		connect(process, SIGNAL(processExited()), SLOT(readPublicKeysFin()));
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

KgpgCore::KgpgKey KgpgInterface::readSignatures(KGpgKeyNode *node)
{
	m_readNode = node;

	m_publiclistkeys.clear();
	m_publickey = KgpgKey();
	m_numberid = 0;

	GPGProc *process = new GPGProc(this);
	*process << "--with-colons" << "--with-fingerprint" << "--fixed-list-mode" << "--list-sigs";

	*process << node->getId();
	process->setOutputChannelMode(KProcess::MergedChannels);

	process->start();
	process->waitForFinished(-1);
	readPublicKeysProcess(process);
	readPublicKeysFin(process, true);

	return m_publiclistkeys.first();
}

void KgpgInterface::readPublicKeysProcess(GPGProc *p)
{
	QStringList lsp;
	int items;
	if (p == NULL)
		p = qobject_cast<GPGProc *>(sender());

	while ((items = p->readln(lsp)) >= 0) {
		if ((lsp.at(0) == "pub") && (items >= 10)) {
			if (!m_publickey.name().isEmpty())
				m_publiclistkeys << m_publickey;

			m_publickey = KgpgKey();

			m_publickey.setTrust(Convert::toTrust(lsp.at(1)));
			m_publickey.setSize(lsp.at(2).toUInt());
			m_publickey.setAlgorithm(Convert::toAlgo(lsp.at(3).toInt()));
			m_publickey.setFingerprint(lsp.at(4));
			m_publickey.setCreation(QDateTime::fromTime_t(lsp.at(5).toUInt()));
			m_publickey.setOwnerTrust(Convert::toOwnerTrust(lsp.at(8)));

			if (lsp.at(6).isEmpty())
				m_publickey.setExpiration(QDateTime());
			else
				m_publickey.setExpiration(QDateTime::fromTime_t(lsp.at(6).toUInt()));

			m_publickey.setValid((items <= 11) || !lsp.at(11).contains('D', Qt::CaseSensitive));  // disabled key

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
			sub.setCreation(QDateTime::fromTime_t(lsp.at(5).toUInt()));

			// FIXME: Please see kgpgkey.h, KgpgSubKey class
			if (items <= 11) {
				sub.setValid(true);
			} else {
				sub.setValid(!lsp.at(11).contains('D'));

				if (lsp.at(11).contains('s'))
					sub.setType(sub.type() | SKT_SIGNATURE);
				if (lsp.at(11).contains('e'))
					sub.setType(sub.type() | SKT_ENCRYPTION);
				if (lsp.at(11).contains('e'))
					sub.setType(sub.type() | SKT_AUTHENTICATION);
				if (lsp.at(11).contains('e'))
					sub.setType(sub.type() | SKT_CERTIFICATION);
			}

			if (lsp.at(6).isEmpty())
				sub.setExpiration(QDateTime());
			else
				sub.setExpiration(QDateTime::fromTime_t(lsp.at(6).toUInt()));

			m_publickey.subList()->append(sub);
			if (m_readNode == NULL)
				m_currentSNode = NULL;
			else
				m_currentSNode = new KGpgSubkeyNode(m_readNode, sub);
		} else if (lsp.at(0) == "uat") {
			m_numberid++;
			if (m_readNode != NULL) {
				m_currentSNode = new KGpgUatNode(m_readNode, m_numberid, lsp);
			}
		} else if ((lsp.at(0) == "uid") && (items >= 10)) {
			if (m_numberid == 0) {
				QString fullname(lsp.at(9));
				QString kmail;
				if (fullname.contains('<') ) {
					kmail = fullname;

					if (fullname.contains(')') )
						kmail = kmail.section(')', 1);

					kmail = kmail.section('<', 1);
					kmail.truncate(kmail.length() - 1);

					if (kmail.contains('<')) {
						// several email addresses in the same key
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

				m_numberid++;
				m_publickey.setEmail(kmail);
				m_publickey.setComment(comment);
				m_publickey.setName(kname);

				m_currentSNode = m_readNode;
			} else {
				m_numberid++;
				if (m_readNode != NULL) {
					m_currentSNode = new KGpgUidNode(m_readNode, m_numberid, lsp);
				}
			}
		} else if (((lsp.at(0) == "sig") || (lsp.at(0) == "rev")) && (items >= 11)) {
			// there are no strings here that could have a recoded ':' in them
			const QString signature = lsp.join(QLatin1String(":"));

			if (m_currentSNode != NULL)
				(void) new KGpgSignNode(m_currentSNode, lsp);
		} else {
			log += lsp.join(QString(':')) + '\n';
		}
	}
}

void KgpgInterface::readPublicKeysFin(GPGProc *p, const bool block)
{
	if (p == NULL)
		p = qobject_cast<GPGProc *>(sender());

	// insert the last key
	if (!m_publickey.name().isEmpty())
		m_publiclistkeys << m_publickey;

	if (p->exitCode() != 0) {
		KMessageBox::detailedError(NULL, i18n("An error occurred while scanning your keyring"), log);
		log.clear();
	}

	p->deleteLater();
	if (!block)
		emit readPublicKeysFinished(m_publiclistkeys);
}


KgpgKeyList KgpgInterface::readSecretKeys(const QStringList &ids)
{
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
			m_secretkey.setCreation(QDateTime::fromTime_t(lsp.at(5).toUInt()));
			m_secretkey.setSecret(true);

			if (lsp.at(6).isEmpty())
				m_secretkey.setExpiration(QDateTime());
			else
				m_secretkey.setExpiration(QDateTime::fromTime_t(lsp.at(6).toUInt()));
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

QPixmap KgpgInterface::loadPhoto(const QString &keyid, const QString &uid)
{
#ifdef Q_OS_WIN32	//krazy:exclude=cpp
	const QString pgpgoutput = QLatin1String("cmd /C \"echo %I\"");
#else
	const QString pgpgoutput = QLatin1String("echo %I");
#endif

	GPGProc workProcess;
	workProcess << "--no-greeting" << "--status-fd=2";
	workProcess << "--photo-viewer" << pgpgoutput << "--edit-key" << keyid << "uid" << uid << "showphoto" << "quit";

	workProcess.start();
	workProcess.waitForFinished();
	if (workProcess.exitCode() != 0)
		return QPixmap();

	QString tmpfile;
	if (workProcess.readln(tmpfile) < 0)
		return QPixmap();

	KUrl url(tmpfile);
	QPixmap pixmap;
	pixmap.load(url.path());
	QFile::remove(url.path());
	QDir dir;
	dir.rmdir(url.directory());

	return pixmap;
}

#include "kgpginterface.moc"
