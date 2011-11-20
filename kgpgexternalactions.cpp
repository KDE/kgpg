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

#include <KActionCollection>
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
#include "kgpgtextedit.h"
#include "selectpublickeydialog.h"
#include "selectsecretkey.h"
#include "kgpginterface.h"

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

void KGpgExternalActions::encryptDroppedFile()
{
	QStringList opts;
	KgpgLibrary *lib = new KgpgLibrary(0);
	connect(lib, SIGNAL(systemMessage(QString)), SLOT(busyMessage(QString)));

	if (KGpgSettings::encryptFilesTo()) {
		if (KGpgSettings::pgpCompatibility())
			opts << QLatin1String( "--pgp6" );

		lib->slotFileEnc(droppedUrls, opts, m_model, goDefaultKey(), KGpgSettings::fileEncryptionKey());
	} else {
		lib->slotFileEnc(droppedUrls, opts, m_model, goDefaultKey());
	}
}

void KGpgExternalActions::encryptDroppedFolder()
{
	compressionScheme = 0;
	kgpgfoldertmp = new KTemporaryFile();
	kgpgfoldertmp->open();

	if (KMessageBox::Cancel == KMessageBox::warningContinueCancel(m_keysmanager,
				i18n("<qt>KGpg will now create a temporary archive file:<br /><b>%1</b> to process the encryption. The file will be deleted after the encryption is finished.</qt>",
				kgpgfoldertmp->fileName()), i18n("Temporary File Creation"), KStandardGuiItem::cont(),
				KStandardGuiItem::cancel(), QLatin1String( "FolderTmpFile" )))
		return;

	dialog = new KgpgSelectPublicKeyDlg(m_keysmanager, m_model, goDefaultKey(), false, droppedUrls);

	KHBox *bGroup = new KHBox(dialog->optionsbox);

	(void) new QLabel(i18n("Compression method for archive:"), bGroup);

	KComboBox *optionbx = new KComboBox(bGroup);
	optionbx->addItem(i18n("Zip"));
	optionbx->addItem(i18n("Gzip"));
	optionbx->addItem(i18n("Bzip2"));
	optionbx->addItem(i18n("Tar"));
	optionbx->addItem(i18n("Tar/XZ"));

	connect(optionbx, SIGNAL(activated (int)), SLOT(slotSetCompression(int)));
	connect(dialog, SIGNAL(okClicked()), SLOT(startFolderEncode()));
	connect(dialog, SIGNAL(cancelClicked()), SLOT(slotAbortEnc()));

	dialog->exec();
}

void KGpgExternalActions::slotAbortEnc()
{
	dialog->deleteLater();
	dialog = NULL;
}

void KGpgExternalActions::slotSetCompression(int cp)
{
	compressionScheme = cp;
}

