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

#include <QApplication>
#include <QTextStream>
#include <QTextCodec>
#include <QClipboard>
#include <QFile>

#include <kio/netaccess.h>
#include <KMessageBox>
#include <KTemporaryFile>
#include <KPasswordDialog>
#include <knewpassworddialog.h>
#include <KLocale>
#include <KCodecs>
#include <K3ProcIO>
#include <KProcess>
#include <KConfig>
#include <KDebug>
#include <KGlobal>
#include <KUrl>

#include "detailedconsole.h"
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

    QString line;
    if (process.readln(line) != -1)
        line = line.simplified().section(' ', -1);

    QStringList values = line.split('.');
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
                    return result.split(" ");
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

int KgpgInterface::sendPassphrase(const QString &text, K3ProcIO *process, const bool isnew)
{
    QByteArray passphrase;
    int code;
    if (isnew)
    {
        KNewPasswordDialog dlg;
        dlg.setPrompt(text);
        code = dlg.exec();
        passphrase = dlg.password().toLocal8Bit();
    }
    else
    {
        KPasswordDialog dlg;
        dlg.setPrompt(text);
        code = dlg.exec();
        passphrase = dlg.password().toLocal8Bit();
    }

    if (code != KPasswordDialog::Accepted)
        return 1;

    process->writeStdin(passphrase, true);

    return 0;
}

void KgpgInterface::updateIDs(QString txt)
{
    int cut = txt.indexOf(' ', 22, Qt::CaseInsensitive);
    txt.remove(0, cut);

    if (txt.contains('(', Qt::CaseInsensitive))
        txt = txt.section('(', 0, 0) + txt.section(')', -1);

    txt.replace(QRegExp("<"), "&lt;");

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
    *process << "--with-fingerprint";
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
    unsigned int uidnum = 0;

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
                m_publickey.setUnlimited(true);
                m_publickey.setExpiration(QDate());
            }
            else
            {
                m_publickey.setUnlimited(false);
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
                    kmail = kmail.replace(">", ";");
                    kmail.remove("<");
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
            uidnum = 1;
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
                sub.setUnlimited(true);
                sub.setExpiration(QDate());
            }
            else
            {
                sub.setUnlimited(false);
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

            uid.setIndex(++uidnum);
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
                    kmail = kmail.replace(">", ";");
                    kmail.remove("<");
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

            m_numberid++;
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
                signature.setUnlimited(true);
                signature.setExpiration(QDate());
            }
            else
            {
                signature.setUnlimited(false);
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
                    kmail = kmail.replace(">", ";");
                    kmail.remove("<");
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

    delete p;
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
        *process << "--list-secret-keys" << "--with-fingerprint";

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
                    m_secretkey.setUnlimited(true);
                    m_secretkey.setExpiration(QDate());
                }
                else
                {
                    m_secretkey.setUnlimited(false);
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
                        kmail = kmail.replace(">", ";");
                        kmail.remove("<");
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

KgpgKeyList KgpgInterface::readJoinedKeys(const KgpgKeyTrust &trust, const QStringList &ids)
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

QString KgpgInterface::getKeys(const bool &block, const QString *attributes, const QStringList &ids)
{
    m_partialline.clear();
    m_ispartial = false;
    m_keystring.clear();

    K3ProcIO *process = gpgProc(2, 0);
    process->setParent(this);
    *process << "--export" << "--armor";

    if (attributes)
	*process << "--export-options" << *attributes;

    *process << ids;

    if (!block)
    {
        kDebug(2100) << "Get a key with K3Process::NotifyOnExit";
        connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(getKeysProcess(K3ProcIO *)));
        connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(getKeysFin(K3Process *)));
        process->start(K3Process::NotifyOnExit, false);
        emit getKeysStarted(this);
        return QString();
    }
    else
    {
        kDebug(2100) << "Get a key with K3Process::Block";
        process->start(K3Process::Block, false);
        getKeysProcess(process);
        delete process;
        return m_keystring;
    }
}

void KgpgInterface::getKeysProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline.clear();
                m_ispartial = false;
            }

            if (!line.startsWith("gpg:"))
                m_keystring += line + '\n';
        }
    }
    p->ackRead();
}

void KgpgInterface::getKeysFin(K3Process *p)
{
    delete p;
    emit getKeysFinished(m_keystring, this);
}

void KgpgInterface::encryptText(const QString &text, const QStringList &userids, const QStringList &options)
{
    m_partialline.clear();
    m_ispartial = false;
    message.clear();

    QTextCodec *codec = QTextCodec::codecForLocale();
    if (codec->canEncode(text))
        message = text;
    else
        message = text.toUtf8();

    K3ProcIO *process = gpgProc(1, 0);
    process->setParent(this);

    for (QStringList::ConstIterator it = options.begin(); it != options.end(); ++it)
        *process << QFile::encodeName(*it);

    if (!userids.isEmpty())
    {
        *process << "-e";
        for (QStringList::ConstIterator it = userids.begin(); it != userids.end(); ++it)
            *process << "--recipient" << *it;
    }
    else
        *process << "-c";

    kDebug(2100) << "Encrypt a text";
    connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(encryptTextProcess(K3ProcIO *)));
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(encryptTextFin(K3Process *)));
    process->start(K3Process::NotifyOnExit, false);
}

void KgpgInterface::encryptTextProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline.clear();
                m_ispartial = false;
            }

            if (line.contains("BEGIN_ENCRYPTION"))
            {
                emit txtEncryptionStarted();
                p->writeStdin(message, false);
                p->closeWhenDone();
                message.clear();
            }
            else
            if (line.contains("passphrase.enter"))
            {
                if (sendPassphrase(i18n("Enter passphrase (symmetrical encryption)"), p))
                {
                    delete p;
                    emit processaborted(true);
                    return;
                }
            }
            else
            if (!line.startsWith("[GNUPG:]"))
                message += line + '\n';
        }
    }

    p->ackRead();
}

void KgpgInterface::encryptTextFin(K3Process *p)
{
    delete p;
    if (!message.isEmpty())
        emit txtEncryptionFinished(QString::fromUtf8(message.toAscii()).trimmed(), this);
    else
        emit txtEncryptionFinished(QString(), this);
}

// FIXME Check if it works well
void KgpgInterface::decryptText(const QString &text, const QStringList &options)
{
    m_partialline.clear();
    message.clear();
    tmp_message.clear();
    userIDs.clear();
    log.clear();
    m_ispartial = false;
    anonymous = false;
    badmdc = false;
    decok = false;
    m_textlength = 0;
    step = 3;

    K3ProcIO *process = gpgProc(1, 0);
    process->setParent(this);
    //*process << "--no-batch";

    for (QStringList::ConstIterator it = options.begin(); it != options.end(); ++it)
        *process << QFile::encodeName(*it);

    *process << "-d";

    kDebug(2100) << "Decrypt a text";
    connect(process, SIGNAL(receivedStdout(K3Process *, char *, int)), this, SLOT(decryptTextStdOut(K3Process *, char *, int)));
    connect(process, SIGNAL(receivedStderr(K3Process *, char *, int)), this, SLOT(decryptTextStdErr(K3Process *, char *, int)));
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(decryptTextFin(K3Process *)));
    process->start(K3Process::NotifyOnExit, K3Process::All);

    // send the encrypted text to gpg
    process->writeStdin(text);
}

