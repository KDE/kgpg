/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2008,2009,2010,2011,2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgexternalactions.h"

#include "detailedconsole.h"
#include "foldercompressjob.h"
#include "keyservers.h"
#include "keysmanager.h"
#include "kgpgfirstassistant.h"
#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "kgpgtextinterface.h"
#include "selectpublickeydialog.h"
#include "selectsecretkey.h"
#include "core/images.h"
#include "editor/kgpgeditor.h"
#include "editor/kgpgtextedit.h"
#include "transactions/kgpgdecrypt.h"
#include "transactions/kgpgencrypt.h"
#include "transactions/kgpgsigntext.h"
#include "transactions/kgpgtransactionjob.h"
#include "transactions/kgpgverify.h"

#include <KActionCollection>
#include <KMessageBox>
#include <KTar>
#include <KTemporaryFile>
#include <KToolInvocation>
#include <KUniqueApplication>
#include <KZip>
#include <QDesktopWidget>
#include <QFont>
#include <QProcess>
#include <kio/global.h>
#include <kio/renamedialog.h>
#include <kjobtrackerinterface.h>

KGpgExternalActions::KGpgExternalActions(KeysManager *parent, KGpgItemModel *model)
	: QObject(parent),
	m_model(model),
	m_keysmanager(parent)
{
	readOptions();
}

KGpgExternalActions::~KGpgExternalActions()
{
}

void KGpgExternalActions::slotEncryptDroppedFiles(const KUrl::List &urls)
{
	Q_ASSERT(!urls.isEmpty());

	KgpgSelectPublicKeyDlg *dialog = new KgpgSelectPublicKeyDlg(0, m_model, goDefaultKey(), false, urls);
	connect(dialog, SIGNAL(accepted()), SLOT(slotEncryptionKeySelected()));
	connect(dialog, SIGNAL(rejected()), dialog, SLOT(deleteLater()));
	dialog->show();
}

void KGpgExternalActions::slotEncryptionKeySelected()
{
	KgpgSelectPublicKeyDlg *dialog = qobject_cast<KgpgSelectPublicKeyDlg *>(sender());
	Q_ASSERT(dialog != NULL);
	sender()->deleteLater();

	QStringList opts;
	QString defaultKey;

	if (KGpgSettings::encryptFilesTo()) {
		if (KGpgSettings::pgpCompatibility())
			opts << QLatin1String( "--pgp6" );

		defaultKey = KGpgSettings::fileEncryptionKey();
	}

	KGpgEncrypt::EncryptOptions eopt = KGpgEncrypt::DefaultEncryption;

	if (dialog->getUntrusted())
		eopt |= KGpgEncrypt::AllowUntrustedEncryption;
	if (dialog->getArmor())
		eopt |= KGpgEncrypt::AsciiArmored;
	if (dialog->getHideId())
		eopt |= KGpgEncrypt::HideKeyId;

	if (KGpgSettings::allowCustomEncryptionOptions()) {
		const QString customopts(dialog->getCustomOptions().isEmpty());

		if (!customopts.isEmpty())
			opts << customopts.split(QLatin1Char(' '), QString::SkipEmptyParts);
	}

	QStringList keys(dialog->selectedKeys());
	if (!defaultKey.isEmpty() && !keys.contains(defaultKey))
		keys.append(defaultKey);

	if (dialog->getSymmetric())
		keys.clear();

	KGpgEncrypt *enc = new KGpgEncrypt(parent(), keys, dialog->getFiles(), eopt, opts);
	KGpgTransactionJob *encjob = new KGpgTransactionJob(enc);

	KIO::getJobTracker()->registerJob(encjob);
	encjob->start();
}

void KGpgExternalActions::encryptDroppedFolders(const KUrl::List &urls)
{
	compressionScheme = 0;

	KTemporaryFile *tmpfolder = new KTemporaryFile();

	if (!tmpfolder->open()) {
		delete tmpfolder;
		KMessageBox::sorry(m_keysmanager, i18n("Cannot create temporary file for folder compression."), i18n("Temporary File Creation"));
		return;
	}

	if (KMessageBox::Continue != KMessageBox::warningContinueCancel(m_keysmanager,
				i18n("<qt>KGpg will now create a temporary archive file:<br /><b>%1</b> to process the encryption. The file will be deleted after the encryption is finished.</qt>",
				tmpfolder->fileName()), i18n("Temporary File Creation"), KStandardGuiItem::cont(),
				KStandardGuiItem::cancel(), QLatin1String( "FolderTmpFile" ))) {
		delete tmpfolder;
		return;
	}

	m_kgpgfoldertmp = tmpfolder;

	KgpgSelectPublicKeyDlg *dialog = new KgpgSelectPublicKeyDlg(m_keysmanager, m_model, goDefaultKey(), false, urls);

	KHBox *bGroup = new KHBox(dialog->optionsbox);

	(void) new QLabel(i18n("Compression method for archive:"), bGroup);

	KComboBox *optionbx = new KComboBox(bGroup);
	foreach (const QString &aname, FolderCompressJob::archiveNames())
		optionbx->addItem(aname);

	connect(optionbx, SIGNAL(activated(int)), SLOT(slotSetCompression(int)));
	connect(dialog, SIGNAL(accepted()), SLOT(startFolderEncode()));
	connect(dialog, SIGNAL(rejected()), SLOT(slotAbortEnc()));

	dialog->exec();
}

