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

#include <QDragEnterEvent>
#include <QVBoxLayout>
#include <QTextStream>
#include <QDropEvent>
#include <QFile>

#include <Q3TextDrag>

#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kbuttonbox.h>
#include <klocale.h>
#include <kaction.h>

#include "selectsecretkey.h"
#include "kgpgsettings.h"
#include "kgpginterface.h"
#include "keyservers.h"
#include "kgpgeditor.h"
#include "popuppublic.h"
#include "detailedconsole.h"
#include "kgpgview.h"

KgpgTextEdit::KgpgTextEdit(QWidget *parent, const char *name)
            : KTextEdit(name, parent)
{
    setTextFormat(Qt::PlainText);
    setCheckSpellingEnabled(true);
    setAcceptDrops(true);
}

KgpgTextEdit::~KgpgTextEdit()
{
    if (!m_tempfile.isEmpty())
    {
        KIO::NetAccess::removeTempFile(m_tempfile);
        m_tempfile = QString::null;
    }
}

void KgpgTextEdit::contentsDragEnterEvent(QDragEnterEvent *e)
{
    // if a file is dragged into editor ...
    e->setAccepted(KUrl::List::canDecode(e->mimeData()) || Q3TextDrag::canDecode(e));
}

void KgpgTextEdit::contentsDropEvent(QDropEvent *e)
{
    // decode dropped file
    QString text;
    KUrl::List uriList = KUrl::List::fromMimeData(e->mimeData());
    if (!uriList.isEmpty())
        slotDroppedFile(uriList.first());
    else
    if (Q3TextDrag::decode(e, text))
        insertPlainText(text);
}

void KgpgTextEdit::slotDroppedFile(const KUrl &url)
{
    if (!m_tempfile.isEmpty())
    {
        KIO::NetAccess::removeTempFile(m_tempfile);
        m_tempfile = QString::null;
    }

    if (url.isLocalFile())
        m_tempfile = url.path();
    else
    {
        if (KMessageBox::warningContinueCancel(this, i18n("<qt><b>Remote file dropped</b>.<br>The remote file will now be copied to a temporary file to process requested operation. This temporary file will be deleted after operation.</qt>"), QString::null, KStdGuiItem::cont(), "RemoteFileWarning") != KMessageBox::Continue)
            return;

        if (!KIO::NetAccess::download(url, m_tempfile, this))
        {
            KMessageBox::sorry(this, i18n("Could not download file."));
            return;
        }
    }

    // if dropped filename ends with gpg, pgp or asc, try to decode it
    if ((m_tempfile.endsWith(".gpg")) || (m_tempfile.endsWith(".asc")) || (m_tempfile.endsWith(".pgp")))
        slotDecodeFile(m_tempfile);
    else
        slotCheckContent(m_tempfile);
}

bool KgpgTextEdit::slotCheckContent(const QString &filetocheck, const bool &checkforpgpmessage)
{
    QFile qfile(filetocheck);
    if (qfile.open(QIODevice::ReadOnly))
    {
        // open file
        QTextStream t(&qfile);
        QString result = t.readAll();

        if ((checkforpgpmessage) && (result.startsWith("-----BEGIN PGP MESSAGE")))
        {
            // if pgp data found, decode it
            qfile.close();
            slotDecodeFile(filetocheck);
            return true;
        }
        else
        if ((result.startsWith("-----BEGIN PGP PUBLIC KEY BLOCK")) || (result.startsWith("-----BEGIN PGP PRIVATE KEY BLOCK")))
        {
            // dropped file is a public key or a private key
            QString tmpinfo;
            if (result.startsWith("-----BEGIN PGP PUBLIC KEY BLOCK"))
                tmpinfo = i18n("This file is a public key.\nPlease use kgpg key management to import it.");
            if (result.startsWith("-----BEGIN PGP PRIVATE KEY BLOCK"))
                tmpinfo = i18n("This file is a private key.\nPlease use kgpg key management to import it.");

            qfile.close();
            KMessageBox::information(this, tmpinfo);
            KIO::NetAccess::removeTempFile(filetocheck); // TODO try to SHRED the file (more secure if it is a secret key)
            return true;
        }
        else
        {
            setPlainText(result);
            qfile.close();
            KIO::NetAccess::removeTempFile(filetocheck);
        }
    }

    return false;
}

