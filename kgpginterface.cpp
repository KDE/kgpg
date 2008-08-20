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
#include <KUrl>

#include "detailedconsole.h"
#include "emailvalidator.h"
#include "kgpgsettings.h"
#include "convert.h"
#include "gpgproc.h"

using namespace KgpgCore;

KgpgInterface::KgpgInterface() : editprocess(0)
{
}

KgpgInterface::~KgpgInterface()
{
    if (editprocess != 0)
    {
        editprocess->kill();
        delete editprocess;
    }

    const QObjectList &list = children();
    // First, we block all signals
    for (int i = 0; i < list.size(); ++i)
        list.at(i)->blockSignals(true);

    // Then, we delete objects
    for (int i = 0; i < list.size(); ++i)
        delete list.at(i);
}

QString KgpgInterface::checkForUtf8(QString txt)
{
    /* The string is not in UTF-8 */
    if (!txt.contains("\\x"))
        return txt;

    // if (!strchr (txt.toAscii(), 0xc3) || (txt.find("\\x")!=-1)) {
    for (int idx = 0; (idx = txt.indexOf( "\\x", idx )) >= 0 ; ++idx)
    {
        char str[2] = "x";
        str[0] = (char) QString(txt.mid(idx + 2, 2)).toShort(0, 16);
        txt.replace(idx, 4, str);
    }

    return QString::fromUtf8(txt.toAscii());
}

QString KgpgInterface::checkForUtf8bis(QString txt)
{
    if (strchr(txt.toAscii(), 0xc3) || txt.contains("\\x"))
        txt = checkForUtf8(txt);
    else
    {
        txt = checkForUtf8(txt);
        txt = QString::fromUtf8(txt.toAscii());
    }
    return txt;
}

int KgpgInterface::gpgVersion()
{
    GPGProc process;
    process << "--version";
    process.start();
    process.waitForFinished(-1);

    if (process.exitCode() == 255)
       return -1;

    QString line;
    if (process.readln(line) != -1)
        line = line.simplified().section(' ', -1);

    QStringList values = line.split('.');
    if (values.count() < 3)
       return -2;

    return (100 * values[0].toInt() + 10 * values[1].toInt() + values[2].toInt());
}

QStringList KgpgInterface::getGpgGroupNames(const QString &configfile)
{
    QStringList groups;
    QFile qfile(QFile::encodeName(configfile));
    if (qfile.open(QIODevice::ReadOnly) && (qfile.exists()))
    {
        QTextStream t(&qfile);
        while (!t.atEnd())
        {
            QString result = t.readLine().simplified();
            if (result.startsWith("group "))
            {
                result.remove(0, 6);
                groups << result.section("=", 0, 0).simplified();
            }
        }
        qfile.close();
    }
    return groups;
}

