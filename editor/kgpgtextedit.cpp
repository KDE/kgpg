/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2009 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgtextedit.h"

#include <QDragEnterEvent>
#include <QTextStream>
#include <QTextCodec>
#include <QDropEvent>
#include <QFile>

#include <kio/netaccess.h>
#include <KMessageBox>
#include <KLocale>

#include "selectsecretkey.h"
#include "kgpgsettings.h"
#include "kgpgtextinterface.h"
#include "keyservers.h"
#include "kgpgeditor.h"
#include "selectpublickeydialog.h"
#include "detailedconsole.h"

#define ENCODEDMESSAGE_BEGIN "-----BEGIN PGP MESSAGE-----"
#define ENCODEDMESSAGE_END   "-----END PGP MESSAGE-----"
#define SIGNEDMESSAGE_BEGIN  "-----BEGIN PGP SIGNED MESSAGE-----"
#define SIGNEDMESSAGE_END    "-----END PGP SIGNATURE-----"
#define PUBLICKEY_BEGIN      "-----BEGIN PGP PUBLIC KEY BLOCK-----"
#define PUBLICKEY_END        "-----END PGP PUBLIC KEY BLOCK-----"
#define PRIVATEKEY_BEGIN     "-----BEGIN PGP PRIVATE KEY BLOCK-----"
#define PRIVATEKEY_END       "-----END PGP PRIVATE KEY BLOCK-----"

KgpgTextEdit::KgpgTextEdit(QWidget *parent, KGpgItemModel *model)
            : KTextEdit(parent),
            m_model(model)
{
    setCheckSpellingEnabled(true);
    setAcceptDrops(true);
    setReadOnly(false);
    setUndoRedoEnabled(true);
}

KgpgTextEdit::~KgpgTextEdit()
{
    deleteFile();
}

void KgpgTextEdit::dragEnterEvent(QDragEnterEvent *e)
{
    // if a file is dragged into editor ...
    if (KUrl::List::canDecode(e->mimeData()) || e->mimeData()->hasText())
        e->acceptProposedAction();
}

void KgpgTextEdit::dropEvent(QDropEvent *e)
{
    // decode dropped file or dropped text
    KUrl::List uriList = KUrl::List::fromMimeData(e->mimeData());
    if (!uriList.isEmpty())
        slotDroppedFile(uriList.first());
    else
    if (e->mimeData()->hasText())
        insertPlainText(e->mimeData()->text());
}

void KgpgTextEdit::slotDroppedFile(const KUrl &url)
{
    deleteFile();

    if (url.isLocalFile())
        m_tempfile = url.path();
    else
    {
        if (KMessageBox::warningContinueCancel(this, i18n("<qt><b>Remote file dropped</b>.<br />The remote file will now be copied to a temporary file to process requested operation. This temporary file will be deleted after operation.</qt>"), QString(), KStandardGuiItem::cont(), KStandardGuiItem::cancel(), "RemoteFileWarning") != KMessageBox::Continue)
            return;

        if (!KIO::NetAccess::download(url, m_tempfile, this))
        {
            KMessageBox::sorry(this, i18n("Could not download file."));
            return;
        }
    }

    // if dropped filename ends with gpg, pgp or asc, try to decode it
    if (m_tempfile.endsWith(QLatin1String(".gpg")) || m_tempfile.endsWith(QLatin1String(".asc")) || m_tempfile.endsWith(QLatin1String(".pgp")))
        slotDecodeFile();
    else
        slotCheckFile();
}

void KgpgTextEdit::slotEncode()
{
#ifdef __GNUC__
#warning FIXME goDefaultKey shortcut
#endif /* _GNUC_ */
    QPointer<KgpgSelectPublicKeyDlg> dialog = new KgpgSelectPublicKeyDlg(this, m_model, KShortcut(QKeySequence(Qt::CTRL + Qt::Key_Home)), true);
    if (dialog->exec() == KDialog::Accepted)
    {
        QStringList options;
        if (dialog->getArmor())     options << "--armor";
        if (dialog->getUntrusted()) options << "--always-trust";
        if (dialog->getHideId())    options << "--throw-keyid";

        QString customoptions = dialog->getCustomOptions();
        if (!customoptions.isEmpty())
            if (KGpgSettings::allowCustomEncryptionOptions())
                options << customoptions.split(' ', QString::SkipEmptyParts);

        if (KGpgSettings::pgpCompatibility())
            options << "--pgp6";

        QStringList listkeys;
        if (!dialog->getSymmetric())
            listkeys = dialog->selectedKeys();

        KGpgTextInterface *interface = new KGpgTextInterface();
        connect(interface, SIGNAL(txtEncryptionFinished(QString)), SLOT(slotEncodeUpdate(QString)));
        interface->encryptText(toPlainText(), listkeys, options);
    }
    delete dialog;
}

