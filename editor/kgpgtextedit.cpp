/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2009,2010,2011 Rolf Eike Beer <kde@opensource.sf-tec.de>
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

#include "selectsecretkey.h"
#include "kgpgsettings.h"
#include "keyservers.h"
#include "selectpublickeydialog.h"
#include "detailedconsole.h"
#include "keysmanager.h"
#include "editor/kgpgeditor.h"
#include "transactions/kgpgdecrypt.h"
#include "transactions/kgpgencrypt.h"
#include "transactions/kgpgimport.h"
#include "transactions/kgpgsigntext.h"
#include "transactions/kgpgverify.h"

#include <KLocale>
#include <KMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QTextCodec>
#include <QTextStream>
#include <kio/netaccess.h>

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
	openDroppedFile(url, true);
}

void KgpgTextEdit::openDroppedFile(const KUrl& url, const bool probe)
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

	if (!probe || KGpgDecrypt::isEncryptedText(result, &m_posstart, &m_posend)) {
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
	QPointer<KgpgSelectPublicKeyDlg> dialog = new KgpgSelectPublicKeyDlg(this, m_model, m_keysmanager->goDefaultShortcut(), true);
	if (dialog->exec() == KDialog::Accepted) {
		QStringList options;
		KGpgEncrypt::EncryptOptions opts = KGpgEncrypt::DefaultEncryption;

		if (dialog->getArmor())
			opts |= KGpgEncrypt::AsciiArmored;

		if (dialog->getUntrusted())
			opts |= KGpgEncrypt::AllowUntrustedEncryption;

		if (dialog->getHideId())
			opts |= KGpgEncrypt::HideKeyId;

		if (KGpgSettings::allowCustomEncryptionOptions()) {
			const QString customoptions = dialog->getCustomOptions();
			if (!customoptions.isEmpty())
				options << customoptions.split(QLatin1Char(' '), QString::SkipEmptyParts);
		}

		if (KGpgSettings::pgpCompatibility())
			options << QLatin1String( "--pgp6" );

		QStringList listkeys;
		if (!dialog->getSymmetric())
			listkeys = dialog->selectedKeys();

		KGpgEncrypt *encr = new KGpgEncrypt(this, listkeys, toPlainText(), opts, options);
		encr->start();
		connect(encr, SIGNAL(done(int)), SLOT(slotEncodeUpdate(int)));
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

	KGpgSignText *signt = new KGpgSignText(this, signkeyid, message);
	connect(signt, SIGNAL(done(int)), SLOT(slotSignUpdate(int)));
	signt->start();
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

    KGpgVerify *verify = new KGpgVerify(this, message.mid(posstart, posend - posstart));
    connect(verify, SIGNAL(done(int)), SLOT(slotVerifyDone(int)));
    verify->start();
}

void KgpgTextEdit::slotDecryptDone(int result)
{
	KGpgDecrypt *decr = qobject_cast<KGpgDecrypt *>(sender());
	Q_ASSERT(decr != NULL);

	if (!m_tempfile.isEmpty()) {
		KIO::NetAccess::removeTempFile(m_tempfile);
		m_tempfile.clear();
	}

	if (result == KGpgTransaction::TS_OK) {
		// FIXME choose codec
		setPlainText(decr->decryptedText().join(QLatin1String("\n")) + QLatin1Char('\n'));
	} else if (result != KGpgTransaction::TS_USER_ABORTED) {
		KMessageBox::detailedSorry(this, i18n("Decryption failed."), decr->getMessages().join( QLatin1String( "\n" )));
	}

	decr->deleteLater();
}

void KgpgTextEdit::slotEncodeUpdate(int result)
{
	KGpgEncrypt *enc = qobject_cast<KGpgEncrypt *>(sender());
	Q_ASSERT(enc != NULL);

	if (result == KGpgTransaction::TS_OK) {
		const QString lf = QLatin1String("\n");
		setPlainText(enc->encryptedText().join(lf) + lf);
	} else {
		KMessageBox::sorry(this, i18n("The encryption failed with error code %1", result),
				i18n("Encryption failed."));
	}

	sender()->deleteLater();
}

void KgpgTextEdit::slotSignUpdate(int result)
{
	const KGpgSignText * const signt = qobject_cast<KGpgSignText *>(sender());
	sender()->deleteLater();
	Q_ASSERT(signt != NULL);

	if (result != KGpgTransaction::TS_OK) {
		KMessageBox::sorry(this, i18n("Signing not possible: bad passphrase or missing key"));
		return;
	}

	const QString content = signt->signedText().join(QLatin1String("\n")) + QLatin1String("\n");

    setPlainText(content);
    emit resetEncoding(false);
}

void KgpgTextEdit::slotVerifyDone(int result)
{
	const KGpgVerify * const verify = qobject_cast<KGpgVerify *>(sender());
	sender()->deleteLater();
	Q_ASSERT(verify != NULL);

	emit verifyFinished();

	if (result == KGpgVerify::TS_MISSING_KEY) {
		verifyKeyNeeded(verify->missingId());
		return;
	}

	const QStringList messages = verify->getMessages();

	if (messages.isEmpty())
		return;

	QStringList msglist;
	foreach (QString rawmsg, messages) // krazy:exclude=foreach
		msglist << rawmsg.replace(QLatin1Char('<'), QLatin1String("&lt;"));

	(void) new KgpgDetailedInfo(this, KGpgVerify::getReport(messages, m_model),
			msglist.join(QLatin1String("<br/>")),
			QStringList(), i18nc("Caption of message box", "Verification Finished"));
}

void KgpgTextEdit::verifyKeyNeeded(const QString &id)
{
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

void KgpgTextEdit::slotHighlightText(const QString &, const int matchingindex, const int matchedlength)
{
	highlightWord(matchedlength, matchingindex);
}

#include "kgpgtextedit.moc"
