/***************************************************************************
                          kgpgview.cpp  -  description
                             -------------------
    begin                : Tue Jul  2 12:31:38 GMT 2002
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

#include "kgpgview.h"

#include <QDragEnterEvent>
#include <QVBoxLayout>
#include <QTextStream>
#include <QDropEvent>
#include <QFile>

#include <KDialogButtonBox>
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

KgpgTextEdit::KgpgTextEdit(QWidget *parent)
            : KTextEdit(parent)
{
    setCheckSpellingEnabled(true);
    setAcceptDrops(true);
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
    if (m_tempfile.endsWith(".gpg") || m_tempfile.endsWith(".asc") || m_tempfile.endsWith(".pgp"))
        slotDecodeFile();
    else
        slotCheckFile();
}

void KgpgTextEdit::slotEncode()
{
    // TODO : goDefaultKey shortcut
    KgpgSelectPublicKeyDlg *dialog = new KgpgSelectPublicKeyDlg(this, 0, KShortcut(QKeySequence(Qt::CTRL + Qt::Key_Home)), true);
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
        connect(interface, SIGNAL(txtEncryptionFinished(QString, KGpgTextInterface*)), this, SLOT(slotEncodeUpdate(QString, KGpgTextInterface*)));
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
    connect(interface, SIGNAL(txtDecryptionFinished(QByteArray, KGpgTextInterface*)), this, SLOT(slotDecodeUpdateSuccess(QByteArray, KGpgTextInterface*)));
    connect(interface, SIGNAL(txtDecryptionFailed(QString, KGpgTextInterface*)), this, SLOT(slotDecodeUpdateFailed(QString, KGpgTextInterface*)));
    interface->decryptText(fullcontent.mid(m_posstart, m_posend - m_posstart), KGpgSettings::customDecrypt().simplified().split(' ', QString::SkipEmptyParts));
}

void KgpgTextEdit::slotSign()
{
    QString signkeyid;

    KgpgSelectSecretKey *opts = new KgpgSelectSecretKey(this);
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
    connect(interface, SIGNAL(txtSigningFinished(QString, KGpgTextInterface*)), this, SLOT(slotSignUpdate(QString, KGpgTextInterface*)));
    interface->signText(toPlainText(), signkeyid, options);
}

void KgpgTextEdit::slotVerify()
{
    QString startmsg = QString(SIGNEDMESSAGE_BEGIN);
    QString endmsg = QString(SIGNEDMESSAGE_END);

    QString fullcontent = toPlainText();

    int posstart = fullcontent.indexOf(startmsg);
    if (posstart == -1)
        return;

    int posend = fullcontent.indexOf(endmsg, posstart);
    if (posend == -1)
        return;
    posend += endmsg.length();

    KGpgTextInterface *interface = new KGpgTextInterface();
    connect(interface, SIGNAL(txtVerifyMissingSignature(QString, KGpgTextInterface*)), this, SLOT(slotVerifyKeyNeeded(QString, KGpgTextInterface*)));
    connect(interface, SIGNAL(txtVerifyFinished(QString, QString, KGpgTextInterface*)), this, SLOT(slotVerifySuccess(QString, QString, KGpgTextInterface*)));
    interface->verifyText(fullcontent.mid(posstart, posend - posstart));
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
    QFile qfile(QFile::encodeName(m_tempfile));
    if (!qfile.open(QIODevice::ReadOnly))
    {
        KMessageBox::sorry(this, i18n("Unable to read file."));
        return;
    }
    qfile.close();

    KGpgTextInterface *interface = new KGpgTextInterface();
    connect(interface, SIGNAL(txtDecryptionFinished(QByteArray, KGpgTextInterface*)), this, SLOT(slotDecodeFileSuccess(QByteArray, KGpgTextInterface*)));
    connect(interface, SIGNAL(txtDecryptionFailed(QString, KGpgTextInterface*)), this, SLOT(slotDecodeFileFailed(QString, KGpgTextInterface*)));
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

    if (result.startsWith(PUBLICKEY_BEGIN) || result.startsWith(PRIVATEKEY_BEGIN))
    {
        // dropped file is a public key or a private key
        bool ispublickey = false;
        QString tmpinfo;
        if (result.startsWith(PUBLICKEY_BEGIN))
        {
            ispublickey = true;
            tmpinfo = i18n("<qt>This file is a <b>public</b> key.\nPlease use kgpg key management to import it.</qt>");
        }

        if (result.startsWith(PRIVATEKEY_BEGIN))
        {
            ispublickey = false;
            tmpinfo = i18n("<qt>This file is a <b>private</b> key.\nPlease use kgpg key management to import it.</qt>");
        }

        KMessageBox::information(this, tmpinfo);

        /*
        if (ispublickey)
        {
            int result = KMessageBox::warningContinueCancel(this, i18n("<p>The file <b>%1</b> is a public key.<br>Do you want to import it ?</p>").arg(filetocheck), QString(), KGuiItem (i18n("Import"), QString(), i18n("Import the public key"), i18n("Import the public key in your keyring")));
            if (result == KMessageBox::Continue)
            {
                //TODO : import key
            }
        }
        else
            KMessageBox::information(this, i18n("This file is a private key.\nPlease use kgpg key management to import it."));
        */

        deleteFile();
        return true;
    }

    setPlainText(result);
    deleteFile();
    return false;
}