void KGpgExternalActions::startFolderEncode()
{
	const QStringList selec(dialog->selectedKeys());
	QStringList encryptOptions(dialog->getCustomOptions().split(QLatin1Char( ' ' ),  QString::SkipEmptyParts));
	bool symetric = dialog->getSymmetric();
	QString extension;

	switch (compressionScheme) {
	case 0:
		extension = QLatin1String( ".zip" );
		break;
	case 1:
		extension = QLatin1String( ".tar.gz" );
		break;
	case 2:
		extension = QLatin1String( ".tar.bz2" );
		break;
	case 3:
		extension = QLatin1String( ".tar" );
		break;
	case 4:
		extension = QLatin1String( ".tar.xz" );
		break;
	default:
		Q_ASSERT(compressionScheme == 0);
		return;
	}

	if (dialog->getArmor())
		extension += QLatin1String( ".asc" );
	else if (KGpgSettings::pgpExtension())
		extension += QLatin1String( ".pgp" );
	else
		extension += QLatin1String( ".gpg" );

	if (dialog->getArmor())
		encryptOptions << QLatin1String( "--armor" );
	if (dialog->getHideId())
		encryptOptions << QLatin1String( "--throw-keyids" );

	QString fname(droppedUrls.first().path());
	if (fname.endsWith(QLatin1Char( '/' )))
		fname.remove(fname.length() - 1, 1);

	KUrl encryptedFile(KUrl::fromPath(fname + extension));
	QFile encryptedFolder(encryptedFile.path());
	if (encryptedFolder.exists()) {
		dialog->hide();
		QPointer<KIO::RenameDialog> over = new KIO::RenameDialog(m_keysmanager, i18n("File Already Exists"), KUrl(), encryptedFile, KIO::M_OVERWRITE);
		if (over->exec() == QDialog::Rejected) {
			dialog->deleteLater();
			dialog = NULL;
			delete over;
			return;
		}
		encryptedFile = over->newDestUrl();
		delete over;
		dialog->show(); // strange, but if dialog is hidden, the passive popup is not displayed...
	}

	pop = new KPassivePopup();
	pop->setView(i18n("Processing folder compression and encryption"), i18n("Please wait..."), Images::kgpg());
	pop->setAutoDelete(false);
	pop->show();
	kapp->processEvents();
	dialog->accept();
	dialog->deleteLater();
	dialog = NULL;

	KArchive *arch = NULL;
	switch (compressionScheme) {
	case 0:
		arch = new KZip(kgpgfoldertmp->fileName());
		break;
	case 1:
		arch = new KTar(kgpgfoldertmp->fileName(), QLatin1String( "application/x-gzip" ));
		break;
	case 2:
		arch = new KTar(kgpgfoldertmp->fileName(), QLatin1String( "application/x-bzip" ));
		break;
	case 3:
		arch = new KTar(kgpgfoldertmp->fileName(), QLatin1String( "application/x-tar" ));
		break;
	case 4:
		arch = new KTar(kgpgfoldertmp->fileName(), QLatin1String( "application/x-xz" ));
		break;
	default:
		Q_ASSERT(0);
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

	KGpgTextInterface *folderprocess = new KGpgTextInterface(this);
	connect(folderprocess, SIGNAL(fileEncryptionFinished(KUrl)), SLOT(slotFolderFinished(KUrl)));
	connect(folderprocess, SIGNAL(errorMessage(const QString &)), SLOT(slotFolderFinishedError(const QString &)));
	folderprocess->encryptFile(selec, KUrl(kgpgfoldertmp->fileName()), encryptedFile, encryptOptions, symetric);
}

void KGpgExternalActions::slotFolderFinished(const KUrl &)
{
	delete pop;
	delete kgpgfoldertmp;
	sender()->deleteLater();
}

void KGpgExternalActions::slotFolderFinishedError(const QString &errmsge)
{
	delete pop;
	delete kgpgfoldertmp;
	sender()->deleteLater();
	KMessageBox::sorry(0, errmsge);
}

void KGpgExternalActions::busyMessage(const QString &mssge)
{
	if (!mssge.isEmpty()) {
#ifdef __GNUC__
#warning FIXME: this need to be ported
#endif
// 		trayIcon->setToolTip(mssge);
	}
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
	if (!droppedUrl.fileName().endsWith(QLatin1String(".sig"))) {
		sigfile = droppedUrl.path() + QLatin1String( ".sig" );
		QFile fsig(sigfile);
		if (!fsig.exists()) {
			sigfile = droppedUrl.path() + QLatin1String( ".asc" );
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
	KGpgTextInterface *verifyFileProcess = new KGpgTextInterface(this);
	connect (verifyFileProcess, SIGNAL(verifyquerykey(QString)), SLOT(importSignature(QString)));
	verifyFileProcess->KgpgVerifyFile(droppedUrl, KUrl(sigfile));
}

void KGpgExternalActions::importSignature(const QString &ID)
{
	KeyServer *kser = new KeyServer(0, m_model);
	kser->slotSetText(ID);
	kser->slotImport();
}

void KGpgExternalActions::signDroppedFile()
{
	// create a detached signature for a chosen file
	if (droppedUrl.isEmpty())
		return;

	// select a private key to sign file --> listkeys.cpp
	QPointer<KgpgSelectSecretKey> opts = new KgpgSelectSecretKey(0, m_model, false);
	if (opts->exec() != QDialog::Accepted) {
		delete opts;
		return;
	}

	const QString signKeyID(opts->getKeyID());
	delete opts;
	QStringList Options;
	if (KGpgSettings::asciiArmor())
		Options << QLatin1String( "--armor" );
	if (KGpgSettings::pgpCompatibility())
		Options << QLatin1String( "--pgp6" );
	Options << QLatin1String( "--detach-sign" );

	KGpgTextInterface signFileProcess;
	signFileProcess.signFilesBlocking(signKeyID, droppedUrls, Options);
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
		decryptNextFile(KUrl(), lib);
	}

	QString oldname(droppedUrls.first().fileName());
	if (oldname.endsWith(QLatin1String(".gpg"), Qt::CaseInsensitive) ||
			oldname.endsWith(QLatin1String(".asc"), Qt::CaseInsensitive) ||
			oldname.endsWith(QLatin1String(".pgp"), Qt::CaseInsensitive))
		oldname.chop(4);
	else
		oldname.append(QLatin1String( ".clear" ));

	KUrl swapname(droppedUrls.first().directory(KUrl::AppendTrailingSlash) + oldname);
	QFile fgpg(swapname.path());
	if (fgpg.exists()) {
		QPointer<KIO::RenameDialog> over = new KIO::RenameDialog(m_keysmanager, i18n("File Already Exists"), KUrl(), swapname, KIO::M_OVERWRITE);
		if (over->exec() != QDialog::Accepted) {
			delete over;
			decryptNextFile(KUrl(), lib);
			return;
		}

		swapname = over->newDestUrl();
		delete over;
	}

	connect(lib, SIGNAL(systemMessage(QString)), SLOT(busyMessage(QString)));
	connect(lib, SIGNAL(decryptionOver(KUrl)), SLOT(decryptNextFile(KUrl)));
	lib->slotFileDec(droppedUrls.first(), swapname);
}

void KGpgExternalActions::decryptNextFile(const KUrl &failed, KgpgLibrary *lib)
{
	if (lib == NULL)
		lib = qobject_cast<KgpgLibrary *>(sender());

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
	KgpgEditor *kgpgtxtedit = new KgpgEditor(m_keysmanager, m_model, 0);
	connect(kgpgtxtedit, SIGNAL(encryptFiles(KUrl::List)), SLOT(encryptFiles(KUrl::List)));
	connect(m_keysmanager, SIGNAL(fontChanged(QFont)), kgpgtxtedit, SLOT(slotSetFont(QFont)));

	kgpgtxtedit->m_editor->openDroppedFile(droppedUrls.first(), false);

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
				KGpgSettings::setGroups(groups.join(QLatin1String( "," )));
		}
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