void KgpgInterface::decryptTextStdErr(K3Process *, char *data, int)
{
    log.append(data);
}

// try to change K3Process by K3ProcIO
void KgpgInterface::decryptTextStdOut(K3Process *p, char *data, int)
{
    QTextCodec *codec = QTextCodec::codecForName("utf8");
    m_readin.append(data);

    int pos;
    while ((pos = m_readin.indexOf("\n")) != -1)
    {
        if (m_textlength == 0)
        {
            QString line = codec->toUnicode(m_readin.left(pos));
            m_readin.remove(0, pos + 1);

            if (line.contains("USERID_HINT"))
                updateIDs(line);
            else
            if (line.contains("ENC_TO") && line.contains("0000000000000000"))
                anonymous = true;
            else
            if (line.contains("GET_"))
            {
                if ((line.contains("passphrase.enter")))
                {
                    if (userIDs.isEmpty())
                        userIDs = i18n("[No user id found]");

                    QString passdlgmessage;
                    if (anonymous)
                        passdlgmessage = i18n("<p><b>No user id found</b>. Trying all secret keys.</p>");
                    if ((step < 3) && (!anonymous))
                        passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
                    passdlgmessage += i18n("Enter passphrase for <b>%1</b>", checkForUtf8bis(userIDs));

                    if (sendPassphrase(passdlgmessage, static_cast<K3ProcIO*>(p), false))
                    {
                        delete p;
                        emit processaborted(true);
                        return;
                    }

                    userIDs.clear();

                    if (step > 1)
                        step--;
                    else
                        step = 3;
                }
                else
                {
                    p->writeStdin("quit", 5);
                    p->closeStdin();
                }
            }
            else
            if (line.contains("BEGIN_DECRYPTION"))
            {
                emit txtDecryptionStarted();
                p->closeStdin();
            }
            else
            if (line.contains("PLAINTEXT_LENGTH"))
                m_textlength = line.mid(line.indexOf("[GNUPG:] PLAINTEXT_LENGTH") + 25).toInt();
            else
            if (line.contains("DECRYPTION_OKAY"))
                decok = true;
            else
            if (line.contains("BADMDC"))
                badmdc = true;
            else
            if (!line.contains("[GNUPG:]"))
                tmp_message.append(line + '\n');
        }
        else
        {
            if (m_readin.length() <= m_textlength)
            {
                message.append(codec->toUnicode(m_readin));
                m_textlength -= m_readin.length();
                m_readin.clear();
            }
            else
            {
                message.append(codec->toUnicode(m_readin.left(m_textlength)));
                m_readin.remove(0, m_textlength);
                m_textlength = 0;
            }
        }
    }
}

void KgpgInterface::decryptTextFin(K3Process *p)
{
    delete p;
    if ((decok) && (!badmdc)) {
        if (message.length() == 0)
            emit txtDecryptionFinished(tmp_message, this);
        else
            emit txtDecryptionFinished(message, this);
    }
    else
    if (badmdc)
    {
        KMessageBox::sorry(0, i18n("Bad MDC detected. The encrypted text has been manipulated."));
        emit txtDecryptionFailed(log, this);
    }
    else
        emit txtDecryptionFailed(log, this);
}

void KgpgInterface::signText(const QString &text, const QString &userid, const QStringList &options)
{
    m_partialline.clear();
    m_ispartial = false;
    message.clear();
    badpassword = false;
    step = 3;

    QTextCodec *codec = QTextCodec::codecForLocale();
    if (codec->canEncode(text))
        message = text;
    else
        message = text.toUtf8();

    K3ProcIO *process = gpgProc(1, 0);
    process->setParent(this);

    for (QStringList::ConstIterator it = options.begin(); it != options.end(); ++it)
        *process << QFile::encodeName(*it);

    *process << "--clearsign" << "-u" << userid;

    kDebug(2100) << "Sign a text";
    connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(signTextProcess(K3ProcIO *)));
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(signTextFin(K3Process *)));
    process->start(K3Process::NotifyOnExit, true);
}

void KgpgInterface::signTextProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline = "";
                m_ispartial = false;
            }

            if (line.contains("USERID_HINT"))
                updateIDs(line);
            else
            if (line.contains("BAD_PASSPHRASE"))
            {
                message.fill('x');
                message.clear();
                badpassword = true;
            }
            else
            if (line.contains("GOOD_PASSPHRASE"))
            {
                emit txtSigningStarted();
                p->writeStdin(message, true);
                p->closeWhenDone();
                message.clear();
            }
            else
            if (line.contains("passphrase.enter"))
            {
                if (userIDs.isEmpty())
                    userIDs = i18n("[No user id found]");

                QString passdlgmessage;
                if (step < 3)
                    passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
                passdlgmessage += i18n("Enter passphrase for <b>%1</b>", checkForUtf8bis(userIDs));

                if (sendPassphrase(passdlgmessage, p, false))
                {
                    delete p;
                    emit processaborted(true);
                    return;
                }
                step--;
            }
            else
            if (!line.startsWith("[GNUPG:]"))
                message += line + '\n';
        }
    }

    p->ackRead();
}

void KgpgInterface::signTextFin(K3Process *p)
{
    delete p;
    if (badpassword)
    {
        emit txtSigningFailed(message, this);
        message.clear();
    }
    else
    if (!message.isEmpty())
        emit txtSigningFinished(message.trimmed(), this);
    else
        emit txtSigningFinished(QString(), this);
}

void KgpgInterface::verifyText(const QString &text)
{
    m_partialline.clear();
    m_ispartial = false;
    QString temp;
    QTextCodec *codec = QTextCodec::codecForLocale();
    if (codec->canEncode(text))
        temp = text;
    else
        temp = text.toUtf8();

    signmiss = false;
    signID.clear();
    message.clear();

    K3ProcIO *process = new K3ProcIO();
    process->setParent(this);
    *process << KGpgSettings::gpgBinaryPath() << "--no-secmem-warning" << "--status-fd=2" << "--command-fd=0" << "--verify";

    kDebug(2100) << "Verify a text";
    connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(verifyTextProcess(K3ProcIO *)));
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(verifyTextFin(K3Process *)));
    process->start(K3Process::NotifyOnExit, true);
    emit txtVerifyStarted();

    process->writeStdin(temp);
    process->closeWhenDone();
}