void KgpgTextEdit::slotDecodeFileSuccess(const QByteArray &content, KGpgTextInterface *interface)
{
    interface->deleteLater();
#ifdef __GNUC__
#warning FIXME choose codec
#endif
    setPlainText(content);
}

void KgpgTextEdit::slotDecodeFileFailed(const QString &content, KGpgTextInterface *interface)
{
    interface->deleteLater();
    if (!slotCheckFile(false))
        KMessageBox::detailedSorry(this, i18n("Decryption failed."), content);
}

void KgpgTextEdit::slotEncodeUpdate(const QString &content, KGpgTextInterface *interface)
{
    interface->deleteLater();
    if (!content.isEmpty())
    {
        setPlainText(content);
    }
    else
        KMessageBox::sorry(this, i18n("Encryption failed."));
}

void KgpgTextEdit::slotDecodeUpdateSuccess(const QByteArray &content, KGpgTextInterface *interface)
{
    interface->deleteLater();

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

void KgpgTextEdit::slotDecodeUpdateFailed(const QString &content, KGpgTextInterface *interface)
{
    interface->deleteLater();
    KMessageBox::detailedSorry(this, i18n("Decryption failed."), content);
}

void KgpgTextEdit::slotSignUpdate(const QString &content, KGpgTextInterface *interface)
{
    interface->deleteLater();
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

void KgpgTextEdit::slotVerifySuccess(const QString &content, const QString &log, KGpgTextInterface *interface)
{
    interface->deleteLater();
    emit verifyFinished();
    (void) new KgpgDetailedInfo(this, content, log);
}

void KgpgTextEdit::slotVerifyKeyNeeded(const QString &id, KGpgTextInterface *interface)
{
    interface->deleteLater();

    KGuiItem importitem = KStandardGuiItem::yes();
    importitem.setText(i18n("&Import"));
    importitem.setToolTip(i18n("Import key in your list"));

    KGuiItem noimportitem = KStandardGuiItem::no();
    noimportitem.setText(i18n("Do &Not Import"));
    noimportitem.setToolTip(i18n("Will not import this key in your list"));

    if (KMessageBox::questionYesNo(this, i18n("<qt><b>Missing signature:</b><br />Key id: %1<br /><br />Do you want to import this key from a keyserver?</qt>", id), i18n("Missing Key"), importitem, noimportitem) == KMessageBox::Yes)
    {
        KeyServer *kser = new KeyServer(0, false, true);
        kser->slotSetText(id);
        kser->slotImport();
    }
    else
        emit verifyFinished();
}






KgpgView::KgpgView(QWidget *parent)
        : QWidget(parent)
{
    editor = new KgpgTextEdit(this);
    editor->setReadOnly(false);
    editor->setUndoRedoEnabled(true);

    setAcceptDrops(true);

    KDialogButtonBox *buttonbox = new KDialogButtonBox(this, Qt::Horizontal);
    buttonbox->addButton(i18n("S&ign/Verify"), KDialogButtonBox::ActionRole, this, SLOT(slotSignVerify()));
    buttonbox->addButton(i18n("En&crypt"), KDialogButtonBox::ActionRole, this, SLOT(slotEncode()));
    buttonbox->addButton(i18n("&Decrypt"), KDialogButtonBox::ActionRole, this, SLOT(slotDecode()));

    connect(editor, SIGNAL(textChanged()), this, SIGNAL(textChanged()));
    connect(editor, SIGNAL(newText()), this, SIGNAL(newText()));
    connect(editor, SIGNAL(resetEncoding(bool)), this, SIGNAL(resetEncoding(bool)));
    connect(editor, SIGNAL(verifyFinished()), this, SIGNAL(verifyFinished()));

    editor->resize(editor->maximumSize());

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setSpacing(3);
    vbox->addWidget(editor);
    vbox->addWidget(buttonbox);
}

KgpgView::~KgpgView()
{
    delete editor;
}

void KgpgView::slotSignVerify()
{
    QString mess = editor->toPlainText();
    if (mess.contains(SIGNEDMESSAGE_BEGIN))
        editor->slotVerify();
    else
        editor->slotSign();
}

void KgpgView::slotEncode()
{
    editor->slotEncode();
}

void KgpgView::slotDecode()
{
    editor->slotDecode();
}

void KgpgView::slotHighlightText(const QString &, const int &matchingindex, const int &matchedlength)
{
    editor->highlightWord(matchedlength, matchingindex);
}

#include "kgpgview.moc"
