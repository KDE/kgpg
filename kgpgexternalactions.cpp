/***************************************************************************
 *   Copyright 2002 by Jean-Baptiste Mardelle <bj@altern.org>              *
 *   Copyright 2008,2009 by Rolf Eike Beer <kde@opensource.sf-tec.de>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "kgpgexternalactions.h"

#include <QDesktopWidget>
#include <QFont>
#include <QProcess>

#include <KMessageBox>
#include <KPassivePopup>
#include <KTar>
#include <KTemporaryFile>
#include <KToolInvocation>
#include <KUniqueApplication>
#include <KZip>
#include <kio/renamedialog.h>

#include "images.h"
#include "keyservers.h"
#include "keysmanager.h"
#include "kgpgeditor.h"
#include "kgpgfirstassistant.h"
#include "kgpglibrary.h"
#include "kgpgsettings.h"
#include "kgpgtextinterface.h"
#include "kgpgview.h"
#include "selectpublickeydialog.h"
#include "selectsecretkey.h"

KGpgExternalActions::KGpgExternalActions(KeysManager *parent, KGpgItemModel *model)
	: QObject(parent),
	openTasks(0),
	m_model(model),
	m_keysmanager(parent)
{
	readOptions();
}

KGpgExternalActions::~KGpgExternalActions()
{
}

void KGpgExternalActions::encryptDroppedFile()
{
	QStringList opts;
	KgpgLibrary *lib = new KgpgLibrary(0);
	if (KGpgSettings::pgpExtension())
		lib->setFileExtension(".pgp");
	connect(lib, SIGNAL(systemMessage(QString, bool)), SLOT(busyMessage(QString, bool)));

	if (KGpgSettings::encryptFilesTo()) {
		if (KGpgSettings::pgpCompatibility())
			opts << QString::fromLocal8Bit("--pgp6");
		
		lib->slotFileEnc(droppedUrls, opts, m_model, goDefaultKey, KGpgSettings::fileEncryptionKey());
	} else {
		lib->slotFileEnc(droppedUrls, opts, m_model, goDefaultKey);
	}
}

void KGpgExternalActions::encryptDroppedFolder()
{
	compressionScheme = 0;
	kgpgfoldertmp = new KTemporaryFile();
	kgpgfoldertmp->open();

	if (KMessageBox::Cancel == KMessageBox::warningContinueCancel(0,
				i18n("<qt>KGpg will now create a temporary archive file:<br /><b>%1</b> to process the encryption. The file will be deleted after the encryption is finished.</qt>",
				kgpgfoldertmp->fileName()), i18n("Temporary File Creation"), KStandardGuiItem::cont(),
				KStandardGuiItem::cancel(), "FolderTmpFile"))
		return;

	dialog = new KgpgSelectPublicKeyDlg(0, m_model, goDefaultKey, droppedUrls.first().fileName());

	KHBox *bGroup = new KHBox(dialog->optionsbox);

	(void) new QLabel(i18n("Compression method for archive:"), bGroup);

	KComboBox *optionbx = new KComboBox(bGroup);
	optionbx->addItem(i18n("Zip"));
	optionbx->addItem(i18n("Gzip"));
	optionbx->addItem(i18n("Bzip2"));
	optionbx->addItem(i18n("Tar"));

	connect(optionbx, SIGNAL(activated (int)), SLOT(slotSetCompression(int)));
	connect(dialog, SIGNAL(okClicked()), SLOT(startFolderEncode()));
	connect(dialog, SIGNAL(cancelClicked()), SLOT(slotAbortEnc()));

	dialog->exec();
}

void KGpgExternalActions::slotAbortEnc()
{
	dialog = NULL;
}

void KGpgExternalActions::slotSetCompression(int cp)
{
	compressionScheme = cp;
}

void KGpgExternalActions::startFolderEncode()
{
	const QStringList selec(dialog->selectedKeys());
	QStringList encryptOptions(dialog->getCustomOptions().split(' ',  QString::SkipEmptyParts));
	bool symetric = dialog->getSymmetric();
	QString extension;

	switch (compressionScheme) {
	case 0:
		extension = ".zip";
		break;
	case 1:
		extension = ".tar.gz";
		break;
	case 2:
		extension = ".tar.bz2";
		break;
	case 3:
		extension = ".tar";
		break;
	default:
		Q_ASSERT(compressionScheme);
		return;
	}

	if (dialog->getArmor())
		extension += ".asc";
	else if (KGpgSettings::pgpExtension())
		extension += ".pgp";
	else
		extension += ".gpg";

	if (dialog->getArmor())
		encryptOptions << "--armor";
	if (dialog->getHideId())
		encryptOptions << "--throw-keyids";

	QString fname(droppedUrls.first().path());
	if (fname.endsWith('/'))
		fname.remove(fname.length() - 1, 1);

	KUrl encryptedFile(KUrl::fromPath(fname + extension));
	QFile encryptedFolder(encryptedFile.path());
	if (encryptedFolder.exists()) {
		dialog->hide();
		KIO::RenameDialog over(0, i18n("File Already Exists"), KUrl(), encryptedFile, KIO::M_OVERWRITE);
		if (over.exec() == QDialog::Rejected) {
			dialog = NULL;
			return;
		}
		encryptedFile = over.newDestUrl();
		dialog->show(); // strange, but if dialog is hidden, the passive popup is not displayed...
	}

	pop = new KPassivePopup();
	pop->setView(i18n("Processing folder compression and encryption"), i18n("Please wait..."), Images::kgpg());
	pop->setAutoDelete(false);
	pop->show();
	kapp->processEvents();
	dialog->accept();
	dialog = NULL;

	KArchive *arch = NULL;
	switch (compressionScheme) {
	case 0:
		arch = new KZip(kgpgfoldertmp->fileName());
		break;
	case 1:
		arch = new KTar(kgpgfoldertmp->fileName(), "application/x-gzip");
		break;
	case 2:
		arch = new KTar(kgpgfoldertmp->fileName(), "application/x-bzip");
		break;
	case 3:
		arch = new KTar(kgpgfoldertmp->fileName(), "application/x-tar");
		break;
	default:
		Q_ASSERT(1);
	}

	if (!arch->open(QIODevice::WriteOnly))
	{
		KMessageBox::sorry(0, i18n("Unable to create temporary file"));
		delete arch;
		return;
	}

	arch->addLocalDirectory(droppedUrls.first().path(), droppedUrls.first().fileName());
	arch->close();
	delete arch;

	KGpgTextInterface *folderprocess = new KGpgTextInterface();
	connect(folderprocess, SIGNAL(fileEncryptionFinished(KUrl, KGpgTextInterface*)), SLOT(slotFolderFinished(KUrl, KGpgTextInterface*)));
	connect(folderprocess, SIGNAL(errorMessage(const QString &, KGpgTextInterface*)), SLOT(slotFolderFinishedError(const QString &, KGpgTextInterface*)));
	folderprocess->encryptFile(selec, KUrl(kgpgfoldertmp->fileName()), encryptedFile, encryptOptions, symetric);
}

void KGpgExternalActions::slotFolderFinished(const KUrl &, KGpgTextInterface *iface)
{
	delete pop;
	delete kgpgfoldertmp;
	iface->deleteLater();
}

void KGpgExternalActions::slotFolderFinishedError(const QString &errmsge, KGpgTextInterface *iface)
{
	delete pop;
	delete kgpgfoldertmp;
	iface->deleteLater();
	KMessageBox::sorry(0, errmsge);
}

void KGpgExternalActions::busyMessage(const QString &mssge, bool reset)
{
	if (reset)
		openTasks = 0;

	if (!mssge.isEmpty()) {
		openTasks++;
#ifdef __GNUC__
#warning FIXME: this need to be ported
#endif
// 		trayIcon->setToolTip(mssge);
	} else {
		openTasks--;
	}

	if (openTasks <= 0)
		openTasks = 0;
}

void KGpgExternalActions::encryptFiles(KUrl::List urls)
{
	droppedUrls = urls;
	encryptDroppedFile();
}

void KGpgExternalActions::slotVerifyFile()
{
	// check file signature
	if (droppedUrl.isEmpty())
		return;

	QString sigfile;
	// try to find detached signature.
	if (!droppedUrl.fileName().endsWith(".sig")) {
		sigfile = droppedUrl.path() + ".sig";
		QFile fsig(sigfile);
		if (!fsig.exists()) {
			sigfile = droppedUrl.path() + ".asc";
			QFile fsig(sigfile);
			// if no .asc or .sig signature file included, assume the file is internally signed
			if (!fsig.exists())
				sigfile.clear();
		}
	} else {
		sigfile = droppedUrl.path();
		droppedUrl = KUrl(sigfile.left(sigfile.length() - 4));
	}

	// pipe gpg command
	KGpgTextInterface *verifyFileProcess = new KGpgTextInterface();
	connect (verifyFileProcess, SIGNAL(verifyquerykey(QString)), SLOT(importSignature(QString)));
	verifyFileProcess->KgpgVerifyFile(droppedUrl, KUrl(sigfile));
}

void KGpgExternalActions::importSignature(const QString &ID)
{
	KeyServer *kser = new KeyServer(0, false);
	kser->setItemModel(m_model);
	kser->slotSetText(ID);
	kser->slotImport();
}

void KGpgExternalActions::signDroppedFile()
{
	// create a detached signature for a chosen file
	if (droppedUrl.isEmpty())
		return;

	// select a private key to sign file --> listkeys.cpp
	KgpgSelectSecretKey *opts = new KgpgSelectSecretKey(0, m_model, false);
	if (opts->exec() != QDialog::Accepted) {
		delete opts;
		return;
	}

	const QString signKeyID(opts->getKeyID());
	delete opts;
	QStringList Options;
	if (KGpgSettings::asciiArmor())
		Options << "--armor";
	if (KGpgSettings::pgpCompatibility())
		Options << "--pgp6";
	Options << "--detach-sign";

	KGpgTextInterface *signFileProcess = new KGpgTextInterface();
	signFileProcess->signFilesBlocking(signKeyID, droppedUrls, Options);
	delete signFileProcess;
}

void KGpgExternalActions::decryptDroppedFile()
{
	m_decryptionFailed.clear();

	decryptFile(new KgpgLibrary(0));
}

void KGpgExternalActions::decryptFile(KgpgLibrary *lib)
{
	if (!droppedUrls.first().isLocalFile()) {
		showDroppedFile();
		decryptNextFile(lib, KUrl());
	}
	
	QString oldname(droppedUrls.first().fileName());
	if (oldname.endsWith(".gpg", Qt::CaseInsensitive) || oldname.endsWith(".asc", Qt::CaseInsensitive) || oldname.endsWith(".pgp", Qt::CaseInsensitive))
		oldname.chop(4);
	else
		oldname.append(".clear");

	KUrl swapname(droppedUrls.first().directory(KUrl::AppendTrailingSlash) + oldname);
	QFile fgpg(swapname.path());
	if (fgpg.exists()) {
		KIO::RenameDialog over(0, i18n("File Already Exists"), KUrl(), swapname, KIO::M_OVERWRITE);
		if (over.exec() == QDialog::Rejected) {
			decryptNextFile(lib, KUrl());
			return;
		}

		swapname=over.newDestUrl();
	}

	QStringList custdecr;
	if (!KGpgSettings::customDecrypt().isEmpty())
		custdecr.append(KGpgSettings::customDecrypt());
	connect(lib, SIGNAL(systemMessage(QString, bool)), SLOT(busyMessage(QString, bool)));
	connect(lib, SIGNAL(decryptionOver(KgpgLibrary *, KUrl)), SLOT(decryptNextFile(KgpgLibrary *, KUrl)));
	lib->slotFileDec(droppedUrls.first(), swapname, custdecr);
}

void KGpgExternalActions::decryptNextFile(KgpgLibrary *lib, const KUrl &failed)
{
	if (!failed.isEmpty())
		m_decryptionFailed << failed;

	if (droppedUrls.count() > 1) {
		droppedUrls.pop_front();
		decryptFile(lib);
	} else if ((droppedUrls.count() <= 1) && (m_decryptionFailed.count() > 0)) {
		lib->deleteLater();
		KMessageBox::errorList(NULL,
					i18np("Decryption of this file failed:", "Decryption of these files failed:",
					m_decryptionFailed.count()), m_decryptionFailed.toStringList(),
					i18n("Decryption failed."));
	} else {
		lib->deleteLater();
	}
}

void KGpgExternalActions::showDroppedFile()
{
	KgpgEditor *kgpgtxtedit = new KgpgEditor(0, m_model, 0, goDefaultKey);
	kgpgtxtedit->setAttribute(Qt::WA_DeleteOnClose);
	kgpgtxtedit->view->editor->slotDroppedFile(droppedUrls.first());

	connect(kgpgtxtedit, SIGNAL(encryptFiles(KUrl::List)), SLOT(encryptFiles(KUrl::List)));
	connect(m_keysmanager, SIGNAL(fontChanged(QFont)), kgpgtxtedit, SLOT(slotSetFont(QFont)));
	connect(kgpgtxtedit, SIGNAL(refreshImported(QStringList)), SIGNAL(importedKeys(QStringList)));
	kgpgtxtedit->show();
}

void KGpgExternalActions::readOptions()
{
	clipboardMode = QClipboard::Clipboard;
	if (KGpgSettings::useMouseSelection() && kapp->clipboard()->supportsSelection())
		clipboardMode = QClipboard::Selection;

	if (KGpgSettings::firstRun()) {
		firstRun();
	} else {
		QString path(KGpgSettings::gpgConfigPath());

		if (path.isEmpty()) {
			if (KMessageBox::Yes == KMessageBox::questionYesNo(0,
					i18n("<qt>You have not set a path to your GnuPG config file.<br />This may cause some surprising results in KGpg's execution.<br />Would you like to start KGpg's assistant to fix this problem?</qt>"),
					QString(), KGuiItem(i18n("Start Assistant")), KGuiItem(i18n("Do Not Start"))))
				startAssistant();
		} else {
			QStringList groups(KgpgInterface::getGpgGroupNames(path));
			if (!groups.isEmpty())
				KGpgSettings::setGroups(groups.join(QString(',')));
		}
	}
}

void KGpgExternalActions::firstRun()
{
	QProcess *createConfigProc = new QProcess(this);
	QStringList args;
	args << "--no-tty" << "--list-secret-keys";
	createConfigProc->start("gpg", args);	// start gnupg so that it will create a config file
	createConfigProc->waitForFinished();
	startAssistant();
}

void KGpgExternalActions::startAssistant()
{
	m_assistant = new KGpgFirstAssistant(m_keysmanager);

	connect(m_assistant, SIGNAL(accepted()), SLOT(slotSaveOptionsPath()));
	connect(m_assistant, SIGNAL(destroyed()), SLOT(slotAssistantClose()));
	connect(m_assistant, SIGNAL(helpClicked()), SLOT(help()));

	m_assistant->show();
}

void KGpgExternalActions::slotSaveOptionsPath()
{
	KGpgSettings::setAutoStart(m_assistant->getAutoStart());
	KGpgSettings::setGpgConfigPath(m_assistant->getConfigPath());
	KGpgSettings::setFirstRun(false);

	const QString gpgConfServer(KgpgInterface::getGpgSetting("keyserver", KGpgSettings::gpgConfigPath()));
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

void KGpgExternalActions::slotAssistantClose()
{
	m_assistant->deleteLater();
}

void KGpgExternalActions::help()
{
	KToolInvocation::invokeHelp(0, "kgpg");
}