void KgpgInterface::verifyTextProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline = "";
                m_ispartial = false;
            }

            if (!line.startsWith("[GNUPG:]"))
                message += line + '\n';
            else
            {
                line = line.section("]", 1, -1).simplified();

                if (line.startsWith("GOODSIG"))
                {
                    QString userName = line.section(" ", 2, -1).replace(QRegExp("<"), "&lt;");
                    userName = checkForUtf8(userName);
                    signID = i18n("<qt>Good signature from:<br /><b>%1</b><br />Key ID: %2</qt>", userName, line.section(" ", 1, 1).right(8));
                }
                else
                if (line.startsWith("BADSIG"))
                {
                    signID = i18n("<qt><b>Bad signature</b> from:<br />%1<br />Key ID: %2<br /><br /><b>Text is corrupted.</b></qt>", line.section(" ", 2, -1).replace(QRegExp("<"), "&lt;"), line.section(" ", 1, 1).right(8));
                }
                else
                if (line.startsWith("NO_PUBKEY"))
                {
                    signID = line.section(" ", 1, 1).right(8);
                    signmiss = true;
                }
                else
                if (line.startsWith("UNEXPECTED") || line.startsWith("NODATA"))
                    signID = i18n("No signature found.");
                else
                if (line.startsWith("TRUST_UNDEFINED"))
                    signID += i18n("The signature is valid, but the key is untrusted");
                else
                if (line.startsWith("TRUST_ULTIMATE"))
                    signID += i18n("The signature is valid, and the key is ultimately trusted");
            }
        }
    }

    p->ackRead();
}

void KgpgInterface::verifyTextFin(K3Process *p)
{
    delete p;
    if (signmiss)
        emit txtVerifyMissingSignature(signID, this);
    else
    {
        if (signID.isEmpty())
            signID = i18n("No signature found.");

        emit txtVerifyFinished(signID, message, this);
    }
}

void KgpgInterface::encryptFile(const QStringList &encryptkeys, const KUrl &srcurl, const KUrl &desturl, const QStringList &options, const bool &symetrical)
{
    m_partialline.clear();
    m_ispartial = false;
    encok = false;
    sourceFile = srcurl;
    message.clear();

    K3ProcIO *process = gpgProc(2, 0);
    process->setParent(this);

    for (QStringList::ConstIterator it = options.begin(); it != options.end(); ++it)
        *process << QFile::encodeName(*it);

    *process << "--output" << QFile::encodeName(desturl.path());

    if (!symetrical)
    {
        *process << "-e";
        for (QStringList::ConstIterator it = encryptkeys.begin(); it != encryptkeys.end(); ++it)
            *process << "--recipient" << *it;
    }
    else  // symetrical encryption, prompt for password
        *process << "-c";

    *process << QFile::encodeName(srcurl.path());

    kDebug(2100) << "Encrypt a file";
    connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(fileReadEncProcess(K3ProcIO *)));
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(fileEncryptFin(K3Process *)));
    process->start(K3Process::NotifyOnExit, true);
}

// TODO : there is a bug when we want to encrypt a file
void KgpgInterface::fileReadEncProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline = "";
                m_ispartial = false;
            }

            kDebug(2100) << line ;
            if (line.contains("BEGIN_ENCRYPTION"))
                emit processstarted(sourceFile.path());
            else
            if (line.contains("GET_" ))
            {
                if (line.contains("openfile.overwrite.okay"))
                    p->writeStdin(QByteArray("Yes"));
                else
                if (line.contains("passphrase.enter"))
                {
                    if (sendPassphrase(i18n("Enter passphrase for your file (symmetrical encryption):"), p))
                    {
                        delete p;
                        emit processaborted(true);
                        return;
                    }
                }
                else
                {
                    p->writeStdin(QByteArray("quit"));
                    p->closeWhenDone();
                }
            }
            else
            if (line.contains("END_ENCRYPTION"))
                encok = true;
            else
            if (!line.startsWith("[GNUPG:]"))
                message += line + '\n';
        }
    }

    p->ackRead();
}

void KgpgInterface::fileEncryptFin(K3Process *p)
{
    delete p;
    if (encok)
        emit fileEncryptionFinished(sourceFile, this);
    else
        emit errorMessage(message, this);
}

void KgpgInterface::signKey(const QString &keyid, const QString &signkeyid, const bool &local, const int &checking, const bool &terminal)
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

    K3ProcIO *process = gpgProc(2, 0);
    process->setParent(this);
    *process << "-u" << signkeyid;
    *process << "--edit-key" << keyid;

    if (local)
        *process << "lsign";
    else
        *process << "sign";

    kDebug(2100) << "Sign a key";
    connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(signKeyProcess(K3ProcIO *)));
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(signKeyFin(K3Process *)));
    process->start(K3Process::NotifyOnExit, true);
}

void KgpgInterface::signKeyProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline = "";
                m_ispartial = false;
            }

            if (line.startsWith("[GNUPG:]"))
            {
                if (line.contains("USERID_HINT"))
                    updateIDs(line);
                else
                if (m_success == 3)
                {
                    // user has aborted the process and don't want to sign the key
                    if (line.contains("GET_"))
                        p->writeStdin(QByteArray("quit"), true);
                    p->closeWhenDone();
                    return;
                }
                else
                if (line.contains("ALREADY_SIGNED"))
                    m_success = 4;
                if (line.contains("GOOD_PASSPHRASE"))
                {
                    emit signKeyStarted();
                    m_success = 2;
                }
                else
                if (line.contains("sign_uid.expire"))
                    p->writeStdin(QByteArray("Never"), true);
                else
                if (line.contains("sign_uid.class"))
                    p->writeStdin(QString::number(m_checking), true);
                else
                if (line.contains("sign_uid.okay"))
                    p->writeStdin(QByteArray("Y"), true);
                else
                if (line.contains("sign_all.okay"))
                    p->writeStdin(QByteArray("Y"), true);
                else
                if (line.contains("passphrase.enter"))
                {
                    QString passdlgmessage;
                    if (step < 3)
                        passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
                    passdlgmessage += i18n("Enter passphrase for <b>%1</b>", checkForUtf8bis(userIDs));

                    if (sendPassphrase(passdlgmessage, p, false))
                    {
                        m_success = 3;
                        p->writeStdin(QByteArray("quit"), true);
                        return;
                    }

                    if (step > 1)
                        step--;
                    else
                        step = 3;
                }
                else
                if ((m_success != 1) && line.contains("keyedit.prompt"))
                    p->writeStdin(QByteArray("save"), true);
                else
                if (line.contains("BAD_PASSPHRASE"))
                    m_success = 1;
                else
                if (line.contains("GET_")) // gpg asks for something unusal, turn to konsole mode
                {
                    if (m_success != 1)
                        m_success = 5; // switching to console mode
                    p->writeStdin(QByteArray("quit"), true);
                }
            }
            else
                log += line + '\n';
        }
    }

    p->ackRead();
}

void KgpgInterface::signKeyFin(K3Process *p)
{
    delete p;
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

void KgpgInterface::keyExpire(const QString &keyid, const QDate &date, const bool &unlimited)
{
    m_partialline.clear();
    m_ispartial = false;
    log.clear();
    m_success = 0;
    step = 3;

    if (unlimited)
        expirationDelay = 0;
    else
        expirationDelay = QDate::currentDate().daysTo(date);

    K3ProcIO *process = gpgProc(2, 0);
    process->setParent(this);
    *process << "--edit-key" << keyid << "expire";

    kDebug(2100) << "Change expiration of the key" << keyid;
    connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(keyExpireProcess(K3ProcIO *)));
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(keyExpireFin(K3Process *)));
    process->start(K3Process::NotifyOnExit, true);
}