void KgpgTextEdit::slotDecode()
{
    QString startmsg = ENCODEDMESSAGE_BEGIN;
    QString endmsg = ENCODEDMESSAGE_END;

    QString fullcontent = toPlainText();

    m_posstart = fullcontent.indexOf(startmsg);
    if (m_posstart == -1)
        return;

    m_posend = fullcontent.indexOf(endmsg, m_posstart);
    if (m_posend == -1)
        return;
    m_posend += endmsg.length();

    KGpgTextInterface *interface = new KGpgTextInterface();
    connect(interface, SIGNAL(txtDecryptionFinished(QByteArray)), SLOT(slotDecodeUpdateSuccess(QByteArray)));
    connect(interface, SIGNAL(txtDecryptionFailed(QString)), this, SLOT(slotDecodeUpdateFailed(QString)));
    interface->decryptText(fullcontent.mid(m_posstart, m_posend - m_posstart), KGpgSettings::customDecrypt().simplified().split(' ', QString::SkipEmptyParts));
}

void KgpgTextEdit::slotSign(const QString &message)
{
    QString signkeyid;

    QPointer<KgpgSelectSecretKey> opts = new KgpgSelectSecretKey(this, m_model);
    if (opts->exec() == QDialog::Accepted)
        signkeyid = opts->getKeyID();
    else
    {
        delete opts;
        return;
    }

    delete opts;

    QStringList options;
    if (KGpgSettings::pgpCompatibility())
        options << "--pgp6";

    KGpgTextInterface *interface = new KGpgTextInterface();
    connect(interface, SIGNAL(txtSigningFinished(QString)), SLOT(slotSignUpdate(QString)));
    interface->signText(message, signkeyid, options);
}

void KgpgTextEdit::slotVerify(const QString &message)
{
    const QString startmsg(SIGNEDMESSAGE_BEGIN);
    const QString endmsg(SIGNEDMESSAGE_END);

    int posstart = message.indexOf(startmsg);
    if (posstart == -1)
        return;

    int posend = message.indexOf(endmsg, posstart);
    if (posend == -1)
        return;
    posend += endmsg.length();

    KGpgTextInterface *interface = new KGpgTextInterface();
    connect(interface, SIGNAL(txtVerifyMissingSignature(QString)), SLOT(slotVerifyKeyNeeded(QString)));
    connect(interface, SIGNAL(txtVerifyFinished(QString, QString)), SLOT(slotVerifySuccess(QString, QString)));
    interface->verifyText(message.mid(posstart, posend - posstart));
}

void KgpgTextEdit::deleteFile()
{
    if (!m_tempfile.isEmpty())
    {
        KIO::NetAccess::removeTempFile(m_tempfile);
        m_tempfile.clear();
    }
}

bool KgpgTextEdit::checkForUtf8(const QString &text)
{
    // try to guess if the decrypted text uses utf-8 encoding
    QTextCodec *codec = QTextCodec::codecForLocale();
    if (!codec->canEncode(text))
        return true;
    return false;
}

void KgpgTextEdit::slotDecodeFile()
{
    // decode file from given url into editor
    QFile qfile(m_tempfile);
    if (!qfile.open(QIODevice::ReadOnly))
    {
        KMessageBox::sorry(this, i18n("Unable to read file."));
        return;
    }
    qfile.close();

    KGpgTextInterface *interface = new KGpgTextInterface();
    connect(interface, SIGNAL(txtDecryptionFinished(QByteArray)), SLOT(slotDecodeFileSuccess(QByteArray)));
    connect(interface, SIGNAL(txtDecryptionFailed(QString)),SLOT(slotDecodeFileFailed(QString)));
    interface->KgpgDecryptFileToText(KUrl(m_tempfile), KGpgSettings::customDecrypt().simplified().split(' ', QString::SkipEmptyParts));
}