void KgpgTextEdit::slotDecodeFile(const QString &fname)
{
    // decode file from given url into editor
    QFile qfile(QFile::encodeName(fname));
    if (qfile.open(QIODevice::ReadOnly))
    {
        KgpgInterface *interface = new KgpgInterface();
        connect(interface, SIGNAL(txtDecryptionFinished(QString, KgpgInterface*)), this, SLOT(editorUpdateDecryptedtxt(QString, KgpgInterface*)));
        connect(interface, SIGNAL(txtDecryptionFailed(QString, KgpgInterface*)), this, SLOT(editorFailedDecryptedtxt(QString, KgpgInterface*)));
        interface->KgpgDecryptFileToText(KUrl(fname), KGpgSettings::customDecrypt().simplified().split(" "));
    }
    else
        KMessageBox::sorry(this, i18n("Unable to read file."));
}

void KgpgTextEdit::editorUpdateDecryptedtxt(const QString &newtxt, KgpgInterface *interface)
{
    delete interface;
    setPlainText(newtxt);
}

void KgpgTextEdit::editorFailedDecryptedtxt(const QString &newtxt, KgpgInterface *interface)
{
    delete interface;
    if (!slotCheckContent(m_tempfile, false))
        KMessageBox::detailedSorry(this, i18n("Decryption failed."), newtxt);
}

// main view configuration
KgpgView::KgpgView(QWidget *parent, const char *name)
        : QWidget(parent, name)
{
    editor = new KgpgTextEdit(this);
    editor->setReadOnly(false);
    editor->setUndoRedoEnabled(true);

    setAcceptDrops(true);

    KButtonBox *boutonbox = new KButtonBox(this, Qt::Horizontal, 15, 12);
    boutonbox->addStretch(1);
    boutonbox->addButton(i18n("S&ign/Verify"), this, SLOT(slotSignVerify()), true);
    boutonbox->addButton(i18n("En&crypt"), this, SLOT(slotEncode()), true);
    boutonbox->addButton(i18n("&Decrypt"), this, SLOT(slotDecode()), true);

    connect(editor, SIGNAL(textChanged()), this, SIGNAL(textChanged()));

    editor->resize(editor->maximumSize());

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setSpacing(3);
    vbox->addWidget(editor);
    vbox->addWidget(boutonbox);
}

KgpgView::~KgpgView()
{
    delete editor;
}

void KgpgView::slotSignVerify()
{
    QString mess = editor->toPlainText();
    if (mess.startsWith("-----BEGIN PGP SIGNED"))
    {
        // this is a signed message, verify it
        KgpgInterface *interface = new KgpgInterface();
        connect(interface, SIGNAL(txtVerifyMissingSignature(QString, KgpgInterface*)), this, SLOT(slotAskForImport(QString, KgpgInterface*)));
        connect(interface, SIGNAL(txtVerifyFinished(QString, QString, KgpgInterface*)), this, SLOT(slotVerifyResult(QString, QString, KgpgInterface*)));
        interface->verifyText(mess);
    }
    else
    {
        // Sign the text in Editor
        QString signkeyid;

        // open key selection dialog
        KgpgSelectSecretKey *opts = new KgpgSelectSecretKey(this);
        if (opts->exec() == QDialog::Accepted)
            signkeyid = opts->getKeyID();
        else
        {
            delete opts;
            return;
        }

        delete opts;

        KgpgInterface *interface = new KgpgInterface();
        connect(interface, SIGNAL(txtSigningFinished(QString, KgpgInterface*)), this, SLOT(slotSignResult(QString, KgpgInterface*)));
        QStringList options = QStringList();
        if (KGpgSettings::pgpCompatibility())
            options << "--pgp6";
        interface->signText(mess, signkeyid, options);
    }
}

void KgpgView::slotEncode()
{
    // dialog to select public key for encryption
    KgpgSelectPublicKeyDlg *dialog = new KgpgSelectPublicKeyDlg(this, "public_keys", 0, false, (static_cast<KgpgEditor*>(parent()))->m_godefaultkey);
    connect(dialog, SIGNAL(selectedKey(QStringList, QStringList, bool, bool)), this, SLOT(encodeTxt(QStringList, QStringList, bool, bool)));
    dialog->exec();
    delete dialog;
}