void KgpgInterface::keyExpireProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline = "";
                m_ispartial = false;
            }

            if (!line.startsWith("[GNUPG:]"))
                log += line + '\n';
            else
            if (m_success == 3)
            {
                if (line.contains("GET_" ))
                    p->writeStdin(QByteArray("quit"), true);
                p->closeWhenDone();
                return;
            }
            else
            if (line.contains("USERID_HINT"))
                updateIDs(line);
            else
            if (line.contains("GOOD_PASSPHRASE"))
            {
                m_success = 2;
                emit keyExpireStarted();
            }
            else
            if (line.contains("keygen.valid"))
                p->writeStdin(QString::number(expirationDelay), true);
            else
            if (line.contains("passphrase.enter"))
            {
                QString passdlgmessage;
                if (step < 3)
                    passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
                passdlgmessage += i18n("Enter passphrase for <b>%1</b>", checkForUtf8bis(userIDs));

                if (sendPassphrase(passdlgmessage, p, false))
                {
                    m_success = 3;  // aborted by user mode
                    p->writeStdin(QByteArray("quit"), true);
                    p->closeWhenDone();
                    return;
                }
                --step;
            }
            else
            if ((m_success == 2) && line.contains("keyedit.prompt"))
                p->writeStdin(QByteArray("save"), true);
            else
            if ((m_success == 2) && line.contains("keyedit.save.okay"))
                p->writeStdin(QByteArray("YES"), true);
            else
            if (line.contains("BAD_PASSPHRASE"))
            {
                m_success = 1; // bad passphrase
                p->writeStdin(QByteArray("quit"), true);
            }
            else
            if (line.contains("GET_")) // gpg asks for something unusal, turn to konsole mode
            {
                if (m_success != 1)
                    m_success = 4; // switching to console mode
                p->writeStdin(QByteArray("quit"), true);
            }
        }
    }

    p->ackRead();
}

void KgpgInterface::keyExpireFin(K3Process *p)
{
    delete p;
    if (m_success < 4)
        emit keyExpireFinished(m_success, this); // signature successful or bad passphrase
    else
    {
        KgpgDetailedConsole *q = new KgpgDetailedConsole(0, i18n("<qt><b>Changing expiration failed.</b><br />"
                                    "Do you want to try changing the key expiration in console mode?</qt>"),output);
        if (q->exec() == QDialog::Accepted)
            KMessageBox::sorry(0, "work in progress...");
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

    K3ProcIO *process = gpgProc(2, 0);
    process->setParent(this);
    *process << "--no-use-agent";
    *process << "--edit-key" << keyid << "passwd";

    kDebug(2100) << "Cange passphrase of the key" << keyid;
    connect(process,SIGNAL(readReady(K3ProcIO *)), this, SLOT(changePassProcess(K3ProcIO *)));
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(changePassFin(K3Process *)));
    process->start(K3Process::NotifyOnExit, true);
}

void KgpgInterface::changePassProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline = "";
                m_ispartial = false;
            }

            if (line.contains("USERID_HINT"))
                updateIDs(line);
            else
            if ((m_success == 4) && line.contains("keyedit.prompt"))
            {
                m_success = 2;
                p->writeStdin(QByteArray("save"), true);
            }
            else
            if (line.contains("GOOD_PASSPHRASE"))
                m_success = 4;
            else
            if (line.contains("passphrase.enter"))
            {
                if (userIDs.isEmpty())
                    userIDs = i18n("[No user id found]");
                userIDs.replace(QRegExp("<"), "&lt;");

                if (m_success == 1)
                {
                    QString passdlgmessage;
                    if (step < 3)
                        passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
                    passdlgmessage += i18n("Enter old passphrase for <b>%1</b>", checkForUtf8bis(userIDs));

                    if (sendPassphrase(passdlgmessage, p, false))
                    {
                        delete p;
                        emit changePassFinished(3, this);
                        return;
                    }
                    --step;
                }
                else
                if (m_success == 4)
                {
                    if (sendPassphrase(i18n("<qt>Enter new passphrase for <b>%1</b><br />If you forget this passphrase all your encrypted files and messages will be inaccessible<br /></qt>", userIDs), p))
                    {
                        delete p;
                        emit changePassFinished(3, this);
                        return;
                    }
                }
            }
            else
            if (line.contains("GET_")) // gpg asks for something unusal, turn to konsole mode
            {
                p->writeStdin(QByteArray("quit"), true);
                p->closeWhenDone();
            }
        }
    }

    p->ackRead();
}

void KgpgInterface::changePassFin(K3Process *p)
{
    delete p;
    emit changePassFinished(m_success, this);
}

void KgpgInterface::changeTrust(const QString &keyid, const KgpgKeyOwnerTrust &keytrust)
{
    m_partialline.clear();
    m_ispartial = false;
    m_trustvalue = keytrust;

    K3ProcIO *process = gpgProc(2, 0);
    process->setParent(this);
    *process << "--edit-key" << keyid << "trust";

    kDebug(2100) << "Change trust of the key" << keyid << "to" << keytrust;
    connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(changeTrustProcess(K3ProcIO *)));
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(changeTrustFin(K3Process *)));
    process->start(K3Process::NotifyOnExit, true);
}

void KgpgInterface::changeTrustProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline = "";
                m_ispartial = false;
            }

            if (line.contains("edit_ownertrust.set_ultimate.okay"))
                p->writeStdin(QByteArray("YES"), true);
            else
            if (line.contains("edit_ownertrust.value"))
                p->writeStdin(QString::number(m_trustvalue), true);
            else
            if (line.contains("keyedit.prompt"))
            {
                p->writeStdin(QByteArray("save"), true);
                p->closeWhenDone();
            }
            else
            if (line.contains("GET_")) // gpg asks for something unusal
            {
                p->writeStdin(QByteArray("quit"), true);
                p->closeWhenDone();
            }
        }
    }

    p->ackRead();
}

void KgpgInterface::changeTrustFin(K3Process *p)
{
    delete p;
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
    delete editprocess;
    editprocess = 0;
    changeDisableFinished(this, res);
}

QPixmap KgpgInterface::loadPhoto(const QString &keyid, const QString &uid, const bool &block)
{
    m_partialline.clear();
    m_ispartial = false;
    m_pixmap = QPixmap();

    m_kgpginfotmp = new KTemporaryFile();
    m_kgpginfotmp->open();

    QString pgpgoutput = "cp %i " + m_kgpginfotmp->fileName();

    K3ProcIO *process = gpgProc(2, 0);
    process->setParent(this);
    *process << "--photo-viewer" << pgpgoutput << "--edit-key" << keyid << "uid" << uid << "showphoto" << "quit";

    if (!block)
    {
        kDebug(2100) << "Load a photo for the key" << keyid << "uid" << uid;
        connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(loadPhotoFin(K3Process *)));
        process->start(K3Process::NotifyOnExit, true);
        return QPixmap();
    }
    else
    {
        kDebug(2100) << "Load a photo for the key" << keyid << "uid" << uid;
        process->start(K3Process::Block, true);
        loadPhotoFin(process, true);
        return m_pixmap;
    }
}

void KgpgInterface::loadPhotoFin(K3Process *p, const bool &block)
{
    delete p;

    m_pixmap.load(m_kgpginfotmp->fileName());
    delete m_kgpginfotmp;
    m_kgpginfotmp = 0;

    if (!block)
        emit loadPhotoFinished(m_pixmap, this);
}

