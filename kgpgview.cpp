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
#include <QMimeData>
#include <QFile>

#include <KDialogButtonBox>
#include <kio/netaccess.h>
#include <KMessageBox>
#include <KLocale>
#include <KAction>

#include "selectsecretkey.h"
#include "kgpgsettings.h"
#include "kgpginterface.h"
#include "keyservers.h"
#include "kgpgeditor.h"
#include "selectpublickeydialog.h"
#include "detailedconsole.h"

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
    //KgpgSelectPublicKeyDlg *dialog = new KgpgSelectPublicKeyDlg(this, 0, false, true, (static_cast<KgpgEditor*>(parent()))->m_godefaultkey);
    KgpgSelectPublicKeyDlg *dialog = new KgpgSelectPublicKeyDlg(this);  // TODO default key
    if (dialog->exec() == KDialog::Accepted)
    {
        QStringList options;
        options << "--armor";

        if (dialog->getUntrusted()) options << "--always-trust";
        if (dialog->getHideId())    options << "--throw-keyid";

        QString customoptions = dialog->getCustomOptions();
        if (!customoptions.isEmpty())
            if (KGpgSettings::allowCustomEncryptionOptions())
                options << customoptions.split(" ");

        if (KGpgSettings::pgpCompatibility())
            options << "--pgp6";

        QStringList listkeys;
        if (!dialog->getSymmetric())
            listkeys = dialog->selectedKeys();

        KgpgInterface *interface = new KgpgInterface();
        connect(interface, SIGNAL(txtEncryptionFinished(QString, KgpgInterface*)), this, SLOT(slotEncodeUpdate(QString, KgpgInterface*)));
        interface->encryptText(toPlainText(), listkeys, options);
    }
    delete dialog;
}

void KgpgTextEdit::slotDecode()
{
    QString startmsg = QString("-----BEGIN PGP MESSAGE-----");
    QString endmsg = QString("-----END PGP MESSAGE-----");

    QString fullcontent = toPlainText();

    m_posstart = fullcontent.indexOf(startmsg);
    if (m_posstart == -1)
        return;

    m_posend = fullcontent.indexOf(endmsg, m_posstart);
    if (m_posend == -1)
        return;
    m_posend += endmsg.length();

    // decode data from the editor. triggered by the decode button
    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(txtDecryptionFinished(QString, KgpgInterface*)), this, SLOT(slotDecodeUpdateSuccess(QString, KgpgInterface*)));
    connect(interface, SIGNAL(txtDecryptionFailed(QString, KgpgInterface*)), this, SLOT(slotDecodeUpdateFailed(QString, KgpgInterface*)));
    interface->decryptText(fullcontent.mid(m_posstart, m_posend - m_posstart), KGpgSettings::customDecrypt().simplified().split(" "));
}

void KgpgTextEdit::slotSign()
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

    QStringList options = QStringList();
    if (KGpgSettings::pgpCompatibility())
        options << "--pgp6";

    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(txtSigningFinished(QString, KgpgInterface*)), this, SLOT(slotSignUpdate(QString, KgpgInterface*)));
    interface->signText(toPlainText(), signkeyid, options);
}

void KgpgTextEdit::slotVerify()
{
    QString startmsg = QString("-----BEGIN PGP SIGNED MESSAGE-----");
    QString endmsg = QString("-----END PGP SIGNATURE-----");

    QString fullcontent = toPlainText();

    int posstart = fullcontent.indexOf(startmsg);
    if (posstart == -1)
        return;

    int posend = fullcontent.indexOf(endmsg, posstart);
    if (posend == -1)
        return;
    posend += endmsg.length();

    // this is a signed message, verify it
    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(txtVerifyMissingSignature(QString, KgpgInterface*)), this, SLOT(slotVerifyKeyNeeded(QString, KgpgInterface*)));
    connect(interface, SIGNAL(txtVerifyFinished(QString, QString, KgpgInterface*)), this, SLOT(slotVerifySuccess(QString, QString, KgpgInterface*)));
    interface->verifyText(fullcontent.mid(posstart, posend - posstart));
}

