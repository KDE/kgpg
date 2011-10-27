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
#include <QPointer>
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

	QStringList values(vstr.split(QLatin1Char( '.' )));
	if (values.count() < 3)
		return -2;

	return (0x10000 * values[0].toInt() + 0x100 * values[1].toInt() + values[2].toInt());
}

QString KgpgInterface::gpgVersionString(const QString &binary)
{
	GPGProc process(0, binary);
	process << QLatin1String( "--version" );
	process.start();
	process.waitForFinished(-1);

	if (process.exitCode() == 255)
		return QString();

	QString line;
	if (process.readln(line) != -1)
		return line.simplified().section(QLatin1Char( ' ' ), -1);
	else
		return QString();
}

QString KgpgInterface::getGpgProcessHome(const QString &binary)
{
	GPGProc process(0, binary);
	process << QLatin1String( "--version" );
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

QString KgpgInterface::getGpgStartupError(const QString &binary)
{
	GPGProc process(0, binary);
	process << QLatin1String( "--version" );
	process.start();
	process.waitForFinished(-1);

	QString result;

	while (process.hasLineStandardError()) {
		QByteArray tmp;
		process.readLineStandardError(&tmp);
		tmp += '\n';
		result += QString::fromUtf8(tmp);
	}

	return result;
}

QString KgpgInterface::getGpgHome(const QString &binary)
{
	// First try: if environment is set GnuPG will use that directory
	// We can use this directly without starting a new process
	QByteArray env(qgetenv("GNUPGHOME"));
	QString gpgHome;
	if (!env.isEmpty()) {
		gpgHome = QLatin1String( env );
	} else if (!binary.isEmpty()) {
		// Second try: start GnuPG and ask what it is
		gpgHome = getGpgProcessHome(binary);
	}

	// Third try: guess what it is.
	if (gpgHome.isEmpty()) {
#ifdef Q_OS_WIN32	//krazy:exclude=cpp
		gpgHome = qgetenv("APPDATA") + QLatin1String( "/gnupg/" );
		gpgHome.replace(QLatin1Char( '\\' ), QLatin1Char( '/' ));
#else
		gpgHome = QDir::homePath() + QLatin1String( "/.gnupg/" );
#endif
	}

	gpgHome.replace(QLatin1String( "//" ), QLatin1String( "/" ));

	if (!gpgHome.endsWith(QLatin1Char( '/' )))
		gpgHome.append(QLatin1Char( '/' ));

	if (gpgHome.startsWith(QLatin1Char( '~' )))
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
				groups.append(result.section(QLatin1Char( '=' ), 0, 0).simplified());
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
					result = result.section(QLatin1Char( '=' ), 1);
					result = result.section(QLatin1Char( '#' ), 0, 0);
					return result.split(QLatin1Char( ' ' ), QString::SkipEmptyParts);
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
				if (result2.startsWith(name) && (result2.remove(0, name.length()).simplified().startsWith(QLatin1Char( '=' )))) {
                                    result = QString::fromLatin1("group %1=%2").arg(name).arg(values.join(QLatin1String( " " )));
					found = true;
				}
			}
			texttowrite += result + QLatin1Char( '\n' );
		}
		qfile.close();

		if (!found)
                    texttowrite += QLatin1Char( '\n' ) + QString::fromLatin1("group %1=%2").arg(name).arg(values.join( QLatin1String( " " )));

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
					found = values.startsWith(QLatin1Char( '=' ));
					if (found) {
						result = QLatin1String("group ") + newName + QLatin1Char( ' ' ) + values;
					}
				}
			}
			texttowrite += result + QLatin1Char( '\n' );
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
				if (result2.startsWith(name) && (result2.remove(0, name.length()).simplified().startsWith(QLatin1Char( '=' ))))
					continue;
			}

			texttowrite += result + QLatin1Char( '\n' );
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
	const QString tmp(name.simplified() + QLatin1Char( ' ' ));
	QFile qfile(configfile);

	if (qfile.open(QIODevice::ReadOnly) && (qfile.exists())) {
		QTextStream t(&qfile);
		while (!t.atEnd()) {
			QString result(t.readLine().simplified());
			if (result.startsWith(tmp)) {
				result = result.mid(tmp.length()).simplified();
				return result.section(QLatin1Char( ' ' ), 0, 0);
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
		const QString temp(name + QLatin1Char( ' ' ));
		QString texttowrite;
		bool found = false;
		QTextStream t(&qfile);

		while (!t.atEnd()) {
			QString result = t.readLine();
			if (result.simplified().startsWith(temp)) {
				if (!value.isEmpty())
					result = temp + QLatin1Char( ' ' ) + value;
				else
					result.clear();
				found = true;
			}

			texttowrite += result + QLatin1Char( '\n' );
		}

		qfile.close();
		if ((!found) && (!value.isEmpty()))
			texttowrite += QLatin1Char( '\n' ) + temp + QLatin1Char( ' ' ) + value;

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

			texttowrite += result + QLatin1Char( '\n' );
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

int KgpgInterface::sendPassphrase(const QString &text, KProcess *process, const bool isnew, QWidget *widget)
{
	QPointer<KProcess> gpgprocess = process;
	QByteArray passphrase;
	int code;
	if (isnew) {
		QPointer<KNewPasswordDialog> dlg = new KNewPasswordDialog(widget);
		dlg->setPrompt(text);
		dlg->setAllowEmptyPasswords(false);
		code = dlg->exec();
		if (!dlg.isNull())
			passphrase = dlg->password().toUtf8();
		delete dlg;
	} else {
		QPointer<KPasswordDialog> dlg = new KPasswordDialog(widget);
		dlg->setPrompt(text);
		code = dlg->exec();
		if (!dlg.isNull())
			passphrase = dlg->password().toUtf8();
		delete dlg;
	}

	if (code != KPasswordDialog::Accepted)
		return 1;

	if (!gpgprocess.isNull())
		gpgprocess->write(passphrase + '\n');

	return 0;
}

KgpgKeyList KgpgInterface::readPublicKeys(const bool block, const QStringList &ids)
{
	m_publiclistkeys.clear();
	m_publickey = KgpgKey();
	m_numberid = 0;

	GPGProc *process = new GPGProc(this);
	*process << QLatin1String( "--with-colons" ) << QLatin1String( "--with-fingerprint" ) << QLatin1String( "--fixed-list-mode" ) << QLatin1String( "--list-keys" );

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
	*process << QLatin1String( "--with-colons" ) << QLatin1String( "--with-fingerprint" ) << QLatin1String( "--fixed-list-mode" ) << QLatin1String( "--list-sigs" );

	*process << node->getId();
	process->setOutputChannelMode(KProcess::MergedChannels);

	process->start();
	process->waitForFinished(-1);
	readPublicKeysProcess(process);
	readPublicKeysFin(process, true);

	if (m_publiclistkeys.isEmpty())
		return KgpgCore::KgpgKey();
	else
		return m_publiclistkeys.first();
}

void KgpgInterface::readPublicKeysProcess(GPGProc *p)
{
	QStringList lsp;
	int items;
	if (p == NULL)
		p = qobject_cast<GPGProc *>(sender());

	while ((items = p->readln(lsp)) >= 0) {
		if ((lsp.at(0) == QLatin1String( "pub" )) && (items >= 10)) {
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

			m_publickey.setValid((items <= 11) || !lsp.at(11).contains(QLatin1Char( 'D' ), Qt::CaseSensitive));  // disabled key

			m_numberid = 0;
		} else if ((lsp.at(0) == QLatin1String( "fpr" )) && (items >= 10)) {
			const QString fingervalue(lsp.at(9));

			m_publickey.setFingerprint(fingervalue);
		} else if ((lsp.at(0) == QLatin1String( "sub" )) && (items >= 7)) {
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
				sub.setValid(!lsp.at(11).contains(QLatin1Char( 'D' )));

				if (lsp.at(11).contains(QLatin1Char( 's' )))
					sub.setType(sub.type() | SKT_SIGNATURE);
				if (lsp.at(11).contains(QLatin1Char( 'e' )))
					sub.setType(sub.type() | SKT_ENCRYPTION);
				if (lsp.at(11).contains(QLatin1Char( 'e' )))
					sub.setType(sub.type() | SKT_AUTHENTICATION);
				if (lsp.at(11).contains(QLatin1Char( 'e' )))
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
		} else if (lsp.at(0) == QLatin1String( "uat" )) {
			m_numberid++;
			if (m_readNode != NULL) {
				m_currentSNode = new KGpgUatNode(m_readNode, m_numberid, lsp);
			}
		} else if ((lsp.at(0) == QLatin1String( "uid" )) && (items >= 10)) {
			if (m_numberid == 0) {
				QString fullname(lsp.at(9));
				QString kmail;
				if (fullname.contains(QLatin1Char( '<' )) ) {
					kmail = fullname;

					if (fullname.contains(QLatin1Char( ')' )) )
						kmail = kmail.section(QLatin1Char( ')' ), 1);

					kmail = kmail.section(QLatin1Char( '<' ), 1);
					kmail.truncate(kmail.length() - 1);

					if (kmail.contains(QLatin1Char( '<' ))) {
						// several email addresses in the same key
						kmail = kmail.replace(QLatin1Char( '>' ), QLatin1Char( ';' ));
						kmail.remove(QLatin1Char( '<' ));
					}
				}

				QString kname(fullname.section( QLatin1String( " <" ), 0, 0));
				QString comment;
				if (fullname.contains(QLatin1Char( '(' )) ) {
					kname = kname.section( QLatin1String( " (" ), 0, 0);
					comment = fullname.section(QLatin1Char( '(' ), 1, 1);
					comment = comment.section(QLatin1Char( ')' ), 0, 0);
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
		} else if (((lsp.at(0) == QLatin1String( "sig" )) || (lsp.at(0) == QLatin1String( "rev" ))) && (items >= 11)) {
			// there are no strings here that could have a recoded QLatin1Char( ':' ) in them
			const QString signature = lsp.join(QLatin1String(":"));

			if (m_currentSNode != NULL)
				(void) new KGpgSignNode(m_currentSNode, lsp);
		} else {
			log += lsp.join(QString(QLatin1Char( ':' ))) + QLatin1Char( '\n' );
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
	*process << QLatin1String( "--with-colons" ) << QLatin1String( "--list-secret-keys" ) << QLatin1String( "--with-fingerprint" ) << QLatin1String( "--fixed-list-mode" );

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
		if ((lsp.at(0) == QLatin1String( "sec" )) && (items >= 10)) {
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
		} else if ((lsp.at(0) == QLatin1String( "uid" )) && (items >= 10)) {
			if (hasuid)
				continue;

			hasuid = true;

			const QString fullname(lsp.at(9));
			if (fullname.contains(QLatin1Char( '<' ) )) {
				QString kmail(fullname);

				if (fullname.contains(QLatin1Char( ')' ) ))
					kmail = kmail.section(QLatin1Char( ')' ), 1);

				kmail = kmail.section(QLatin1Char( '<' ), 1);
				kmail.truncate(kmail.length() - 1);

				if (kmail.contains(QLatin1Char( '<' ) )) { // several email addresses in the same key
					kmail = kmail.replace(QLatin1Char( '>' ), QLatin1Char( ';' ));
					kmail.remove(QLatin1Char( '<' ));
				}

				m_secretkey.setEmail(kmail);
			} else {
				m_secretkey.setEmail(QString());
			}

			QString kname(fullname.section( QLatin1String( " <" ), 0, 0));
			if (fullname.contains(QLatin1Char( '(' ) )) {
				kname = kname.section( QLatin1String( " (" ), 0, 0);
				QString comment = fullname.section(QLatin1Char( '(' ), 1, 1);
				comment = comment.section(QLatin1Char( ')' ), 0, 0);

				m_secretkey.setComment(comment);
			} else {
				m_secretkey.setComment(QString());
			}
			m_secretkey.setName(kname);
		} else if ((lsp.at(0) == QLatin1String( "fpr" )) && (items >= 10)) {
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
	workProcess << QLatin1String( "--no-greeting" ) << QLatin1String( "--status-fd=2" );
        workProcess << QLatin1String( "--photo-viewer" ) << pgpgoutput << QLatin1String( "--edit-key" ) << keyid << QLatin1String( "uid" ) << uid << QLatin1String( "showphoto" ) << QLatin1String( "quit" );

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