void KgpgInterface::addPhoto(const QString &keyid, const QString &imagepath)
{
    m_partialline.clear();
    m_ispartial = false;

    photoUrl = imagepath;
    m_success = 0;
    step = 3;

    K3ProcIO *process = gpgProc(2, 0);
    process->setParent(this);
    *process << "--edit-key" << keyid << "addphoto";

    kDebug(2100) << "Add the photo" << imagepath << "to the key" << keyid;
    connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(addPhotoProcess(K3ProcIO *)));
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(addPhotoFin(K3Process *)));
    process->start(K3Process::NotifyOnExit, true);
}

void KgpgInterface::addPhotoProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline = "";
                m_ispartial = false;
            }

            if (line.contains("USERID_HINT"))
                updateIDs(line);
            else
            if (line.contains("BAD_PASSPHRASE"))
                m_success = 1;
            else
            if (line.contains("GOOD_PASSPHRASE"))
                m_success = 2;
            if (line.contains("photoid.jpeg.add"))
                p->writeStdin(photoUrl, true);
            else
            if (line.contains("photoid.jpeg.size"))
            {
                if (KMessageBox::questionYesNo(0, i18n("This image is very large. Use it anyway?"), QString(), KGuiItem(i18n("Use Anyway")), KGuiItem(i18n("Do Not Use"))) == KMessageBox::Yes)
                    p->writeStdin(QByteArray("Yes"), true);
                else
                {
                    delete p;
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
                    delete p;
                    emit addPhotoFinished(3, this);
                    return;
                }

                step--;
            }
            else
            if ((m_success == 2) && (line.contains("keyedit.prompt")))
                p->writeStdin(QByteArray("save"), true);
            else
            if ((line.contains("GET_"))) // gpg asks for something unusal, turn to konsole mode
            {
                p->writeStdin(QByteArray("quit"), true);
                p->closeWhenDone();
            }
        }
    }

    p->ackRead();
}

void KgpgInterface::addPhotoFin(K3Process *p)
{
    delete p;
    emit addPhotoFinished(m_success, this);
}

void KgpgInterface::deletePhoto(const QString &keyid, const QString &uid)
{
    m_partialline.clear();
    m_ispartial = false;

    m_success = 0;
    step = 3;

    K3ProcIO *process = gpgProc(2, 0);
    process->setParent(this);
    *process << "--edit-key" << keyid << "uid" << uid << "deluid";

    kDebug(2100) << "Delete a photo from the key" << keyid;
    connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(deletePhotoProcess(K3ProcIO *)));
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(deletePhotoFin(K3Process *)));
    process->start(K3Process::NotifyOnExit, true);
}

void KgpgInterface::deletePhotoProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline = "";
                m_ispartial = false;
            }

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
                p->writeStdin(QByteArray("YES"), true);
            else
            if (line.contains("passphrase.enter"))
            {
                QString passdlgmessage;
                if (step < 3)
                    passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
                passdlgmessage += i18n("Enter passphrase for <b>%1</b>", checkForUtf8bis(userIDs));

                if (sendPassphrase(passdlgmessage, p, false))
                {
                    delete p;
                    emit deletePhotoFinished(3, this);
                    return;
                }
            }
            else
            if (line.contains("keyedit.prompt"))
                p->writeStdin(QByteArray("save"), true);
            else
            if (line.contains("GET_")) // gpg asks for something unusal, turn to konsole mode
            {
                p->writeStdin(QByteArray("quit"), true);
                p->closeWhenDone();
            }
        }
    }
    p->ackRead();
}

void KgpgInterface::deletePhotoFin(K3Process *p)
{
    delete p;
    emit deletePhotoFinished(m_success, this);
}

void KgpgInterface::importKey(const QString &keystr)
{
    m_partialline.clear();
    m_ispartial = false;
    message.clear();

    K3ProcIO *process = gpgProc(2);
    process->setParent(this);
    *process << "--import";
    *process << "--allow-secret-key-import";

    kDebug(2100) << "Import a key (text)";
    connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(importKeyProcess(K3ProcIO *)));
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(importKeyFinished(K3Process *)));
    process->start(K3Process::NotifyOnExit, true);

    process->writeStdin(keystr, true);
    process->closeWhenDone();
}

void KgpgInterface::importKey(const KUrl &url)
{
    m_partialline.clear();
    m_ispartial = false;
    message.clear();

    if(KIO::NetAccess::download(url, m_tempkeyfile, 0))
    {
        K3ProcIO *process = gpgProc(2);
        process->setParent(this);
        *process << "--import";
        *process << "--allow-secret-key-import";
        *process << m_tempkeyfile;

        kDebug(2100) << "Import a key (file)";
        connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(importKeyProcess(K3ProcIO *)));
        connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(importURLover(K3Process *)));
        process->start(K3Process::NotifyOnExit, true);
    }
}

void KgpgInterface::importURLover(K3Process *p)
{
    KIO::NetAccess::removeTempFile(m_tempkeyfile);
    importKeyFinished(p);
}

void KgpgInterface::importKeyProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline = "";
                m_ispartial = false;
            }

            if (line.contains("http-proxy"))
                message += line + '\n';
        }
    }
    p->ackRead();
}

void KgpgInterface::importKeyFinished(K3Process *p)
{
    delete p;

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
        QStringList messageList = parsedOutput.split(" ");

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
            importedKeysIds = QStringList("ALL");
        }
        if (messageList[9] != "0")
        {
            resultMessage += i18np("<qt>One secret key processed.</qt>", "<qt>%1 secret keys processed.</qt>", messageList[9].toULong());
            secretImport = true;
            if (importedKeysIds.isEmpty()) // orphaned secret key imported
               emit importKeyOrphaned();
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

    }
    else
        resultMessage = i18n("No key imported... \nCheck detailed log for more infos");

    emit importKeyFinished(importedKeysIds);

    // TODO : should be deleted. KgpgInterface should not show any dialog (but password).
    // When a message should be shown, it should be passed by parameter in a SIGNAL.
    (void) new KgpgDetailedInfo(0, resultMessage, message, importedKeys);
}

void KgpgInterface::addUid(const QString &keyid, const QString &name, const QString &email, const QString &comment)
{
    m_partialline.clear();
    m_ispartial = false;
    step = 3;
    m_success = 0;

    if ((!email.isEmpty()) && ((email.contains(" ")) || !email.contains('.') || !email.contains('@')))
    {
        emit addUidFinished(4, this);
        return;
    }

    uidName = name;
    uidComment = comment;
    uidEmail = email;

    K3ProcIO *process = gpgProc(2, 0);
    process->setParent(this);
    *process << "--edit-key" << keyid << "adduid";

    kDebug(2100) << "Add Uid" << name << "," << email << "," << comment << "to key" << keyid;
    connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(addUidProcess(K3ProcIO *)));
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(addUidFin(K3Process *)));
    process->start(K3Process::NotifyOnExit, true);
}