bool KgpgTextEdit::slotCheckFile(const bool &checkforpgpmessage)
{
    QString result;

    QFile qfile(m_tempfile);
    if (qfile.open(QIODevice::ReadOnly))
    {
        QTextStream t(&qfile);
        result = t.readAll();
        qfile.close();
    }

    if (result.isEmpty())
        return false;

    if (checkforpgpmessage && result.startsWith(ENCODEDMESSAGE_BEGIN))
    {
        // if pgp data found, decode it
        slotDecodeFile();
        return true;
    }

	QString tmpinfo;

	switch (checkForKey(result)) {
	case 1:
		tmpinfo = i18n("<qt>This file is a <b>public</b> key.\nPlease use kgpg key management to import it.</qt>");
		break;
	case 2:
		tmpinfo = i18n("<qt>This file is a <b>private</b> key.\nPlease use kgpg key management to import it.</qt>");
		break;
        }

	if (!tmpinfo.isEmpty())
		KMessageBox::information(this, tmpinfo);
	else
		setPlainText(result);

	deleteFile();

	return tmpinfo.isEmpty();
}

int KgpgTextEdit::checkForKey(const QString &message)
{
	if (message.startsWith(PUBLICKEY_BEGIN)) {
		return 1;
	} else if (message.startsWith(PRIVATEKEY_BEGIN)) {
		return 2;
	} else {
		return 0;
	}
}

void KgpgTextEdit::slotDecodeFileSuccess(const QByteArray &content)
{
    sender()->deleteLater();
#ifdef __GNUC__
#warning FIXME choose codec
#endif
    setPlainText(content);
}

void KgpgTextEdit::slotDecodeFileFailed(const QString &content)
{
    sender()->deleteLater();
    if (!slotCheckFile(false))
        KMessageBox::detailedSorry(this, i18n("Decryption failed."), content);
}

void KgpgTextEdit::slotEncodeUpdate(const QString &content)
{
    sender()->deleteLater();
    if (!content.isEmpty())
    {
        setPlainText(content);
    }
    else
        KMessageBox::sorry(this, i18n("Encryption failed."));
}

void KgpgTextEdit::slotDecodeUpdateSuccess(const QByteArray &content)
{
    sender()->deleteLater();

    QString decryptedcontent;
    if (checkForUtf8(content))
    {
        decryptedcontent = QString::fromUtf8(content);
        emit resetEncoding(true);
    }
    else
    {
        decryptedcontent = content;
        emit resetEncoding(false);
    }

    QString fullcontent = toPlainText();
    fullcontent.replace(m_posstart, m_posend - m_posstart, decryptedcontent);
    setPlainText(fullcontent);
}

void KgpgTextEdit::slotDecodeUpdateFailed(const QString &content)
{
    sender()->deleteLater();
    if (!content.contains("gpg: cancelled by user"))
        KMessageBox::detailedSorry(this, i18n("Decryption failed."), content);
}

void KgpgTextEdit::slotSignUpdate(const QString &content)
{
    sender()->deleteLater();
    if (content.isEmpty())
    {
        KMessageBox::sorry(this, i18n("Signing not possible: bad passphrase or missing key"));
        return;
    }

    if (checkForUtf8(content))
    {
        setPlainText(QString::fromUtf8(content.toAscii()));
        emit resetEncoding(true);
    }
    else
    {
        setPlainText(content);
        emit resetEncoding(false);
    }
}

void KgpgTextEdit::slotVerifySuccess(const QString &content, const QString &log)
{
    sender()->deleteLater();
    emit verifyFinished();
    (void) new KgpgDetailedInfo(this, content, log);
}

void KgpgTextEdit::slotVerifyKeyNeeded(const QString &id)
{
    sender()->deleteLater();

    KGuiItem importitem = KStandardGuiItem::yes();
    importitem.setText(i18n("&Import"));
    importitem.setToolTip(i18n("Import key in your list"));

    KGuiItem noimportitem = KStandardGuiItem::no();
    noimportitem.setText(i18n("Do &Not Import"));
    noimportitem.setToolTip(i18n("Will not import this key in your list"));

    if (KMessageBox::questionYesNo(this, i18n("<qt><b>Missing signature:</b><br />Key id: %1<br /><br />Do you want to import this key from a keyserver?</qt>", id), i18n("Missing Key"), importitem, noimportitem) == KMessageBox::Yes)
    {
        KeyServer *kser = new KeyServer(0, m_model, true);
        kser->slotSetText(id);
        kser->slotImport();
    }
    else
        emit verifyFinished();
}

void KgpgTextEdit::slotSignVerify()
{
	signVerifyText(toPlainText());
}

void KgpgTextEdit::signVerifyText(const QString &message)
{
	if (message.contains(SIGNEDMESSAGE_BEGIN))
		slotVerify(message);
	else
		slotSign(message);
}

void KgpgTextEdit::slotHighlightText(const QString &, const int &matchingindex, const int &matchedlength)
{
	highlightWord(matchedlength, matchingindex);
}

#include "kgpgtextedit.moc"