void KGpgExternalActions::slotAbortEnc()
{
	sender()->deleteLater();
	delete m_kgpgfoldertmp;
	m_kgpgfoldertmp = NULL;
}

void KGpgExternalActions::slotSetCompression(int cp)
{
	compressionScheme = cp;
}

void KGpgExternalActions::startFolderEncode()
{
	KgpgSelectPublicKeyDlg *dialog = qobject_cast<KgpgSelectPublicKeyDlg *>(sender());
	Q_ASSERT(dialog != NULL);
	dialog->deleteLater();

	const KUrl::List urls = dialog->getFiles();

	QStringList selec = dialog->selectedKeys();
	KGpgEncrypt::EncryptOptions encOptions = KGpgEncrypt::DefaultEncryption;
	const QStringList encryptOptions = dialog->getCustomOptions().split(QLatin1Char(' '),  QString::SkipEmptyParts);
	if (dialog->getSymmetric()) {
		selec.clear();
	} else {
		Q_ASSERT(!selec.isEmpty());
	}

	QString extension = FolderCompressJob::extensionForArchive(compressionScheme);

	if (dialog->getArmor())
		extension += QLatin1String( ".asc" );
	else if (KGpgSettings::pgpExtension())
		extension += QLatin1String( ".pgp" );
	else
		extension += QLatin1String( ".gpg" );

	if (dialog->getArmor())
		encOptions |= KGpgEncrypt::AsciiArmored;
	if (dialog->getHideId())
		encOptions |= KGpgEncrypt::HideKeyId;
	if (dialog->getUntrusted())
		encOptions |= KGpgEncrypt::AllowUntrustedEncryption;

	KUrl encryptedFile(KUrl::fromPath(urls.first().path(KUrl::RemoveTrailingSlash) + extension));
	QFile encryptedFolder(encryptedFile.path());
	dialog->hide();
	if (encryptedFolder.exists()) {
		QPointer<KIO::RenameDialog> over = new KIO::RenameDialog(m_keysmanager, i18n("File Already Exists"), KUrl(), encryptedFile, KIO::M_OVERWRITE);
		if (over->exec() == QDialog::Rejected) {
			dialog = NULL;
			delete over;
			return;
		}
		encryptedFile = over->newDestUrl();
		delete over;
	}

	FolderCompressJob *trayinfo = new FolderCompressJob(m_keysmanager, urls, encryptedFile, m_kgpgfoldertmp, selec, encryptOptions, encOptions, compressionScheme);
	connect(trayinfo, SIGNAL(result(KJob*)), SLOT(slotFolderFinished(KJob*)));
	KIO::getJobTracker()->registerJob(trayinfo);
	trayinfo->start();
}

void KGpgExternalActions::slotFolderFinished(KJob *job)
{
	FolderCompressJob *trayinfo = qobject_cast<FolderCompressJob *>(job);
	Q_ASSERT(trayinfo != NULL);

	delete m_kgpgfoldertmp;
	m_kgpgfoldertmp = NULL;
	if (trayinfo->error())
		KMessageBox::sorry(m_keysmanager, trayinfo->errorString());
}

void KGpgExternalActions::verifyFile(KUrl url)
{
	// check file signature
	if (url.isEmpty())
		return;

	QString sigfile;
	// try to find detached signature.
	if (!url.fileName().endsWith(QLatin1String(".sig"))) {
		sigfile = url.path() + QLatin1String( ".sig" );
		QFile fsig(sigfile);
		if (!fsig.exists()) {
			sigfile = url.path() + QLatin1String( ".asc" );
			QFile fsig(sigfile);
			// if no .asc or .sig signature file included, assume the file is internally signed
			if (!fsig.exists())
				sigfile.clear();
		}
	} else {
		sigfile = url.path();
		url = KUrl(sigfile.left(sigfile.length() - 4));
	}

	KGpgVerify *kgpv = new KGpgVerify(parent(), KUrl::List(sigfile));
	connect(kgpv, SIGNAL(done(int)), SLOT(slotVerificationDone(int)));
	kgpv->start();
}