void KgpgInterface::addUidProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline = "";
                m_ispartial = false;
            }

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
                p->writeStdin(uidName, true);
            else
            if (line.contains("keygen.email"))
                p->writeStdin(uidEmail, true);
            else
            if (line.contains("keygen.comment"))
                p->writeStdin(uidComment, true);
            else
            if (line.contains("passphrase.enter"))
            {
                QString passdlgmessage;
                if (step < 3)
                    passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
                passdlgmessage += i18n("Enter passphrase for <b>%1</b>", checkForUtf8bis(userIDs));

                if (sendPassphrase(passdlgmessage, p, false))
                {
                    delete p;
                    emit addUidFinished(3, this);
                    return;
                }
                step--;
            }
            else
            if (line.contains("keyedit.prompt"))
                p->writeStdin(QByteArray("save"), true);
            else
            if (line.contains("GET_")) // gpg asks for something unusal, turn to konsole mode
            {
                p->writeStdin(QByteArray("quit"), true);
                p->closeWhenDone();
            }
        }
    }

    p->ackRead();
}

void KgpgInterface::addUidFin(K3Process *p)
{
    delete p;
    emit addUidFinished(m_success, this);
}

void KgpgInterface::generateKey(const QString &keyname, const QString &keyemail, const QString &keycomment, const KgpgKeyAlgo &keyalgo, const uint &keysize, const uint &keyexp, const uint &keyexpnumber)
{
    m_partialline.clear();
    m_ispartial = false;
    step = 3;
    m_success = 0;

    if ((!keyemail.isEmpty()) && ((keyemail.contains(" ")) || !keyemail.contains('.') || !keyemail.contains('@')))
    {
        emit generateKeyFinished(4, this, keyname, keyemail, QString(), QString());
        return;
    }

    m_newkeyid.clear();
    m_newfingerprint.clear();
    m_keyname = keyname;
    m_keyemail = keyemail;
    m_keycomment = keycomment;
    m_keyalgo = keyalgo;
    m_keysize = keysize;
    m_keyexp = keyexp;
    m_keyexpnumber = keyexpnumber;

    K3ProcIO *process = gpgProc(2, 0);
    process->setParent(this);
    *process << "--no-verbose" << "--no-greeting";
    *process << "--gen-key";

    kDebug(2100) << "Generate a new key-pair";
    connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(generateKeyProcess(K3ProcIO *)));
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(generateKeyFin(K3Process *)));
    process->start(K3Process::NotifyOnExit, true);
}

void KgpgInterface::generateKeyProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;

    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline = "";
                m_ispartial = false;
            }

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
                    p->writeStdin(QString("5"), true);
                else
                    p->writeStdin(QString("1"), true);
            }
            else
            if (line.contains("keygen.size"))
                p->writeStdin(QString::number(m_keysize), true);
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

                p->writeStdin(output, true);
            }
            else
            if (line.contains("keygen.name")) {
		if (m_success == 10) {
			p->kill();
		} else {
			m_success = 10;
			p->writeStdin(m_keyname, true);
		}
            } else
            if (line.contains("keygen.email"))
                p->writeStdin(m_keyemail, true);
            else
            if (line.contains("keygen.comment"))
                p->writeStdin(m_keycomment, true);
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
                    delete p;
                    emit generateKeyFinished(3, this, m_keyname, m_keyemail, QString(), QString());
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
                m_newkeyid = line.right(8);
                m_success = 2;
            }
            else
            if (line.contains("GET_"))
            {
                p->writeStdin(QByteArray("quit"), true);
                p->closeWhenDone();
            }
        }
    }

    p->ackRead();
}

void KgpgInterface::generateKeyFin(K3Process *p)
{
    delete p;
    emit generateKeyFinished(m_success, this, m_keyname, m_keyemail, m_newkeyid, m_newfingerprint);
}

void KgpgInterface::decryptFile(const KUrl &src, const KUrl &dest, const QStringList &Options)
{
    m_partialline.clear();
    m_ispartial = false;
    anonymous = false;
    step = 3;
    m_success = 0;
    decryptFileUrl = src;

    K3ProcIO *process = gpgProc(2, 0);
    process->setParent(this);
    *process << "--no-verbose" << "--no-greeting";

    *process << Options;

    if (!dest.fileName().isEmpty())
        *process << "-o" << dest.path();
    *process << "-d" << src.path();

    kDebug(2100) << "Decrypt a file";
    connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(decryptFileProcess(K3ProcIO *)));
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(decryptFileFin(K3Process *)));
    process->start(K3Process::NotifyOnExit, true);
}

void KgpgInterface::decryptFileProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline = "";
                m_ispartial = false;
            }

            if (line.contains("BEGIN_DECRYPTION", Qt::CaseInsensitive))
                emit decryptFileStarted(decryptFileUrl);

            if (line.contains("USERID_HINT", Qt::CaseInsensitive))
                updateIDs(line);

            if ((line.contains("ENC_TO")) && line.contains("0000000000000000"))
                anonymous = true;

            if (line.contains("GET_"))
            {
                if (line.contains("openfile.overwrite.okay"))
                    p->writeStdin(QByteArray("Yes"));
                else
                if ((line.contains("passphrase.enter")))
                {
                    if (userIDs.isEmpty())
                        userIDs = i18n("[No user id found]");
                    userIDs.replace(QRegExp("<"), "&lt;");

                    QString passdlgmessage;
                    if (anonymous)
                        passdlgmessage = i18n("<p><b>No user id found</b>. Trying all secret keys.</p>");
                    if ((step < 3) && (!anonymous))
                        passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
                    passdlgmessage += i18n("Enter passphrase for <b>%1</b>", userIDs);

                    if (sendPassphrase(passdlgmessage, p))
                    {
                        delete p;
                        emit decryptFileFinished(3, this);
                        return;
                    }
                    step--;
                }
                else
                {
                    p->writeStdin(QByteArray("quit"));
                    p->closeWhenDone();
                }
            }
        }
    }

    p->ackRead();
}

void KgpgInterface::decryptFileFin(K3Process *p)
{
    delete p;
    if (message.contains("DECRYPTION_OKAY") && message.contains("END_DECRYPTION"))
        emit decryptFileFinished(5, this);
    else
        emit decryptFileFinished(0, this);
}

void KgpgInterface::downloadKeys(const QStringList &keys, const QString &keyserver, const bool &refresh, const QString &proxy)
{
    m_partialline.clear();
    m_ispartial = false;
    m_downloadkeys.clear();
    m_downloadkeys_log.clear();

    m_downloadprocess = gpgProc(1, 0);

    if (proxy.isEmpty())
        *m_downloadprocess << "--keyserver-options" << "no-honor-http-proxy";
    else
    {
        *m_downloadprocess << "--keyserver-options" << "honor-http-proxy";
        m_downloadprocess->setEnvironment("http_proxy", proxy);
    }

    *m_downloadprocess << "--keyserver" << keyserver;
    if (refresh)
        *m_downloadprocess << "--refresh-keys";
    else
        *m_downloadprocess << "--recv-keys";
    *m_downloadprocess << keys;

    connect(m_downloadprocess, SIGNAL(processExited(K3Process *)), this, SLOT(downloadKeysFin(K3Process *)));
    connect(m_downloadprocess, SIGNAL(readReady(K3ProcIO *)), this, SLOT(downloadKeysProcess(K3ProcIO *)));
    m_downloadprocess->start(K3Process::NotifyOnExit, false);
}