void KgpgView::slotDecode()
{
    // decode data from the editor. triggered by the decode button
    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(txtDecryptionFinished(QString, KgpgInterface*)), this, SLOT(updateDecryptedtxt(QString, KgpgInterface*)));
    connect(interface, SIGNAL(txtDecryptionFailed(QString, KgpgInterface*)), this, SLOT(failedDecryptedtxt(QString, KgpgInterface*)));
    interface->decryptText(editor->toPlainText(), KGpgSettings::customDecrypt().simplified().split(" "));
}

void KgpgView::updateDecryptedtxt(const QString &newtxt, KgpgInterface *interface)
{
    delete interface;
    if (checkForUtf8(newtxt))
    {
        editor->setPlainText(QString::fromUtf8(newtxt.toAscii()));
        emit resetEncoding(true);
    }
    else
    {
        editor->setPlainText(newtxt);
        emit resetEncoding(false);
    }
    emit newText();
}

void KgpgView::failedDecryptedtxt(const QString &newtxt, KgpgInterface *interface)
{
    delete interface;
    KMessageBox::detailedSorry(this, i18n("Decryption failed."), newtxt);
}

void KgpgView::slotAskForImport(const QString &id, KgpgInterface *interface)
{
    delete interface;
    KGuiItem importitem = KStdGuiItem::yes();
    importitem.setText(i18n("&Import"));
    importitem.setToolTip(i18n("Import key in your list"));

    KGuiItem noimportitem = KStdGuiItem::no();
    noimportitem.setText(i18n("Do &Not Import"));
    noimportitem.setToolTip(i18n("Will not import this key in your list"));

    if (KMessageBox::questionYesNo(this, i18n("<qt><b>Missing signature:</b><br>Key id: %1<br><br>Do you want to import this key from a keyserver?</qt>", id), i18n("Missing Key"), importitem, noimportitem) == KMessageBox::Yes)
    {
        keyServer *kser = new keyServer(0, "server_dialog", false, true);
        kser->slotSetText(id);
        kser->slotImport();
    }
    else
        emit verifyFinished();
}

void KgpgView::slotVerifyResult(const QString &mssge, const QString &log, KgpgInterface *interface)
{
    delete interface;
    emit verifyFinished();
    (void) new KgpgDetailedInfo(0, "verify_result", mssge, log);
}

void KgpgView::slotSignResult(const QString &signResult, KgpgInterface*)
{
    if (signResult.isEmpty())
        KMessageBox::sorry(this, i18n("Signing not possible: bad passphrase or missing key"));
    else
    {
        if (checkForUtf8(signResult))
        {
            editor->setPlainText(QString::fromUtf8(signResult.toAscii()));
            emit resetEncoding(true);
        }
        else
        {
            editor->setPlainText(signResult);
            emit resetEncoding(false);
        }
        emit newText();
    }
}

void KgpgView::encodeTxt(QStringList selec, QStringList encryptoptions, bool, bool symmetric)
{
    // encode from editor
    if (KGpgSettings::pgpCompatibility())
        encryptoptions << "--pgp6";

    if (symmetric)
        selec.clear();

    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(txtEncryptionFinished(QString, KgpgInterface*)), this, SLOT(updateTxt(QString, KgpgInterface*)));
    interface->encryptText(editor->toPlainText(), selec, encryptoptions);
}

void KgpgView::updateTxt(const QString &newtxt, KgpgInterface *interface)
{
    delete interface;
    if (!newtxt.isEmpty())
    {
        editor->setPlainText(newtxt);
        emit newText();
    }
    else
        KMessageBox::sorry(this, i18n("Encryption failed."));
}

bool KgpgView::checkForUtf8(const QString &text)
{
    // try to guess if the decrypted text uses utf-8 encoding
    QTextCodec *codec = QTextCodec::codecForLocale();
    if (!codec->canEncode(text))
        return true;
    return false;
}

#include "kgpgview.moc"