void KGpgExternalActions::slotVerificationDone(int result)
{
	KGpgVerify *kgpv = qobject_cast<KGpgVerify *>(sender());
	Q_ASSERT(kgpv != NULL);
	kgpv->deleteLater();

	if (result == KGpgVerify::TS_MISSING_KEY) {
		KeyServer *kser = new KeyServer(m_keysmanager, m_model);
		kser->slotSetText(kgpv->missingId());
		kser->slotImport();
	} else {
		const QStringList messages = kgpv->getMessages();

		if (messages.isEmpty())
			return;

		QStringList msglist;
		foreach (QString rawmsg, messages)
			msglist << rawmsg.replace(QLatin1Char('<'), QLatin1String("&lt;"));

		(void) new KgpgDetailedInfo(m_keysmanager, KGpgVerify::getReport(messages, m_model),
				msglist.join(QLatin1String("<br/>")),
				QStringList(), i18nc("Caption of message box", "Verification Finished"));
	}
}

void KGpgExternalActions::signDroppedFiles(const KUrl::List &urls)
{
	Q_ASSERT(!urls.isEmpty());

	droppedUrls = urls;

	KgpgSelectSecretKey *keydlg = new KgpgSelectSecretKey(0, m_model, false);
	connect(keydlg, SIGNAL(accepted()), SLOT(slotSignFiles()));
	connect(keydlg, SIGNAL(rejected()), keydlg, SLOT(deleteLater()));
	keydlg->show();
}

void KGpgExternalActions::slotSignFiles()
{
	KgpgSelectSecretKey *keydlg = qobject_cast<KgpgSelectSecretKey *>(sender());
	Q_ASSERT(keydlg != NULL);
	sender()->deleteLater();

	const QString signKeyID = keydlg->getKeyID();

	QStringList Options;
	KGpgSignText::SignOptions sopts = KGpgSignText::DetachedSignature;
	if (KGpgSettings::asciiArmor()) {
		Options << QLatin1String( "--armor" );
		sopts |= KGpgSignText::AsciiArmored;
	}
	if (KGpgSettings::pgpCompatibility())
		Options << QLatin1String( "--pgp6" );

	if (droppedUrls.count() > 1) {
		KGpgTextInterface *signFileProcess = new KGpgTextInterface(this, signKeyID, Options);
		connect(signFileProcess, SIGNAL(fileSignFinished()), SLOT(slotSigningFinished()));
		signFileProcess->signFiles(droppedUrls);
	} else {
		KGpgSignText *signt = new KGpgSignText(this, signKeyID, droppedUrls, sopts);
		connect(signt, SIGNAL(done(int)), SLOT(slotSigningFinished()));
		signt->start();
	}
}

void KGpgExternalActions::slotSigningFinished()
{
	sender()->deleteLater();
}

void KGpgExternalActions::decryptDroppedFiles(const KUrl::List &urls)
{
	m_decryptionFailed.clear();

	decryptFile(urls);
}

void KGpgExternalActions::decryptFile(KUrl::List urls)
{
	while (!urls.first().isLocalFile()) {
		showDroppedFile(urls.takeFirst());
	}

	if (urls.isEmpty())
		return;

	KUrl first = urls.first();

	QString oldname(first.fileName());
	if (oldname.endsWith(QLatin1String(".gpg"), Qt::CaseInsensitive) ||
			oldname.endsWith(QLatin1String(".asc"), Qt::CaseInsensitive) ||
			oldname.endsWith(QLatin1String(".pgp"), Qt::CaseInsensitive))
		oldname.chop(4);
	else
		oldname.append(QLatin1String( ".clear" ));

	KUrl swapname(first.directory(KUrl::AppendTrailingSlash) + oldname);
	QFile fgpg(swapname.path());
	if (fgpg.exists()) {
		QPointer<KIO::RenameDialog> over = new KIO::RenameDialog(m_keysmanager, i18n("File Already Exists"), KUrl(), swapname, KIO::M_OVERWRITE);
		if (over->exec() != QDialog::Accepted) {
			delete over;
			urls.pop_front();
			decryptFile(urls);
			return;
		}

		swapname = over->newDestUrl();
		delete over;
	}

	droppedUrls = urls;
	KGpgDecrypt *decr = new KGpgDecrypt(this, droppedUrls.first(), swapname);
	connect(decr, SIGNAL(done(int)), SLOT(slotDecryptionDone(int)));
	decr->start();
}