void KgpgInterface::downloadKeysAbort()
{
    if (m_downloadprocess && m_downloadprocess->isRunning())
    {
        disconnect(m_downloadprocess, 0, 0, 0);
        m_downloadprocess->kill();

        delete m_downloadprocess;
        m_downloadprocess = 0;

        emit downloadKeysAborted(this);
    }
}

void KgpgInterface::downloadKeysProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline = "";
                m_ispartial = false;
            }

            if (line.startsWith("[GNUPG:]"))
                m_downloadkeys += line + '\n';
            else
            if  (line.startsWith("gpgkeys: "))
                m_downloadkeys_log += line.mid(9) + '\n';
            else
                m_downloadkeys_log += line + '\n';
        }
    }

    p->ackRead();
}

void KgpgInterface::downloadKeysFin(K3Process *p)
{
    delete p;
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
    m_partialline.clear();
    m_ispartial = false;
    m_uploadkeys_log.clear();

    m_uploadprocess = gpgProc(1, 0);

    if (proxy.isEmpty())
        *m_uploadprocess << "--keyserver-options" << "no-honor-http-proxy";
    else
    {
        *m_uploadprocess << "--keyserver-options" << "honor-http-proxy";
        m_uploadprocess->setEnvironment("http_proxy", proxy);
    }

    *m_uploadprocess << "--keyserver" << keyserver;
    *m_uploadprocess << "--export-options";
    if (attributes.isEmpty())
        *m_uploadprocess << "no-export-attributes";
    else
        *m_uploadprocess << attributes;
    *m_uploadprocess << "--send-keys";
    *m_uploadprocess << keys;

    connect(m_uploadprocess, SIGNAL(processExited(K3Process *)), this, SLOT(uploadKeysFin(K3Process *)));
    connect(m_uploadprocess, SIGNAL(readReady(K3ProcIO *)), this, SLOT(uploadKeysProcess(K3ProcIO *)));
    m_uploadprocess->start(K3Process::NotifyOnExit, true);
}

void KgpgInterface::uploadKeysAbort()
{
    if (m_uploadprocess && m_uploadprocess->isRunning())
    {
        disconnect(m_uploadprocess, 0, 0, 0);
        m_uploadprocess->kill();

        delete m_uploadprocess;
        m_uploadprocess = 0;

        emit uploadKeysAborted(this);
    }
}

void KgpgInterface::uploadKeysProcess(K3ProcIO *p)
{
    QString line;
    bool partial = false;
    while (p->readln(line, false, &partial) != -1)
    {
        if (partial == true)
        {
            m_partialline += line;
            m_ispartial = true;
            partial = false;
        }
        else
        {
            if (m_ispartial)
            {
                m_partialline += line;
                line = m_partialline;

                m_partialline = "";
                m_ispartial = false;
            }

            if (line.startsWith("gpg: "))
                m_uploadkeys_log += line.mid(5) + '\n';
        }
    }

    p->ackRead();
}

void KgpgInterface::uploadKeysFin(K3Process *p)
{
    delete p;
    m_uploadprocess = 0;

    emit uploadKeysFinished(m_uploadkeys_log, this);
}

// decrypt file to text
void KgpgInterface::KgpgDecryptFileToText(const KUrl &srcUrl, const QStringList &Options)
{
    message.clear();
    userIDs.clear();
    step = 3;
    anonymous = false;
    decfinished = false;
    decok = false;
    badmdc = false;
    m_textlength = 0;

    K3ProcIO *process = gpgProc(1, 0);
    *process << "--no-batch" << "-o" << "-";

    for (QStringList::ConstIterator it = Options.begin(); it != Options.end(); ++it)
        *process << QFile::encodeName(*it);

    *process << "-d" << QFile::encodeName(srcUrl.path());

    // when process ends, update dialog infos
    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(decryptTextFin(K3Process *)));
    connect(process, SIGNAL(receivedStdout(K3Process *, char *, int)), this, SLOT(decryptTextStdOut(K3Process *, char *, int)));
    connect(process, SIGNAL(receivedStderr(K3Process *, char *, int)), this, SLOT(decryptTextStdErr(K3Process *, char *, int)));
    process->start(K3Process::NotifyOnExit, false);
}

// signatures
void KgpgInterface::KgpgSignFile(const QString &keyID, const KUrl &srcUrl, const QStringList &Options)
{
    message.clear();
    step = 3;

    K3ProcIO *process = gpgProc(2, 0);
    *process << "-u" << keyID.simplified().toLocal8Bit();

    for (QStringList::ConstIterator it = Options.begin(); it != Options.end(); ++it)
        *process << QFile::encodeName(*it);

    *process << "--output" << QFile::encodeName(srcUrl.path() + ".sig");
    *process << "--detach-sig" << QFile::encodeName(srcUrl.path());

    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(signfin(K3Process *)));
    connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(readsignprocess(K3ProcIO *)));
    process->start(K3Process::NotifyOnExit, true);
}

void KgpgInterface::signfin(K3Process *p)
{
    delete p;

    if (message.contains("SIG_CREATED"))
        KMessageBox::information(0, i18n("The signature file %1 was successfully created.", file.fileName()));
    else
    if (message.contains("BAD_PASSPHRASE"))
        KMessageBox::sorry(0, i18n("Bad passphrase, signature was not created."));
    else
        KMessageBox::sorry(0,message);

    emit signfinished();
}

void KgpgInterface::readsignprocess(K3ProcIO *p)
{
    QString required;
    while (p->readln(required, true) != -1)
    {
        if (required.contains("USERID_HINT", Qt::CaseInsensitive))
            updateIDs(required);

        if (required.contains("GET_"))
        {
            if (required.contains("openfile.overwrite.okay"))
                p->writeStdin(QByteArray("Yes"));
            else
            if ((required.contains("passphrase.enter")))
            {
                if (userIDs.isEmpty())
                    userIDs = i18n("[No user id found]");

                QString passdlgmessage;
                if (step < 3)
                    passdlgmessage = i18n("<p><b>Bad passphrase</b>. You have %1 tries left.</p>", step);
                passdlgmessage += i18n("Enter passphrase for <b>%1</b>", checkForUtf8bis(userIDs));

                if (sendPassphrase(passdlgmessage, p))
                {
                    delete p;
                    emit signfinished();
                    return;
                }

                userIDs.clear();
                step--;
            }
            else
            {
                p->writeStdin(QByteArray("quit"));
                p->closeWhenDone();
            }
        }
        message += required + '\n';
    }
}

void KgpgInterface::KgpgVerifyFile(const KUrl &sigUrl, const KUrl &srcUrl)
{
    message.clear();
    signID.clear();
    signmiss = false;

    file = sigUrl;

    K3ProcIO *process = gpgProc(2);
    *process << "--verify";
    if (!srcUrl.isEmpty())
        *process << QFile::encodeName(srcUrl.path());
    *process << QFile::encodeName(sigUrl.path());

    connect(process, SIGNAL(processExited(K3Process *)), this, SLOT(verifyfin(K3Process *)));
    connect(process, SIGNAL(readReady(K3ProcIO *)), this, SLOT(readprocess(K3ProcIO *)));
    process->start(K3Process::NotifyOnExit,true);
}