QStringList KgpgInterface::getGpgGroupSetting(const QString &name, const QString &configfile)
{
    QFile qfile(QFile::encodeName(configfile));
    if (qfile.open(QIODevice::ReadOnly) && (qfile.exists()))
    {
        QTextStream t(&qfile);
        while (!t.atEnd())
        {
    	    QString result = t.readLine().simplified();

            if (result.startsWith("group "))
            {
                kDebug(2100) << "Found 1 group" ;
                result.remove(0, 6);
                if (result.simplified().startsWith(name))
                {
                    kDebug(2100) << "Found group: " << name ;
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
    QString texttowrite;
    bool found = false;
    QFile qfile(QFile::encodeName(configfile));

    kDebug(2100) << "Changing group: " << name ;
    if (qfile.open(QIODevice::ReadOnly) && (qfile.exists()))
    {
        QTextStream t(&qfile);
        while (!t.atEnd())
        {
	    QString result = t.readLine();
            if (result.simplified().startsWith("group "))
            {
                QString result2 = result.simplified();
                result2.remove(0, 6);
                result2 = result2.simplified();
                if (result2.startsWith(name) && (result2.remove(0, name.length()).simplified().startsWith('=')))
                {
                    // kDebug(2100) << "Found group: " << name ;
                    // kDebug(2100) << "New values: " << values ;
                    result = QString("group %1=%2").arg(name).arg(values.join(" "));
                    found = true;
                }
            }
            texttowrite += result + '\n';
        }
        qfile.close();

        if (!found)
            texttowrite += '\n' + QString("group %1=%2").arg(name).arg(values.join(" "));

        if (qfile.open(QIODevice::WriteOnly))
        {
            QTextStream t(&qfile);
            t << texttowrite;
            qfile.close();
        }
    }
}

void KgpgInterface::delGpgGroup(const QString &name, const QString &configfile)
{
    QString texttowrite;
    QFile qfile(QFile::encodeName(configfile));
    if (qfile.open(QIODevice::ReadOnly) && (qfile.exists()))
    {
        QTextStream t(&qfile);
        while (!t.atEnd())
        {
	    QString result = t.readLine();
            if (result.simplified().startsWith("group "))
            {
                QString result2 = result.simplified();
                result2.remove(0, 6);
                result2 = result2.simplified();
                if (result2.startsWith(name) && (result2.remove(0, name.length()).simplified().startsWith('=')))
                    result.clear();
            }

            texttowrite += result + '\n';
        }

        qfile.close();
        if (qfile.open(QIODevice::WriteOnly))
        {
            QTextStream t(&qfile);
            t << texttowrite;
            qfile.close();
        }
    }
}

QString KgpgInterface::getGpgSetting(QString name, const QString &configfile)
{
    name = name.simplified() + ' ';
    QFile qfile(QFile::encodeName(configfile));
    if (qfile.open(QIODevice::ReadOnly) && (qfile.exists()))
    {
        QTextStream t(&qfile);
        while (!t.atEnd())
        {
            QString result = t.readLine();
            if (result.simplified().startsWith(name))
            {
                result = result.simplified();
                result.remove(0, name.length());
                result = result.simplified();
                return result.section(" ", 0, 0);
            }
        }
        qfile.close();
    }

    return QString();
}

void KgpgInterface::setGpgSetting(const QString &name, const QString &value, const QString &url)
{
    QString temp = name + ' ';
    QString texttowrite;
    bool found = false;
    QFile qfile(QFile::encodeName(url));

    if (qfile.open(QIODevice::ReadOnly) && (qfile.exists()))
    {
        QTextStream t(&qfile);
        while (!t.atEnd())
        {
	    QString result = t.readLine();
            if (result.simplified().startsWith(temp))
            {
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

        if (qfile.open(QIODevice::WriteOnly))
        {
            QTextStream t(&qfile);
            t << texttowrite;
            qfile.close();
        }
    }
}

bool KgpgInterface::getGpgBoolSetting(const QString &name, const QString &configfile)
{
    QFile qfile(QFile::encodeName(configfile));
    if (qfile.open(QIODevice::ReadOnly) && (qfile.exists()))
    {
        QTextStream t(&qfile);
        while (!t.atEnd())
        {
            if (t.readLine().simplified().startsWith(name))
                return true;
        }
        qfile.close();
    }
    return false;
}

void KgpgInterface::setGpgBoolSetting(const QString &name, const bool &enable, const QString &url)
{
    QString texttowrite;
    bool found = false;
    QFile qfile(QFile::encodeName(url));

    if (qfile.open(QIODevice::ReadOnly) && (qfile.exists()))
    {
        QTextStream t(&qfile);

        while (!t.atEnd())
        {
	    QString result = t.readLine();

            if (result.simplified().startsWith(name))
            {
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

        if (qfile.open(QIODevice::WriteOnly))
        {
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

    if (!userIDs.contains(txt))
    {
        if (!userIDs.isEmpty())
            userIDs += i18n(" or ");
        userIDs += txt;
    }
}

KgpgKeyList KgpgInterface::readPublicKeys(const bool &block, const QStringList &ids, const bool &withsigs)
{
    m_publiclistkeys = KgpgKeyList();
    m_publickey = KgpgKey();
    m_numberid = 1;
    cycle = "none";

    GPGProc *process = new GPGProc(this);
    *process << "--with-colons" << "--with-fingerprint";
    if (withsigs)
        *process << "--list-sigs";
    else
        *process << "--list-keys";

    *process << ids;

    if (!block)
    {
        connect(process, SIGNAL(readReady(GPGProc *)), this, SLOT(readPublicKeysProcess(GPGProc *)));
        connect(process, SIGNAL(processExited(GPGProc *)), this, SLOT(readPublicKeysFin(GPGProc *)));
        process->start();
        return KgpgKeyList();
    }
    else
    {
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

    while ((items = p->readln(lsp)) >= 0)
    {
        if ((lsp.at(0) == "pub") && (items >= 10))
        {
            if (cycle != "none")
            {
                cycle = "none";
                m_publiclistkeys << m_publickey;
            }

            m_publickey = KgpgKey();

            m_publickey.setTrust(Convert::toTrust(lsp.at(1)));
            m_publickey.setSize(lsp.at(2).toUInt());
            m_publickey.setAlgorithm(Convert::toAlgo(lsp.at(3).toInt()));
            m_publickey.setFingerprint(lsp.at(4));
            m_publickey.setCreation(QDate::fromString(lsp.at(5), Qt::ISODate));
            m_publickey.setOwnerTrust(Convert::toOwnerTrust(lsp.at(8)));

            if (lsp.at(6).isEmpty())
            {
                m_publickey.setExpiration(QDate());
            }
            else
            {
                m_publickey.setExpiration(QDate::fromString(lsp.at(6), Qt::ISODate));
            }

            if (lsp.at(11).contains("D", Qt::CaseSensitive))  // disabled key
                m_publickey.setValid(false);
            else
                m_publickey.setValid(true);

            QString fullname = lsp.at(9);
            if (fullname.contains("<"))
            {
                QString kmail = fullname;

                if (fullname.contains(')') )
                    kmail = kmail.section(')', 1);

                kmail = kmail.section('<', 1);
                kmail.truncate(kmail.length() - 1);

                if (kmail.contains('<') ) // several email addresses in the same key
                {
                    kmail = kmail.replace('>', ';');
                    kmail.remove('<');
                }

                m_publickey.setEmail(kmail);
            }
            else
                m_publickey.setEmail(QString());

            QString kname = fullname.section(" <", 0, 0);
            if (fullname.contains('(') )
            {
                kname = kname.section(" (", 0, 0);
                QString comment = fullname.section('(', 1, 1);
                comment = comment.section(')', 0, 0);

                m_publickey.setComment(comment);
            }
            else
                m_publickey.setComment(QString());
            m_publickey.setName(kname);

            cycle = "pub";

            // the first uid is merged into the public key
            m_numberid = 1;
        }
        else
        if ((lsp.at(0) == "fpr") && (items >= 10))
        {
            QString fingervalue = lsp.at(9);

            m_publickey.setFingerprint(fingervalue);
        }
        else
        if ((lsp.at(0) == "sub") && (items >= 7))
        {
            KgpgKeySub sub;

            sub.setId(lsp.at(4).right(8));
            sub.setTrust(Convert::toTrust(lsp.at(1)));
            sub.setSize(lsp.at(2).toUInt());
            sub.setAlgorithm(Convert::toAlgo(lsp.at(3).toInt()));
            sub.setCreation(QDate::fromString(lsp.at(5), Qt::ISODate));

            // FIXME: Please see kgpgkey.h, KgpgSubKey class
            if (lsp.at(11).contains('D'))
                sub.setValid(false);
            else
                sub.setValid(true);

            if (lsp.at(11).contains('s'))
                sub.setType(SKT_SIGNATURE);
            else
            if (lsp.at(11).contains('e'))
                sub.setType(SKT_ENCRYPTION);

            if (lsp.at(6).isEmpty())
            {
                sub.setExpiration(QDate());
            }
            else
            {
                sub.setExpiration(QDate::fromString(lsp.at(6), Qt::ISODate));
            }

            m_publickey.subList()->append(sub);
            cycle = "sub";
        }
        else
        if (lsp.at(0) == "uat")
        {
            m_numberid++;
            KgpgKeyUat uat;
            uat.setId(QString::number(m_numberid));
            uat.setCreation(QDate::fromString(lsp.at(5), Qt::ISODate));
            m_publickey.uatList()->append(uat);

            cycle = "uat";
        }
        else
        if ((lsp.at(0) == "uid") && (items >= 10))
        {
            KgpgKeyUid uid;

            uid.setTrust(Convert::toTrust(lsp.at(1)));
            if ((items > 11) && lsp.at(11).contains('D'))
                uid.setValid(false);
            else
                uid.setValid(true);

            uid.setIndex(++m_numberid);
            QString fullname = lsp.at(9);
            if (fullname.contains('<') )
            {
                QString kmail = fullname;

                if (fullname.contains(')') )
                    kmail = kmail.section(')', 1);

                kmail = kmail.section('<', 1);
                kmail.truncate(kmail.length() - 1);

                if ( kmail.contains('<') ) // several email addresses in the same key
                {
                    kmail = kmail.replace('>', ';');
                    kmail.remove('<');
                }

                uid.setEmail(kmail);
            }
            else
                uid.setEmail(QString());

            QString kname = fullname.section(" <", 0, 0);
            if (fullname.contains('(') )
            {
                kname = kname.section(" (", 0, 0);
                QString comment = fullname.section('(', 1, 1);
                comment = comment.section(')', 0, 0);

                uid.setComment(comment);
            }
            else
                uid.setComment(QString());
            uid.setName(kname);

            m_publickey.uidList()->append(uid);

            cycle = "uid";
        }
        else
        if (((lsp.at(0) == "sig") || (lsp.at(0) == "rev")) && (items >= 11))
        {
            KgpgKeySign signature;

            signature.setId(lsp.at(4));
            signature.setCreation(QDate::fromString(lsp.at(5), Qt::ISODate));

            if (lsp.at(6).isEmpty())
            {
                signature.setExpiration(QDate());
            }
            else
            {
                signature.setExpiration(QDate::fromString(lsp.at(6), Qt::ISODate));
            }

            QString fullname = lsp.at(9);
            if (fullname.contains('<') )
            {
                QString kmail = fullname;

                if (fullname.contains(')') )
                    kmail = kmail.section(')', 1);

                kmail = kmail.section('<', 1);
                kmail.truncate(kmail.length() - 1);

                if (kmail.contains('<' )) // several email addresses in the same key
                {
                    kmail = kmail.replace('>', ';');
                    kmail.remove('<');
                }

                signature.setEmail(kmail);
            }
            else
                signature.setEmail(QString());

            QString kname = fullname.section(" <", 0, 0);
            if (fullname.contains('(' ))
            {
                kname = kname.section(" (", 0, 0);
                QString comment = fullname.section('(', 1, 1);
                comment = comment.section(')', 0, 0);

                signature.setComment(comment);
            }
            else
                signature.setComment(QString());
            signature.setName(kname);

            if (lsp.at(10).endsWith('l'))
                signature.setLocal(true);

            if (lsp.at(0) == "rev")
                signature.setRevocation(true);

            if (cycle == "pub")
                m_publickey.addSign(signature);
            else
            if (cycle == "uat")
                m_publickey.uatList()->last().addSign(signature);
            else
            if (cycle == "uid")
                m_publickey.uidList()->last().addSign(signature);
            else
            if (cycle == "sub")
                m_publickey.subList()->last().addSign(signature);
        }
    }
}

void KgpgInterface::readPublicKeysFin(GPGProc *p, const bool &block)
{
    // insert the last key
    if (cycle != "none")
        m_publiclistkeys << m_publickey;

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
        *process << "--with-colons" << "--list-secret-keys" << "--with-fingerprint";

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

    while ( (items = p->readln(lsp)) >= 0 )
    {
            if ((lsp.at(0) == "sec") && (items >= 10))
            {
                if (m_secretactivate)
                    m_secretlistkeys << m_secretkey;

                m_secretactivate = true;
                m_secretkey = KgpgKey();

                m_secretkey.setTrust(Convert::toTrust(lsp.at(1)));
                m_secretkey.setSize(lsp.at(2).toUInt());
                m_secretkey.setAlgorithm(Convert::toAlgo(lsp.at(3).toInt()));
                m_secretkey.setFingerprint(lsp.at(4));
                m_secretkey.setCreation(QDate::fromString(lsp[5], Qt::ISODate));
                m_secretkey.setSecret(true);

                if (lsp.at(6).isEmpty())
                {
                    m_secretkey.setExpiration(QDate());
                }
                else
                {
                    m_secretkey.setExpiration(QDate::fromString(lsp.at(6), Qt::ISODate));
                }

                QString fullname = lsp.at(9);
                if (fullname.contains('<' ))
                {
                    QString kmail = fullname;

                    if (fullname.contains(')' ))
                        kmail = kmail.section(')', 1);

                    kmail = kmail.section('<', 1);
                    kmail.truncate(kmail.length() - 1);

                    if (kmail.contains('<' )) // several email addresses in the same key
                    {
                        kmail = kmail.replace('>', ';');
                        kmail.remove('<');
                    }

                    m_secretkey.setEmail(kmail);
                }
                else
                    m_secretkey.setEmail(QString());

                QString kname = fullname.section(" <", 0, 0);
                if (fullname.contains('(' ))
                {
                    kname = kname.section(" (", 0, 0);
                    QString comment = fullname.section('(', 1, 1);
                    comment = comment.section(')', 0, 0);

                    m_secretkey.setComment(comment);
                }
                else
                    m_secretkey.setComment(QString());
                m_secretkey.setName(kname);
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

QString KgpgInterface::getKeys(const QString *attributes, const QStringList &ids)
{
	m_keystring.clear();

	GPGProc *gpgProcess = new GPGProc(this);
	*gpgProcess << "--export" << "--armor" << "--status-fd=1" << "--command-fd=0";

	if (attributes)
		*gpgProcess << "--export-options" << *attributes;

	*gpgProcess << ids;

	connect(gpgProcess, SIGNAL(readReady(GPGProc *)), this, SLOT(getKeysProcess(GPGProc *)));
	gpgProcess->start();
	gpgProcess->waitForFinished(-1);
	delete gpgProcess;
	return m_keystring;
}

void KgpgInterface::getKeysProcess(GPGProc *gpgProcess)
{
	QString line;

	while (gpgProcess->readln(line, true) >= 0) {
		if (!line.startsWith("gpg:"))
			m_keystring += line + '\n';
	}
}

void KgpgInterface::getKeysFin(GPGProc *gpgProcess)
{
	gpgProcess->deleteLater();
	emit getKeysFinished(m_keystring, this);
}

void KgpgInterface::signKey(const QString &keyid, const QString &signkeyid, const bool &local, const int &checking, const bool &terminal, const QString &uid)
{
    m_partialline.clear();
    m_ispartial = false;
    log.clear();
    m_signkey = signkeyid;
    m_keyid = keyid;
    m_checking = checking;
    m_local = local;
    step = 3;

    if (terminal)
    {
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
        QString buffer = m_partialline + m_workProcess->readAllStandardOutput();

        while (buffer.contains('\n')) {
            int pos = buffer.indexOf('\n');
            QString line = buffer.left(pos);
            buffer.remove(0, pos + 1);

            if (line.startsWith("[GNUPG:]"))
            {
                if (line.contains("USERID_HINT"))
                    updateIDs(line);
                else
                if (m_success == 3)
                {
                    // user has aborted the process and don't want to sign the key
                    if (line.contains("GET_"))
                        m_workProcess->write("quit\n");
                    return;
                }
                else
                if (line.contains("ALREADY_SIGNED"))
                    m_success = 4;
                if (line.contains("GOOD_PASSPHRASE"))
                {
                    m_success = 2;
                }
                else
                if (line.contains("sign_uid.expire"))
                    m_workProcess->write("Never\n");
                else
                if (line.contains("sign_uid.class"))
                    m_workProcess->write(QString::number(m_checking).toAscii() + '\n');
                else
                if (line.contains("sign_uid.okay"))
                    m_workProcess->write("Y\n");
                else
                if (line.contains("sign_all.okay"))
                    m_workProcess->write("Y\n");
                else
                if (line.contains("passphrase.enter"))
                {
                    QString passdlgmessage;
                    if (step < 3)
                        passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
                    passdlgmessage += i18n("Enter passphrase for <b>%1</b>", checkForUtf8bis(userIDs));

                    if (sendPassphrase(passdlgmessage, m_workProcess, false))
                    {
                        m_success = 3;
                        m_workProcess->write("quit\n");
                        return;
                    }

                    if (step > 1)
                        step--;
                    else
                        step = 3;
                }
                else
                if ((m_success != 1) && line.contains("keyedit.prompt"))
                    m_workProcess->write("save\n");
                else
                if (line.contains("BAD_PASSPHRASE"))
                    m_success = 1;
                else
                if (line.contains("GET_")) // gpg asks for something unusal, turn to konsole mode
                {
                    if (m_success != 1)
                        m_success = 5; // switching to console mode
                    m_workProcess->write("quit\n");
                }
            }
            else
                log += line + '\n';
        }
        m_partialline = buffer;
}

void KgpgInterface::signKeyFin()
{
    m_workProcess->deleteLater();
    if ((m_success != 0) && (m_success != 5))
        emit signKeyFinished(m_success, m_keyid, this); // signature successful or bad passphrase or aborted or already signed
    else
    {
        KgpgDetailedConsole *q = new KgpgDetailedConsole(0, i18n("<qt>Signing key <b>%1</b> with key <b>%2</b> failed.<br />Do you want to try signing the key in console mode?</qt>", m_keyid, m_signkey), log);
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

void KgpgInterface::keyExpire(const QString &keyid, const QDate &date)
{
    m_partialline.clear();
    m_ispartial = false;
    log.clear();
    m_success = 0;
    step = 3;

    if (date.isNull())
        expirationDelay = 0;
    else
        expirationDelay = QDate::currentDate().daysTo(date);

	m_workProcess = new KProcess(this);
	m_workProcess->setOutputChannelMode(KProcess::OnlyStdoutChannel);
	*m_workProcess << KGpgSettings::gpgBinaryPath();
	*m_workProcess << "--no-secmem-warning" << "--no-tty" << "--status-fd=1" << "--command-fd=0";
	*m_workProcess << "--edit-key" << keyid << "expire";

	kDebug(2100) << "Change expiration of the key" << keyid << "to" << expirationDelay << "days";
	connect(m_workProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(keyExpireProcess()));
	connect(m_workProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(keyExpireFin()));

	m_workProcess->start();
}

void KgpgInterface::keyExpireProcess()
{
        QString buffer = m_partialline + m_workProcess->readAllStandardOutput();

        while (buffer.contains('\n')) {
            int pos = buffer.indexOf('\n');
            QString line = buffer.left(pos);
            buffer.remove(0, pos + 1);

            if (!line.startsWith("[GNUPG:]"))
                log += line + '\n';
            else
            if (m_success == 3)
            {
                if (line.contains("GET_" ))
                    m_workProcess->write("quit\n");
                return;
            }
            else
            if (line.contains("USERID_HINT"))
                updateIDs(line);
            else
            if (line.contains("GOOD_PASSPHRASE"))
            {
                m_success = 2;
            }
            else
            if (line.contains("keygen.valid"))
                m_workProcess->write(QString::number(expirationDelay).toAscii() + '\n');
            else
            if (line.contains("passphrase.enter"))
            {
                QString passdlgmessage;
                if (step < 3)
                    passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
                passdlgmessage += i18n("Enter passphrase for <b>%1</b>", checkForUtf8bis(userIDs));

                if (sendPassphrase(passdlgmessage, m_workProcess, false))
                {
                    m_success = 3;  // aborted by user mode
                    m_workProcess->write("quit\n");
                    return;
                }
                --step;
            }
            else
            if ((m_success == 2) && line.contains("keyedit.prompt"))
                m_workProcess->write("save\n");
            else
            if ((m_success == 2) && line.contains("keyedit.save.okay"))
                m_workProcess->write("YES\n");
            else
            if (line.contains("BAD_PASSPHRASE"))
            {
                m_success = 1; // bad passphrase
                m_workProcess->write("quit\n");
            }
            else
            if (line.contains("GET_")) // gpg asks for something unusal, turn to konsole mode
            {
                if (m_success != 1)
                    m_success = 4; // switching to console mode
                m_workProcess->write("quit\n");
            }
        }

}

void KgpgInterface::keyExpireFin()
{
    m_workProcess->deleteLater();
    if (m_success < 4)
        emit keyExpireFinished(m_success, this); // signature successful or bad passphrase
    else
    {
        KgpgDetailedConsole *q = new KgpgDetailedConsole(0, i18n("<qt><b>Changing expiration failed.</b><br />"
                                    "Do you want to try changing the key expiration in console mode?</qt>"),output);
        if (q->exec() == QDialog::Accepted)
            KMessageBox::sorry(0, i18n("work in progress..."));
        else
            emit keyExpireFinished(3, this);
    }
}

void KgpgInterface::changePass(const QString &keyid)
{
    m_partialline.clear();
    m_ispartial = false;
    m_success = 1;
    step = 3;

	m_workProcess = new KProcess(this);
	m_workProcess->setOutputChannelMode(KProcess::OnlyStdoutChannel);
	*m_workProcess << KGpgSettings::gpgBinaryPath();
	*m_workProcess << "--no-secmem-warning" << "--no-tty" << "--status-fd=1" << "--command-fd=0";
	*m_workProcess << "--edit-key" << keyid << "passwd";

	kDebug(2100) << "Change passphrase of the key" << keyid;
	connect(m_workProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(changePassProcess()));
	connect(m_workProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(changePassFin()));

	m_workProcess->start();
}

void KgpgInterface::changePassProcess()
{
        QString buffer = m_partialline + m_workProcess->readAllStandardOutput();

        while (buffer.contains('\n')) {
            int pos = buffer.indexOf('\n');
            QString line = buffer.left(pos);
            buffer.remove(0, pos + 1);

            if (line.contains("USERID_HINT"))
                updateIDs(line);
            else
            if ((m_success == 4) && line.contains("keyedit.prompt"))
            {
                m_success = 2;
                m_workProcess->write("save\n");
            }
            else
            if (line.contains("GOOD_PASSPHRASE"))
                m_success = 4;
            else
            if (line.contains("passphrase.enter"))
            {
                if (userIDs.isEmpty())
                    userIDs = i18n("[No user id found]");
                userIDs.replace('<', "&lt;");

                if (m_success == 1)
                {
                    QString passdlgmessage;
                    if (step < 3)
                        passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
                    passdlgmessage += i18n("Enter old passphrase for <b>%1</b>", checkForUtf8bis(userIDs));

                    if (sendPassphrase(passdlgmessage, m_workProcess, false))
                    {
                        m_workProcess->deleteLater();
                        emit changePassFinished(3, this);
                        return;
                    }
                    --step;
                }
                else
                if (m_success == 4)
                {
                    if (sendPassphrase(i18n("<qt>Enter new passphrase for <b>%1</b><br />If you forget this passphrase all your encrypted files and messages will be inaccessible<br /></qt>", userIDs), m_workProcess))
                    {
                        m_workProcess->deleteLater();
                        emit changePassFinished(3, this);
                        return;
                    }
                }
            }
            else
            if (line.contains("GET_")) // gpg asks for something unusal, turn to konsole mode
            {
                m_workProcess->write("quit\n");
            }
    }
}

void KgpgInterface::changePassFin()
{
    m_workProcess->deleteLater();
    emit changePassFinished(m_success, this);
}

void
KgpgInterface::changeTrust(const QString &keyid, const KgpgCore::KgpgKeyOwnerTrust &keytrust)
{
    m_partialline.clear();
    m_ispartial = false;
    m_trustvalue = keytrust;

	m_workProcess = new KProcess(this);
	m_workProcess->setOutputChannelMode(KProcess::OnlyStdoutChannel);
	*m_workProcess << KGpgSettings::gpgBinaryPath();
	*m_workProcess << "--no-secmem-warning" << "--no-tty" << "--status-fd=1" << "--command-fd=0";
	*m_workProcess << "--edit-key" << keyid << "trust";

	kDebug(2100) << "Change trust of the key" << keyid << "to" << keytrust;
	connect(m_workProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(changeTrustProcess()));
	connect(m_workProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(changeTrustFin()));

	m_workProcess->start();
}

void KgpgInterface::changeTrustProcess()
{
        QString buffer = m_partialline + m_workProcess->readAllStandardOutput();

        while (buffer.contains('\n')) {
            int pos = buffer.indexOf('\n');
            QString line = buffer.left(pos);
            buffer.remove(0, pos + 1);

            if (line.contains("edit_ownertrust.set_ultimate.okay"))
                m_workProcess->write("YES\n");
            else
            if (line.contains("edit_ownertrust.value"))
                m_workProcess->write(QString::number(m_trustvalue).toAscii() + '\n');
            else
            if (line.contains("keyedit.prompt"))
            {
                m_workProcess->write(QByteArray("save\n"));
            }
            else
            if (line.contains("GET_")) // gpg asks for something unusal
            {
                m_workProcess->write(QByteArray("quit\n"));
            }
    }
}

void KgpgInterface::changeTrustFin()
{
    m_workProcess->deleteLater();
    emit changeTrustFinished(this);
}

void KgpgInterface::changeDisable(const QString &keyid, const bool &ison)
{
    m_partialline.clear();
    m_ispartial = false;

    editprocess = new KProcess(this);
    *editprocess << KGpgSettings::gpgBinaryPath() << "--no-tty" << "--no-greeting" << "--edit-key" << keyid;

    if (ison)
        *editprocess << "disable";
    else
        *editprocess << "enable";

    *editprocess << "save";

    kDebug(2100) << "Change disable of the key" << keyid << "to" << ison;
    connect(editprocess, SIGNAL(finished(int)), this, SLOT(changeDisableFin(int)));
    editprocess->start();
}

void KgpgInterface::changeDisableFin(int res)
{
    editprocess->deleteLater();
    editprocess = 0;
    emit changeDisableFinished(this, res);
}

QPixmap KgpgInterface::loadPhoto(const QString &keyid, const QString &uid, const bool &block)
{
	QString pgpgoutput = "cat %i";

	m_workProcess = new KProcess(this);
	m_workProcess->setOutputChannelMode(KProcess::OnlyStdoutChannel);
	*m_workProcess << KGpgSettings::gpgBinaryPath();
	*m_workProcess << "--no-secmem-warning" << "--no-tty" << "--status-fd=2";
	*m_workProcess << "--photo-viewer" << pgpgoutput << "--edit-key" << keyid << "uid" << uid << "showphoto" << "quit";

	connect(m_workProcess, SIGNAL(finished(int)), this, SLOT(loadPhotoFin(int)));
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
		QByteArray pic = m_workProcess->readAllStandardOutput();
		m_pixmap.loadFromData(pic);
	}

	emit loadPhotoFinished(m_pixmap, this);
}

void KgpgInterface::addPhoto(const QString &keyid, const QString &imagepath)
{
	photoUrl = imagepath;
	m_success = 0;
	step = 3;

	GPGProc *process = new GPGProc(this);
	*process << "--status-fd=1" << "--command-fd=0" << "--edit-key" << keyid << "addphoto";

	kDebug(2100) << "Add the photo" << imagepath << "to the key" << keyid;
	connect(process, SIGNAL(readReady(GPGProc *)), this, SLOT(addPhotoProcess(GPGProc *)));
	connect(process, SIGNAL(processExited(GPGProc *)), this, SLOT(addPhotoFin(GPGProc *)));
	process->start();
}

void KgpgInterface::addPhotoProcess(GPGProc *p)
{
    QString line;

    while (p->readln(line, true) >= 0) {

            if (line.contains("USERID_HINT"))
                updateIDs(line);
            else
            if (line.contains("BAD_PASSPHRASE"))
                m_success = 1;
            else
            if (line.contains("GOOD_PASSPHRASE"))
                m_success = 2;
            if (line.contains("photoid.jpeg.add"))
                p->write(photoUrl.toAscii() + '\n');
            else
            if (line.contains("photoid.jpeg.size"))
            {
                if (KMessageBox::questionYesNo(0, i18n("This image is very large. Use it anyway?"), QString(), KGuiItem(i18n("Use Anyway")), KGuiItem(i18n("Do Not Use"))) == KMessageBox::Yes)
                    p->write("Yes\n");
                else
                {
                    p->deleteLater();
                    emit addPhotoFinished(3, this);
                    return;
                }
            }
            else
            if (line.contains("passphrase.enter"))
            {
                QString passdlgmessage;
                if (step < 3)
                    passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
                passdlgmessage += i18n("Enter passphrase for <b>%1</b>", checkForUtf8bis(userIDs));

                if (sendPassphrase(passdlgmessage, p, false))
                {
                    p->deleteLater();
                    emit addPhotoFinished(3, this);
                    return;
                }

                step--;
            }
            else
            if ((m_success == 2) && (line.contains("keyedit.prompt")))
                p->write("save\n");
            else
            if ((line.contains("GET_"))) // gpg asks for something unusal, turn to konsole mode
            {
                p->write("quit\n");
            }
    }
}

void KgpgInterface::addPhotoFin(GPGProc *p)
{
    p->deleteLater();
    emit addPhotoFinished(m_success, this);
}

void KgpgInterface::deletePhoto(const QString &keyid, const QString &uid)
{
    m_success = 0;
    step = 3;

    GPGProc *process = new GPGProc(this);
    *process << "--status-fd=1" << "--command-fd=0" << "--edit-key" << keyid << "uid" << uid << "deluid";

    kDebug(2100) << "Delete a photo from the key" << keyid;
    connect(process, SIGNAL(readReady(GPGProc *)), this, SLOT(deletePhotoProcess(GPGProc *)));
    connect(process, SIGNAL(processExited(GPGProc *)), this, SLOT(deletePhotoFin(GPGProc *)));
    process->start();
}

void KgpgInterface::deletePhotoProcess(GPGProc *p)
{
    QString line;

    while (p->readln(line, true) >= 0) {
            if (line.contains("USERID_HINT"))
                updateIDs(line);
            else
            if (line.contains("BAD_PASSPHRASE"))
                m_success = 1;
            else
            if (line.contains("GOOD_PASSPHRASE"))
                m_success = 2;
            else
            if (line.contains("keyedit.remove.uid.okay"))
                p->write("YES\n");
            else
            if (line.contains("passphrase.enter"))
            {
                QString passdlgmessage;
                if (step < 3)
                    passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
                passdlgmessage += i18n("Enter passphrase for <b>%1</b>", checkForUtf8bis(userIDs));

                if (sendPassphrase(passdlgmessage, p, false))
                {
                    p->deleteLater();
                    emit deletePhotoFinished(3, this);
                    return;
                }
            }
            else
            if (line.contains("keyedit.prompt"))
                p->write("save\n");
            else
            if (line.contains("GET_")) // gpg asks for something unusal, turn to konsole mode
            {
                p->write("quit\n");
            }
    }
}

void KgpgInterface::deletePhotoFin(GPGProc *p)
{
    p->deleteLater();
    emit deletePhotoFinished(m_success, this);
}

void KgpgInterface::importKey(const QString &keystr)
{
    message.clear();

    GPGProc *process = new GPGProc(this);
    *process << "--status-fd=1";
    *process << "--import";
    *process << "--allow-secret-key-import";

    connect(process, SIGNAL(readReady(GPGProc *)), this, SLOT(importKeyProcess(GPGProc *)));
    connect(process, SIGNAL(processExited(GPGProc *)), this, SLOT(importKeyFinished(GPGProc *)));
    process->start();

    process->write(keystr.toAscii() + '\n');
    process->closeWriteChannel();
}

void KgpgInterface::importKey(const KUrl &url)
{
	message.clear();

	GPGProc *process = new GPGProc(this);
	*process << "--status-fd=1";
	*process << "--import";
	*process << "--allow-secret-key-import";
	connect(process, SIGNAL(readReady(GPGProc *)), this, SLOT(importKeyProcess(GPGProc *)));

	if ( url.isLocalFile() ) {
		*process << url.path();
		connect(process, SIGNAL(processExited(GPGProc *)), this, SLOT(importKeyFinished(GPGProc *)));
	} else {
		if (KIO::NetAccess::download(url, m_tempkeyfile, 0)) {
			*process << m_tempkeyfile;

			connect(process, SIGNAL(processExited(GPGProc *)), this, SLOT(importURLover(GPGProc *)));
		} else {
			delete process;
			KMessageBox::error(0, KIO::NetAccess::lastErrorString() );
			return;
		}
	}
	process->start();
}

void KgpgInterface::importURLover(GPGProc *p)
{
    KIO::NetAccess::removeTempFile(m_tempkeyfile);
    emit importKeyFinished(p);
}

void KgpgInterface::importKeyProcess(GPGProc *p)
{
    QString line;

    while (p->readln(line, true) >= 0) {
            if (!line.contains("http-proxy"))
                message += line + '\n';
    }
}

void KgpgInterface::importKeyFinished(GPGProc *p)
{
    p->deleteLater();

    QStringList importedKeysIds;
    QStringList importedKeys;
    QString resultMessage;
    bool secretImport = false;

    QString parsedOutput = message;

    while (parsedOutput.contains("IMPORTED"))
    {
        parsedOutput.remove(0, parsedOutput.indexOf("IMPORTED") + 8);
        importedKeys << parsedOutput.section("\n", 0, 0).simplified();
        importedKeysIds << parsedOutput.simplified().section(' ', 0, 0);
    }

    if (message.contains("IMPORT_RES"))
    {
        parsedOutput = message.section("IMPORT_RES", -1, -1).simplified();
        QStringList messageList = parsedOutput.split(' ');

        resultMessage = i18np("<qt>%1 key processed.</qt>", "<qt>%1 keys processed.</qt>", messageList[0].toULong());

        if (messageList[1] != "0")
            resultMessage += i18np("<qt>One key without ID.</qt>", "<qt>%1 keys without ID.</qt>", messageList[1].toULong());
        if (messageList[2] != "0")
            resultMessage += i18np("<qt><b>One key imported:</b></qt>", "<qt><b>%1 keys imported:</b></qt>", messageList[2].toULong());
        if (messageList[3] != "0")
            resultMessage += i18np("<qt>One RSA key imported.</qt>", "<qt>%1 RSA keys imported.</qt>", messageList[3].toULong());
        if (messageList[4] != "0")
            resultMessage += i18np("<qt>One key unchanged.</qt>", "<qt>%1 keys unchanged.</qt>", messageList[4].toULong());
        if (messageList[5] != "0")
            resultMessage += i18np("<qt>One user ID imported.</qt>", "<qt>%1 user IDs imported.</qt>", messageList[5].toULong());
        if (messageList[6] != "0")
            resultMessage += i18np("<qt>One subkey imported.</qt>", "<qt>%1 subkeys imported.</qt>", messageList[6].toULong());
        if (messageList[7] != "0")
            resultMessage += i18np("<qt>One signature imported.</qt>", "<qt>%1 signatures imported.</qt>", messageList[7].toULong());
        if (messageList[8] != "0")
        {
            resultMessage += i18np("<qt>One revocation certificate imported.</qt>", "<qt>%1 revocation certificates imported.</qt>", messageList[8].toULong());
            // empty list means "reload all"
            importedKeysIds.clear();
        }
        if (messageList[9] != "0")
        {
            resultMessage += i18np("<qt>One secret key processed.</qt>", "<qt>%1 secret keys processed.</qt>", messageList[9].toULong());
            secretImport = true;
        }
        if (messageList[10] != "0")
            resultMessage += i18np("<qt><b>One secret key imported.</b></qt>", "<qt><b>%1 secret keys imported.</b></qt>", messageList[10].toULong());
        if (messageList[11] != "0")
            resultMessage += i18np("<qt>One secret key unchanged.</qt>", "<qt>%1 secret keys unchanged.</qt>", messageList[11].toULong());
        if (messageList[12] != "0")
            resultMessage += i18np("<qt>One secret key not imported.</qt>", "<qt>%1 secret keys not imported.</qt>", messageList[12].toULong());

        if (secretImport)
            resultMessage += i18n("<qt><br /><b>You have imported a secret key.</b> <br />"
                                  "Please note that imported secret keys are not trusted by default.<br />"
                                  "To fully use this secret key for signing and encryption, you must edit the key (double click on it) and set its trust to Full or Ultimate.</qt>");

        emit importKeyFinished(importedKeysIds);
   }
    else
        resultMessage = i18n("No key imported... \nCheck detailed log for more infos");

    // TODO : should be deleted. KgpgInterface should not show any dialog (but password).
    // When a message should be shown, it should be passed by parameter in a SIGNAL.
    (void) new KgpgDetailedInfo(0, resultMessage, message, importedKeys);
}

void KgpgInterface::addUid(const QString &keyid, const QString &name, const QString &email, const QString &comment)
{
	step = 3;
	m_success = 0;
	// why the heck does QValidator don't take const arguments?
	QString Email = email;

	int pos = 0;
	if (!email.isEmpty() && (EmailValidator().validate(Email, pos) == QValidator::Invalid)) {
		emit addUidFinished(4, this);
		return;
	}

	uidName = name;
	uidComment = comment;
	uidEmail = email;

	GPGProc *process = new GPGProc(this);
	*process << "--status-fd=1" << "--command-fd=0" << "--edit-key" << keyid << "adduid";

	connect(process, SIGNAL(readReady(GPGProc *)), this, SLOT(addUidProcess(GPGProc *)));
	connect(process, SIGNAL(processExited(GPGProc *)), this, SLOT(addUidFin(GPGProc *)));
	process->start();
}

void KgpgInterface::addUidProcess(GPGProc *p)
{
    QString line;

    while (p->readln(line, true) >= 0) {
            if (line.contains("USERID_HINT"))
                updateIDs(line);
            else
            if (line.contains("BAD_PASSPHRASE"))
                m_success = 1;
            else
            if (line.contains("GOOD_PASSPHRASE"))
                m_success = 2;
            else
            if (line.contains("keygen.name"))
                p->write(uidName.toAscii() + '\n');
            else
            if (line.contains("keygen.email"))
                p->write(uidEmail.toAscii() + '\n');
            else
            if (line.contains("keygen.comment"))
                p->write(uidComment.toAscii() + '\n');
            else
            if (line.contains("passphrase.enter"))
            {
                QString passdlgmessage;
                if (step < 3)
                    passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
                passdlgmessage += i18n("Enter passphrase for <b>%1</b>", checkForUtf8bis(userIDs));

                if (sendPassphrase(passdlgmessage, p, false))
                {
                    p->deleteLater();
                    emit addUidFinished(3, this);
                    return;
                }
                step--;
            }
            else
            if (line.contains("keyedit.prompt"))
                p->write("save\n");
            else
            if (line.contains("GET_")) // gpg asks for something unusal, turn to konsole mode
            {
                p->write("quit\n");
            }
    }
}

void KgpgInterface::addUidFin(GPGProc *p)
{
    p->deleteLater();
    emit addUidFinished(m_success, this);
}

void
KgpgInterface::generateKey(const QString &keyname, const QString &keyemail, const QString &keycomment, const KgpgCore::KgpgKeyAlgo &keyalgo, const uint &keysize, const uint &keyexp, const uint &keyexpnumber)
{
    step = 3;
    m_success = 0;
    QString Email = keyemail;

	int pos = 0;
	if (!keyemail.isEmpty() && (EmailValidator().validate(Email, pos) == QValidator::Invalid)) {
		emit generateKeyFinished(4, this, keyname, keyemail, QString());
		return;
	}

    m_newfingerprint.clear();
    m_keyname = keyname;
    m_keyemail = keyemail;
    m_keycomment = keycomment;
    m_keyalgo = keyalgo;
    m_keysize = keysize;
    m_keyexp = keyexp;
    m_keyexpnumber = keyexpnumber;

    GPGProc *process = new GPGProc(this);
    *process << "--command-fd=0" << "--status-fd=1" << "--no-verbose" << "--no-greeting";
    *process << "--gen-key";

    kDebug(2100) << "Generate a new key-pair";
    connect(process, SIGNAL(readReady(GPGProc *)), this, SLOT(generateKeyProcess(GPGProc *)));
    connect(process, SIGNAL(processExited(GPGProc *)), this, SLOT(generateKeyFin(GPGProc *)));
    process->start();
}

void KgpgInterface::generateKeyProcess(GPGProc *p)
{
    QString line;

    while (p->readln(line, true) >= 0) {

            if (line.contains("BAD_PASSPHRASE"))
                m_success = 1;
            else
            if ((m_success == 5) && line.contains("PROGRESS"))
            {
                m_success = 0;
                emit generateKeyStarted(this);
            }
            else
            if (line.contains("keygen.algo"))
            {
                if (m_keyalgo == ALGO_RSA)
                    p->write("5\n");
                else
                    p->write("1\n");
            }
            else
            if (line.contains("keygen.size"))
                p->write(QString::number(m_keysize).toAscii() + '\n');
            else
            if (line.contains("keygen.valid"))
            {
                QString output;
                if (m_keyexp != 0)
                {
                    output = QString::number(m_keyexpnumber);
                    if (m_keyexp == 1)
                        output.append("d");
                    if (m_keyexp == 2)
                        output.append("w");
                    if (m_keyexp == 3)
                        output.append("m");
                    if (m_keyexp == 4)
                        output.append("y");
                }
                else
                  output = QString("0");

                p->write(output.toAscii() + '\n');
            }
            else
            if (line.contains("keygen.name")) {
		if (m_success == 10) {
			p->kill();
		} else {
			m_success = 10;
			p->write(m_keyname.toAscii() + '\n');
		}
            } else
            if (line.contains("keygen.email"))
                p->write(m_keyemail.toAscii() + '\n');
            else
            if (line.contains("keygen.comment"))
                p->write(m_keycomment.toAscii() + '\n');
            else
            if (line.contains("passphrase.enter"))
            {
                QString keyid;
                if (!m_keyemail.isEmpty())
                    keyid = m_keyname + " <" + m_keyemail + '>';
                else
                    keyid = m_keyname;
                QString passdlgmessage = i18n("<p><b>Enter passphrase for %1</b>:<br />Passphrase should include non alphanumeric characters and random sequences</p>", keyid);
                if (sendPassphrase(passdlgmessage, p, true))
                {
                    p->deleteLater();
                    emit generateKeyFinished(3, this, m_keyname, m_keyemail, QString());
                    return;
                }
                step--;
                m_success = 5;
            }
            else
            if (line.contains("GOOD_PASSPHRASE"))
		m_success = 5;
            else
            if (line.contains("KEY_CREATED"))
            {
                m_newfingerprint = line.right(40);
                m_success = 2;
            }
            else
            if (line.contains("GET_"))
            {
                p->write("quit\n");
            } else if (line.contains("NEED_PASSPHRASE")) {
		m_success = 3;
            }
    }
}

void KgpgInterface::generateKeyFin(GPGProc *p)
{
    p->deleteLater();
    emit generateKeyFinished(m_success, this, m_keyname, m_keyemail, m_newfingerprint);
}

void KgpgInterface::downloadKeys(const QStringList &keys, const QString &keyserver, const bool &refresh, const QString &proxy)
{
    m_downloadkeys.clear();
    m_downloadkeys_log.clear();

    m_downloadprocess = new GPGProc(this);
    *m_downloadprocess << "--command-fd=0" << "--status-fd=1";

    m_downloadprocess->setOutputChannelMode(KProcess::MergedChannels);

    if (proxy.isEmpty())
        *m_downloadprocess << "--keyserver-options" << "no-honor-http-proxy";
    else
    {
        *m_downloadprocess << "--keyserver-options" << "honor-http-proxy";
        m_downloadprocess->setEnvironment(QStringList("http_proxy=" + proxy));
    }

    *m_downloadprocess << "--keyserver" << keyserver;
    if (refresh)
        *m_downloadprocess << "--refresh-keys";
    else
        *m_downloadprocess << "--recv-keys";
    *m_downloadprocess << keys;

    connect(m_downloadprocess, SIGNAL(processExited(GPGProc *)), this, SLOT(downloadKeysFin(GPGProc *)));
    connect(m_downloadprocess, SIGNAL(readReady(GPGProc *)), this, SLOT(downloadKeysProcess(GPGProc *)));
    m_downloadprocess->start();
}

void KgpgInterface::downloadKeysAbort()
{
    if (m_downloadprocess && (m_downloadprocess->state() == QProcess::Running))
    {
        disconnect(m_downloadprocess, 0, 0, 0);
        m_downloadprocess->kill();

        delete m_downloadprocess;
        m_downloadprocess = 0;

        emit downloadKeysAborted(this);
    }
}

void KgpgInterface::downloadKeysProcess(GPGProc *p)
{
    QString line;

    while (p->readln(line, true) >= 0) {
            if (line.startsWith("[GNUPG:]"))
                m_downloadkeys += line + '\n';
            else
            if  (line.startsWith("gpgkeys: "))
                m_downloadkeys_log += line.mid(9) + '\n';
            else
                m_downloadkeys_log += line + '\n';
    }
}

void KgpgInterface::downloadKeysFin(GPGProc *p)
{
    p->deleteLater();
    m_downloadprocess = 0;

    QStringList importedkeys;
    QString parsedoutput = m_downloadkeys;
    while (parsedoutput.contains("IMPORTED"))
    {
        parsedoutput.remove(0, parsedoutput.indexOf("IMPORTED") + 8);
        importedkeys += parsedoutput.section("\n", 0, 0).simplified();
    }

    while (parsedoutput.contains("IMPORT_OK"))
    {
        parsedoutput.remove(0, parsedoutput.indexOf("IMPORT_OK") + 9);
        importedkeys += parsedoutput.section("\n", 0, 0).simplified().right(16);
    }

    QStringList nbs;
    if (m_downloadkeys.contains("IMPORT_RES"))
        nbs = m_downloadkeys.section("IMPORT_RES", -1, -1).simplified().split(' ');
    else
        nbs = QString("0 0 0 0 0 0 0 0 0 0 0 0 0 0").split(' ');

    QList<int> result;
    bool key = false;
    for(int i = 0; i < nbs.count(); ++i)
    {
        result.append(nbs[i].toInt());
        if (!key && result[i] != 0)
            key = true;
    }

    emit downloadKeysFinished(result, importedkeys, key, m_downloadkeys_log, this);
}

void KgpgInterface::uploadKeys(const QStringList &keys, const QString &keyserver, const QString &attributes, const QString &proxy)
{
    m_uploadkeys_log.clear();

    m_uploadprocess = new GPGProc(this);
    *m_uploadprocess << "--status-fd=1";

    if (proxy.isEmpty())
        *m_uploadprocess << "--keyserver-options" << "no-honor-http-proxy";
    else
    {
        *m_uploadprocess << "--keyserver-options" << "honor-http-proxy";
        m_uploadprocess->setEnvironment(QStringList("http_proxy=" + proxy));
    }

    *m_uploadprocess << "--keyserver" << keyserver;
    *m_uploadprocess << "--export-options";
    if (attributes.isEmpty())
        *m_uploadprocess << "no-export-attributes";
    else
        *m_uploadprocess << attributes;
    *m_uploadprocess << "--send-keys";
    *m_uploadprocess << keys;

    connect(m_uploadprocess, SIGNAL(processExited(GPGProc *)), this, SLOT(uploadKeysFin(GPGProc *)));
    connect(m_uploadprocess, SIGNAL(readReady(GPGProc *)), this, SLOT(uploadKeysProcess(GPGProc *)));
    m_uploadprocess->start();
}

void KgpgInterface::uploadKeysAbort()
{
    if (m_uploadprocess && (m_uploadprocess->state() == QProcess::Running))
    {
        disconnect(m_uploadprocess, 0, 0, 0);
        m_uploadprocess->kill();

        delete m_uploadprocess;
        m_uploadprocess = 0;

        emit uploadKeysAborted(this);
    }
}

void KgpgInterface::uploadKeysProcess(GPGProc *p)
{
	QString line;

	while (p->readln(line, true) >= 0) {
		if (line.startsWith("gpg: "))
			m_uploadkeys_log += line.mid(5) + '\n';
	}
}

void KgpgInterface::uploadKeysFin(GPGProc *p)
{
    p->deleteLater();
    m_uploadprocess = 0;

    emit uploadKeysFinished(m_uploadkeys_log, this);
}

// delete signature
void KgpgInterface::KgpgDelSignature(const QString &keyID, const QString &uid, QString signKeyID)
{
    deleteSuccess = false;
    step = 0;

    QString encResult;
    signb = 0;
    sigsearch = 0;
    QList<int> signoff;

    findSigns(keyID, QStringList(signKeyID), uid, &signoff);

    if (signoff.count() == 0)
        return;

    signb = signoff.at(0);

	GPGProc *conprocess = new GPGProc(this);
	*conprocess << "--command-fd=0" << "--status-fd=1" << "--edit-key" << keyID << "uid" << uid << "delsig";
	connect(conprocess,SIGNAL(readReady(GPGProc *)),this,SLOT(delsigprocess(GPGProc *)));
	connect(conprocess, SIGNAL(processExited(GPGProc *)),this, SLOT(delsignover(GPGProc *)));
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

void KgpgInterface::KgpgRevokeKey(const QString &keyID, const QString &revokeUrl, const int reason, const QString &description)
{
	revokeReason=reason;
	revokeSuccess=false;
	revokeDescription=description;
	certificateUrl=revokeUrl;
	output.clear();

	GPGProc *process = new GPGProc(this);
	*process << "--status-fd=1" << "--command-fd=0";

	if (!revokeUrl.isEmpty())
		*process << "-o" << revokeUrl;
	*process << "--gen-revoke" << keyID;
	QObject::connect(process, SIGNAL(processExited(GPGProc *)),this, SLOT(revokeover(GPGProc *)));
	QObject::connect(process, SIGNAL(readReady(GPGProc *)),this, SLOT(revokeprocess(GPGProc *)));
	process->start();
}

void KgpgInterface::revokeover(GPGProc *)
{
        if (!revokeSuccess)
                KMessageBox::detailedSorry(0,i18n("Creation of the revocation certificate failed..."),output);
        else {
                output=output.section("-----BEGIN",1);
                output.prepend("-----BEGIN");
                output=output.section("BLOCK-----",0);
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
	*listproc << "--with-colons" << "--list-sigs" << keyID;
	listproc->start();
	listproc->waitForFinished(-1);

	QString line;
	int curuid = 1;
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