void KGpgExternalActions::slotDecryptionDone(int status)
{
	KGpgDecrypt *decr = qobject_cast<KGpgDecrypt *>(sender());
	Q_ASSERT(decr != NULL);

	if (status != KGpgTransaction::TS_OK)
		m_decryptionFailed << droppedUrls.first();

	decr->deleteLater();

	droppedUrls.pop_front();

	if (!droppedUrls.isEmpty()) {
		decryptFile(droppedUrls);
	} else if (!m_decryptionFailed.isEmpty()) {
		KMessageBox::errorList(NULL,
					i18np("Decryption of this file failed:", "Decryption of these files failed:",
					m_decryptionFailed.count()), m_decryptionFailed.toStringList(),
					i18n("Decryption failed."));
	}
}

void KGpgExternalActions::showDroppedFile(const KUrl &file)
{
	KgpgEditor *kgpgtxtedit = new KgpgEditor(m_keysmanager, m_model, 0);
	connect(kgpgtxtedit, SIGNAL(encryptFiles(KUrl::List)), SLOT(slotEncryptDroppedFiles(KUrl::List)));
	connect(m_keysmanager, SIGNAL(fontChanged(QFont)), kgpgtxtedit, SLOT(slotSetFont(QFont)));

	kgpgtxtedit->m_editor->openDroppedFile(file, false);

	kgpgtxtedit->show();
}

void KGpgExternalActions::readOptions()
{
	clipboardMode = QClipboard::Clipboard;
	if (KGpgSettings::useMouseSelection() && kapp->clipboard()->supportsSelection())
		clipboardMode = QClipboard::Selection;

	if (KGpgSettings::firstRun()) {
		firstRun();
	} else if (KGpgSettings::gpgConfigPath().isEmpty()) {
		if (KMessageBox::Yes == KMessageBox::questionYesNo(0,
				i18n("<qt>You have not set a path to your GnuPG config file.<br />This may cause some surprising results in KGpg's execution.<br />Would you like to start KGpg's assistant to fix this problem?</qt>"),
				QString(), KGuiItem(i18n("Start Assistant")), KGuiItem(i18n("Do Not Start"))))
			startAssistant();
	}
}

void KGpgExternalActions::firstRun()
{
	QProcess *createConfigProc = new QProcess(this);
	QStringList args;
	args << QLatin1String( "--no-tty" ) << QLatin1String( "--list-secret-keys" );
	createConfigProc->start(QLatin1String( "gpg" ), args);	// start GnuPG so that it will create a config file
	createConfigProc->waitForFinished();
	startAssistant();
}

void KGpgExternalActions::startAssistant()
{
	if (m_assistant.isNull()) {
		m_assistant = new KGpgFirstAssistant(m_keysmanager);

		connect(m_assistant, SIGNAL(accepted()), SLOT(slotSaveOptionsPath()));
		connect(m_assistant, SIGNAL(rejected()), m_assistant, SLOT(deleteLater()));
		connect(m_assistant, SIGNAL(helpClicked()), SLOT(help()));
	}

	m_assistant->show();
}

void KGpgExternalActions::slotSaveOptionsPath()
{
	KGpgSettings::setAutoStart(m_assistant->getAutoStart());
	KGpgSettings::setGpgConfigPath(m_assistant->getConfigPath());
	KGpgSettings::setFirstRun(false);

	const QString gpgConfServer(KgpgInterface::getGpgSetting(QLatin1String( "keyserver" ), KGpgSettings::gpgConfigPath()));
	if (!gpgConfServer.isEmpty()) {
		// The user already had configured a keyserver, set this one as default.
		QStringList serverList(KGpgSettings::keyServers());
		serverList.prepend(gpgConfServer);
		KGpgSettings::setKeyServers(serverList);
	}

	const QString defaultID(m_assistant->getDefaultKey());

	KGpgSettings::self()->writeConfig();
	emit updateDefault(defaultID);
	if (m_assistant->runKeyGenerate())
		emit createNewKey();
	m_assistant->deleteLater();
}

void KGpgExternalActions::help()
{
	KToolInvocation::invokeHelp(QString(), QLatin1String( "kgpg" ));
}

KShortcut KGpgExternalActions::goDefaultKey() const
{
	return qobject_cast<KAction *>(m_keysmanager->actionCollection()->action(QLatin1String( "go_default_key" )))->shortcut();
}

#include "kgpgexternalactions.moc"