void KgpgInterface::readprocess(K3ProcIO *p)
{
    QString required;
    while (p->readln(required,true) != -1)
    {
        message += required + '\n';
        if (required.contains("GET_"))
        {
            p->writeStdin(QByteArray("quit"));
            p->closeWhenDone();
        }

        required = required.section("]", 1, -1).simplified();
        if (required.startsWith("UNEXPECTED") || required.startsWith("NODATA"))
            signID = i18n("No signature found.");

        if (required.startsWith("GOODSIG"))
            signID = i18n("<qt>Good signature from:<br /><b>%1</b><br />Key ID: %2</qt>", required.section(" ", 2, -1).replace(QRegExp("<"), "&lt;"), required.section(" ", 1, 1).right(8));

        if (required.startsWith("BADSIG"))
    signID=i18n("<qt><b>BAD signature</b> from:<br /> %1<br />Key id: %2<br /><br /><b>The file is corrupted</b></qt>", required.section(" ", 2, -1).replace(QRegExp("<"), "&lt;"), required.section(" ", 1, 1).right(8));

        if (required.startsWith("NO_PUBKEY"))
        {
            signmiss = true;
            signID = required.section(" ", 1, 1).right(8);
        }

        if (required.startsWith("TRUST_UNDEFINED"))
            signID += i18n("The signature is valid, but the key is untrusted");

        if (required.startsWith("TRUST_ULTIMATE"))
            signID += i18n("The signature is valid, and the key is ultimately trusted");
    }
}

void KgpgInterface::verifyfin(K3Process *)
{
    if (!signmiss)
    {
        if (signID.isEmpty())
            signID = i18n("No signature found.");

        (void) new KgpgDetailedInfo(0, signID, message);
    }
    else
    {
        if (KMessageBox::questionYesNo(0, i18n("<qt><b>Missing signature:</b><br />Key id: %1<br /><br />Do you want to import this key from a keyserver?</qt>", signID), file.fileName(), KGuiItem(i18n("Import")), KGuiItem(i18n("Do Not Import"))) == KMessageBox::Yes)
        emit verifyquerykey(signID);
    }
    emit verifyfinished();
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

        K3ProcIO *conprocess = gpgProc(2, 0);
        *conprocess << "--edit-key" << keyID << "uid" << uid << "delsig";
        connect(conprocess,SIGNAL(readReady(K3ProcIO *)),this,SLOT(delsigprocess(K3ProcIO *)));
        connect(conprocess, SIGNAL(processExited(K3Process *)),this, SLOT(delsignover(K3Process *)));
        conprocess->start(K3Process::NotifyOnExit,true);
}

void KgpgInterface::delsigprocess(K3ProcIO *p)//ess *p,char *buf, int buflen)
{

        QString required = QString();
        while (p->readln(required,true)!=-1)
        {
                if (required.contains("keyedit.delsig")){

                        if ((sigsearch==signb) && (step==0)) {
                                p->writeStdin(QByteArray("Y"), true);
                                step=1;
                        } else
                                p->writeStdin(QByteArray("n"), true);
                        sigsearch++;
                        required.clear();
                } else if ((step==1) && required.contains("keyedit.prompt")) {
                        p->writeStdin(QByteArray("save"), true);
                        required.clear();
                        deleteSuccess=true;
                } else if (required.contains("GET_LINE")) {
                        p->writeStdin(QByteArray("quit"), true);
                        p->closeWhenDone();
                        deleteSuccess=false;
                }
        }
}

void KgpgInterface::delsignover(K3Process *)
{
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
        K3ProcIO *conprocess = gpgProc(2, 0);
        *conprocess << "--logger-fd=2";
        if (!revokeUrl.isEmpty())
                *conprocess<<"-o"<<revokeUrl;
        *conprocess<<"--gen-revoke"<<keyID;
        QObject::connect(conprocess, SIGNAL(processExited(K3Process *)),this, SLOT(revokeover(K3Process *)));
        QObject::connect(conprocess, SIGNAL(readReady(K3ProcIO *)),this, SLOT(revokeprocess(K3ProcIO *)));
        conprocess->start(K3Process::NotifyOnExit,true);
}

void KgpgInterface::revokeover(K3Process *)
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

void KgpgInterface::revokeprocess(K3ProcIO *p)
{
        QString required = QString();
        while (p->readln(required,true)!=-1) {
                output += required + '\n';

                if (required.contains("USERID_HINT",Qt::CaseInsensitive))
                    updateIDs(required);

                if (required.contains("GOOD_PASSPHRASE"))
                        revokeSuccess=true;

                if (required.contains("gen_revoke.okay") || required.contains("ask_revocation_reason.okay") || required.contains("openfile.overwrite.okay")) {
                        p->writeStdin(QByteArray("YES"));
                        required.clear();
                }

                if (required.contains("ask_revocation_reason.code")) {
                        p->writeStdin(QString::number(revokeReason));
                        required.clear();
                }

                if (required.contains("passphrase.enter"))
                {
                    if (sendPassphrase(i18n("<qt>Enter passphrase for <b>%1</b>:</qt>", checkForUtf8bis(userIDs)), p))
                    {
                                expSuccess=3;  /////  aborted by user mode
                                p->writeStdin(QByteArray("quit"));
                                p->closeWhenDone();
                                return;
                        }
                        required.clear();

                }
                if (required.contains("ask_revocation_reason.text")) {
                        //      kDebug(2100)<<"description";
                        p->writeStdin(revokeDescription);
                        revokeDescription.clear();
                        required.clear();
                }
                if ((required.contains("GET_"))) /////// gpg asks for something unusal, turn to konsole mode
                {
                        kDebug(2100)<<"unknown request";
                        expSuccess=1;  /////  switching to console mode
                        p->writeStdin(QByteArray("quit"));
                        p->closeWhenDone();

                }
        }
}

K3ProcIO *KgpgInterface::gpgProc(const int &statusfd, const int &cmdfd)
{
    K3ProcIO *process = new K3ProcIO(QTextCodec::codecForName("utf8"));
    *process << KGpgSettings::gpgBinaryPath() << "--no-secmem-warning" << "--no-tty";

    if (statusfd >= 0)
        *process << "--status-fd=" + QString::number(statusfd);

    if (cmdfd >= 0)
        *process << "--command-fd=" + QString::number(cmdfd);

    return process;
}

void KgpgInterface::findSigns(const QString &keyID, const QStringList &ids, const QString &uid, QList<int> *res)
{
	K3ProcIO* listproc = gpgProc(1);
	*listproc << "--with-colons" << "--list-sigs" << keyID;
	listproc->start(K3Process::Block, false);

	QString line;
	int curuid = 1;
	int signs = 0;
	int tgtuid = uid.toInt();

	while (listproc->readln(line, true) != -1) {
		if (line.startsWith("sig:") && (tgtuid == curuid)) {
			if (ids.contains(line.section(':', 4, 4), Qt::CaseInsensitive))
				*res << signs;
			signs++;
		} else if (line.startsWith("uid:")) {
			curuid++;
		}
	}
}

#include "kgpginterface.moc"