void KgpgTextEdit::deleteFile()
{
    if (!m_tempfile.isEmpty())
    {
        KIO::NetAccess::removeTempFile(m_tempfile);     // TODO shred the file
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

    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(txtDecryptionFinished(QString, KgpgInterface*)), this, SLOT(editorUpdateDecryptedtxt(QString, KgpgInterface*)));
    connect(interface, SIGNAL(txtDecryptionFailed(QString, KgpgInterface*)), this, SLOT(editorFailedDecryptedtxt(QString, KgpgInterface*)));
    interface->KgpgDecryptFileToText(KUrl(m_tempfile), KGpgSettings::customDecrypt().simplified().split(" "));
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

    if (checkforpgpmessage && result.startsWith("-----BEGIN PGP MESSAGE"))
    {
        // if pgp data found, decode it
        slotDecodeFile();
        return true;
    }

    if (result.startsWith("-----BEGIN PGP PUBLIC KEY BLOCK") || result.startsWith("-----BEGIN PGP PRIVATE KEY BLOCK"))
    {
        // dropped file is a public key or a private key
        bool ispublickey = false;
        QString tmpinfo;
        if (result.startsWith("-----BEGIN PGP PUBLIC KEY BLOCK"))
        {
            ispublickey = true;
            tmpinfo = i18n("<qt>This file is a <b>public</b> key.\nPlease use kgpg key management to import it.</qt>");
        }

        if (result.startsWith("-----BEGIN PGP PRIVATE KEY BLOCK"))
        {
            ispublickey = false;
            tmpinfo = i18n("<qt>This file is a <b>private</b> key.\nPlease use kgpg key management to import it.</qt>");
        }

        KMessageBox::information(this, tmpinfo);

        /*
        if (ispublickey)
        {
            int result = KMessageBox::warningContinueCancel(this, i18n("<p>The file <b>%1</b> is a public key.<br>Do you want to import it ?</p>").arg(filetocheck), i18n("Warning"), KGuiItem (i18n("Import"), QString(), i18n("Import the public key"), i18n("Import the public key in your keyring")));
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
    emit newText();
    return false;
}

void KgpgTextEdit::editorUpdateDecryptedtxt(const QString &content, KgpgInterface *interface)
{
    delete interface;
    setPlainText(content);
    emit newText();
}

void KgpgTextEdit::editorFailedDecryptedtxt(const QString &content, KgpgInterface *interface)
{
    delete interface;
    if (!slotCheckFile(false))
        KMessageBox::detailedSorry(this, i18n("Decryption failed."), content);
}

void KgpgTextEdit::slotEncodeUpdate(const QString &content, KgpgInterface *interface)
{
    delete interface;
    if (!content.isEmpty())
    {
        setPlainText(content);
        emit newText();
    }
    else
        KMessageBox::sorry(this, i18n("Encryption failed."));
}

void KgpgTextEdit::slotDecodeUpdateSuccess(const QString &content, KgpgInterface *interface)
{
    delete interface;

    QString decryptedcontent;
    if (checkForUtf8(content))
    {
        decryptedcontent = QString::fromUtf8(content.toAscii());
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

    emit newText();
}

void KgpgTextEdit::slotDecodeUpdateFailed(const QString &content, KgpgInterface *interface)
{
    delete interface;
    KMessageBox::detailedSorry(this, i18n("Decryption failed."), content);
}

void KgpgTextEdit::slotSignUpdate(const QString &content, KgpgInterface *interface)
{
    delete interface;
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

    emit newText();
}

void KgpgTextEdit::slotVerifySuccess(const QString &content, const QString &log, KgpgInterface *interface)
{
    delete interface;
    emit verifyFinished();
    (void) new KgpgDetailedInfo(this, content, log);
}

void KgpgTextEdit::slotVerifyKeyNeeded(const QString &id, KgpgInterface *interface)
{
    delete interface;

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






// main view configuration
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
    if (mess.contains("-----BEGIN PGP SIGNED"))
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
