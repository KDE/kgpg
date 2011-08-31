/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2009,2010 Rolf Eike Beer <kde@opensource.sf-tec.de>
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
#include "kgpgdecrypt.h"
#include "kgpgimport.h"
#include "keysmanager.h"

#define SIGNEDMESSAGE_BEGIN  QLatin1String( "-----BEGIN PGP SIGNED MESSAGE-----" )
#define SIGNEDMESSAGE_END    QLatin1String( "-----END PGP SIGNATURE-----" )

KgpgTextEdit::KgpgTextEdit(QWidget *parent, KGpgItemModel *model, KeysManager *manager)
            : KTextEdit(parent),
            m_posstart(-1),
            m_posend(-1),
            m_model(model),
            m_keysmanager(manager)
{
    setCheckSpellingEnabled(true);
    setAcceptDrops(true);
    setReadOnly(false);
    setUndoRedoEnabled(true);
}

KgpgTextEdit::~KgpgTextEdit()
{
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
	KUrl tmpurl;

	if (url.isLocalFile()) {
		m_tempfile = url.path();
		tmpurl = url;
	} else {
		if (KMessageBox::warningContinueCancel(this, i18n("<qt><b>Remote file dropped</b>.<br />The remote file will now be copied to a temporary file to process requested operation. This temporary file will be deleted after operation.</qt>"), QString(), KStandardGuiItem::cont(), KStandardGuiItem::cancel(), QLatin1String( "RemoteFileWarning" )) != KMessageBox::Continue)
			return;

		if (!KIO::NetAccess::download(url, m_tempfile, this)) {
			KMessageBox::sorry(this, i18n("Could not download file."));
			return;
		}
		tmpurl = KUrl::fromPath(m_tempfile);
	}

	QString result;

	QFile qfile(m_tempfile);
	if (qfile.open(QIODevice::ReadOnly)) {
		QTextStream t(&qfile);
		result = t.readAll();
		qfile.close();
	}

	if (result.isEmpty())
		return;

	if (KGpgDecrypt::isEncryptedText(result, &m_posstart, &m_posend)) {
		// if pgp data found, decode it
		KGpgDecrypt *decr = new KGpgDecrypt(this, KUrl::List(tmpurl));
		connect(decr, SIGNAL(done(int)), SLOT(slotDecryptDone(int)));
		decr->start();
		return;
	}
	// remove only here, as KGpgDecrypt will use and remove the file itself
	KIO::NetAccess::removeTempFile(m_tempfile);
	m_tempfile.clear();

	QString tmpinfo;

	switch (KGpgImport::isKey(result)) {
	case 1:
		tmpinfo = i18n("<qt>This file is a <b>public</b> key.<br />Do you want to import it instead of opening it in editor?</qt>");
		break;
	case 2:
		tmpinfo = i18n("<qt>This file is a <b>private</b> key.<br />Do you want to import it instead of opening it in editor?</qt>");
		break;
        }

	if (!tmpinfo.isEmpty()) {
		if (KMessageBox::questionYesNo(this, tmpinfo, i18n("Key file dropped on Editor")) != KMessageBox::Yes) {
			setPlainText(result);
		} else {
			KGpgImport *imp = new KGpgImport(this, result);
			connect(imp, SIGNAL(done(int)), m_keysmanager, SLOT(slotImportDone(int)));
			imp->start();
		}
	} else {
		if (m_posstart != -1) {
			Q_ASSERT(m_posend != -1);
			QString fullcontent = toPlainText();
			fullcontent.replace(m_posstart, m_posend - m_posstart, result);
			setPlainText(fullcontent);
			m_posstart = -1;
			m_posend = -1;
		} else {
			setPlainText(result);
		}
	}
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
        if (dialog->getArmor())     options << QLatin1String( "--armor" );
        if (dialog->getUntrusted()) options << QLatin1String( "--always-trust" );
        if (dialog->getHideId())    options << QLatin1String( "--throw-keyid" );

        QString customoptions = dialog->getCustomOptions();
        if (!customoptions.isEmpty())
            if (KGpgSettings::allowCustomEncryptionOptions())
                options << customoptions.split(QLatin1Char( ' ' ), QString::SkipEmptyParts);

        if (KGpgSettings::pgpCompatibility())
            options << QLatin1String( "--pgp6" );

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
	const QString fullcontent = toPlainText();

	if (!KGpgDecrypt::isEncryptedText(fullcontent, &m_posstart, &m_posend))
		return;

	KGpgDecrypt *decr = new KGpgDecrypt(this, fullcontent.mid(m_posstart, m_posend - m_posstart));
	connect(decr, SIGNAL(done(int)), SLOT(slotDecryptDone(int)));
	decr->start();
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
        options << QLatin1String( "--pgp6" );

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

bool KgpgTextEdit::checkForUtf8(const QString &text)
{
    // try to guess if the decrypted text uses utf-8 encoding
    QTextCodec *codec = QTextCodec::codecForLocale();
    if (!codec->canEncode(text))
        return true;
    return false;
}

void KgpgTextEdit::slotDecryptDone(int result)
{
	KGpgDecrypt *decr = qobject_cast<KGpgDecrypt *>(sender());
	Q_ASSERT(decr != NULL);
	Q_ASSERT(!m_tempfile.isEmpty());

	KIO::NetAccess::removeTempFile(m_tempfile);
	m_tempfile.clear();

	if (result == KGpgTransaction::TS_OK) {
#ifdef __GNUC__
#warning FIXME choose codec
#endif
		setPlainText(decr->decryptedText().join(QLatin1String("\n")) + QLatin1Char('\n'));
	} else if (result != KGpgTransaction::TS_USER_ABORTED) {
		KMessageBox::detailedSorry(this, i18n("Decryption failed."), decr->getMessages().join( QLatin1String( "\n" )));
	}

	decr->deleteLater();
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
    (void) new KgpgDetailedInfo(this, content, log, QStringList(), i18nc("Caption of message box", "Verification Finished"));
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
