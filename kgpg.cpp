/***************************************************************************
                          kgpg.cpp  -  description
                             -------------------
    begin                : Mon Nov 18 2002
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

#include "kgpg.h"


#include <QApplication>
#include <QDragEnterEvent>
#include <QDesktopWidget>
#include <QTextStream>
#include <QDropEvent>
#include <QClipboard>
#include <QCheckBox>
#include <QWidget>
#include <QRegExp>
#include <QCursor>
#include <QLabel>
#include <QFile>
#include <Q3TextDrag>
#include <QtDBus/QtDBus>
#include <QProcess>

#include <KAboutApplicationDialog>
#include <KToolInvocation>
#include <kio/renamedialog.h>
#include <KPassivePopup>
#include <KCmdLineArgs>
#include <KMessageBox>
#include <KShortcut>
#include <KComboBox>
#include <KTemporaryFile>
#include <KGlobal>
#include <KLocale>
#include <KAction>
#include <KDebug>
#include <KMenu>
#include <KWindowSystem>
#include <ktip.h>
#include <KTar>
#include <KZip>
#include <KActionCollection>
#include <KStandardAction>
#include <KIcon>
#include <kdefakes.h>
#include <KHBox>
#include <KFileDialog>
#include <KMimeType>

#include "images.h"
#include "selectsecretkey.h"
#include "kgpgeditor.h"
#include "kgpgsettings.h"   // automatically created by compilation
#include "keysmanager.h"
#include "keyservers.h"
#include "selectpublickeydialog.h"
#include "kgpgview.h"
#include "kgpglibrary.h"
#include "kgpg_interface.h"
#include "kgpgtextinterface.h"
#include "kgpgfirstassistant.h"

using namespace KgpgCore;

KGpgUserActions::KGpgUserActions(QWidget *parent, KSystemTrayIcon *parentTrayIcon, KGpgItemModel *model)
      : QObject(parent)
{
    trayIcon = parentTrayIcon;
    m_parentWidget = parent;

    openTasks = 0;

    saveDecrypt = new KAction(KIcon(QString("document-decrypt")), i18n("&Decrypt && Save File"), this);
    saveDecrypt->setObjectName("decrypt_file");
    connect(saveDecrypt, SIGNAL(triggered(bool)), SLOT(decryptDroppedFile()));
    showDecrypt = new KAction(KIcon(QString("document-preview")), i18n("&Show Decrypted File"), this);
    showDecrypt->setObjectName("show_file");
    connect(showDecrypt, SIGNAL(triggered(bool)), SLOT(showDroppedFile()));
    encrypt = new KAction(KIcon(QString("document-encrypt")), i18n("&Encrypt File"), this);
    encrypt->setObjectName("encrypt_file");
    connect(encrypt, SIGNAL(triggered(bool)), SLOT(encryptDroppedFile()));
    sign = new KAction(KIcon(QString("document-sign")), i18n("&Sign File"), this);
    sign->setObjectName("sign_file");
    connect(sign, SIGNAL(triggered(bool)), SLOT(signDroppedFile()));

    readOptions();

    droppopup = new KMenu();
    droppopup->addAction( showDecrypt );
    droppopup->addAction( saveDecrypt );

    udroppopup = new KMenu();
    udroppopup->addAction( encrypt );
    udroppopup->addAction( sign );

    trayIcon->setToolTip( i18n("KGpg - encryption tool"));

    m_model = model;
}

KGpgUserActions::~KGpgUserActions()
{
    delete droppopup;
    delete udroppopup;
    delete saveDecrypt;
    delete showDecrypt;
    delete encrypt;
    delete sign;
}

void KGpgUserActions::clipEncrypt()
{
    if (kapp->clipboard()->text(clipboardMode).isEmpty())
        KPassivePopup::message(i18n("Clipboard is empty."), QString(), Images::kgpg(), trayIcon);
    else
    {
        KgpgSelectPublicKeyDlg *dialog = new KgpgSelectPublicKeyDlg(0, m_model, goDefaultKey);
        if (dialog->exec() == KDialog::Accepted)
        {
            QStringList options;
            if (dialog->getUntrusted()) options << "--always-trust";
            if (dialog->getArmor())     options << "--armor";
            if (dialog->getHideId())    options << "--throw-keyid";

            if (!dialog->getCustomOptions().isEmpty())
                if (KGpgSettings::allowCustomEncryptionOptions())
                    options << dialog->getCustomOptions().split(' ', QString::SkipEmptyParts);

            encryptClipboard(dialog->selectedKeys(), options, dialog->getSymmetric());
        }

        delete dialog;
    }
}

void KGpgUserActions::clipDecrypt()
{
    QString clippie = kapp->clipboard()->text(clipboardMode).simplified();
    droppedtext(clippie, false);
}

void KGpgUserActions::clipSign(bool openEditor)
{
    QString clippie = kapp->clipboard()->text(clipboardMode).simplified();
    if (!clippie.isEmpty())
    {
        KgpgEditor *kgpgtxtedit = new KgpgEditor(0, m_model, Qt::WDestructiveClose, goDefaultKey);
        connect(this,SIGNAL(setFont(QFont)), kgpgtxtedit, SLOT(slotSetFont(QFont)));
        connect(kgpgtxtedit, SIGNAL(encryptFiles(KUrl::List)), this, SLOT(encryptFiles(KUrl::List)));

        if (!openEditor)
            connect(kgpgtxtedit->view, SIGNAL(verifyFinished()), kgpgtxtedit, SLOT(closeWindow()));

        kgpgtxtedit->view->editor->setPlainText(clippie);
        kgpgtxtedit->view->slotSignVerify();
        kgpgtxtedit->show();
    }
    else
        KPassivePopup::message(i18n("Clipboard is empty."), QString(), Images::kgpg(), trayIcon);
}

void KGpgUserActions::encryptDroppedFile()
{
    QStringList opts;
    KgpgLibrary *lib = new KgpgLibrary(0);
    if (KGpgSettings::pgpExtension())
        lib->setFileExtension(".pgp");
    connect(lib, SIGNAL(systemMessage(QString, bool)), this, SLOT(busyMessage(QString, bool)));

    if (KGpgSettings::encryptFilesTo())
    {
        if (KGpgSettings::allowUntrustedKeys())
            opts << QString::fromLocal8Bit("--always-trust");
        if (KGpgSettings::asciiArmor())
            opts << QString::fromLocal8Bit("--armor");
        if (KGpgSettings::hideUserID())
            opts << QString::fromLocal8Bit("--throw-keyid");
        if (KGpgSettings::pgpCompatibility())
            opts << QString::fromLocal8Bit("--pgp6");

        lib->slotFileEnc(droppedUrls, opts, m_model, goDefaultKey, KGpgSettings::fileEncryptionKey());
    }
    else
        lib->slotFileEnc(droppedUrls, opts, m_model, goDefaultKey);
}

void KGpgUserActions::encryptDroppedFolder()
{
    compressionScheme = 0;
    kgpgfoldertmp = new KTemporaryFile();
    kgpgfoldertmp->open();

    if (KMessageBox::warningContinueCancel(0, i18n("<qt>KGpg will now create a temporary archive file:<br /><b>%1</b> to process the encryption. The file will be deleted after the encryption is finished.</qt>", kgpgfoldertmp->fileName()), i18n("Temporary File Creation"), KStandardGuiItem::cont(), KStandardGuiItem::cancel(), "FolderTmpFile") == KMessageBox::Cancel)
        return;

    dialog = new KgpgSelectPublicKeyDlg(0, m_model, goDefaultKey, droppedUrls.first().fileName());

    KHBox *bGroup = new KHBox(dialog->optionsbox);

    (void) new QLabel(i18n("Compression method for archive:"),bGroup);

    KComboBox *optionbx = new KComboBox(bGroup);
    optionbx->addItem(i18n("Zip"));
    optionbx->addItem(i18n("Gzip"));
    optionbx->addItem(i18n("Bzip2"));
    optionbx->addItem(i18n("Tar"));

    connect(optionbx,SIGNAL(activated (int)),this,SLOT(slotSetCompression(int)));
    connect(dialog, SIGNAL(okClicked()), this, SLOT(startFolderEncode()));
    connect(dialog, SIGNAL(cancelClicked()), this, SLOT(slotAbortEnc()));

    dialog->exec();
}

void KGpgUserActions::slotAbortEnc()
{
    dialog = NULL;
}

void KGpgUserActions::slotSetCompression(int cp)
{
    compressionScheme = cp;
}

void KGpgUserActions::startFolderEncode()
{
    QStringList selec = dialog->selectedKeys();
    QStringList encryptOptions = dialog->getCustomOptions().split(' ',  QString::SkipEmptyParts);
    bool symetric = dialog->getSymmetric();
    QString extension;

    switch (compressionScheme) {
    case 0:	extension = ".zip"; break;
    case 1:	extension = ".tar.gz"; break;
    case 2:	extension = ".tar.bz2"; break;
    case 3:	extension = ".tar"; break;
    default:	Q_ASSERT(compressionScheme);
		return;
    }

    if (dialog->getArmor())
        extension += ".asc";
    else
    if (KGpgSettings::pgpExtension())
        extension += ".pgp";
    else
        extension += ".gpg";

    if (dialog->getArmor())
	encryptOptions << "--armor";
    if (dialog->getHideId())
	encryptOptions << "--throw-keyids";

    QString fname = droppedUrls.first().path();
    if (fname.endsWith('/'))
	fname.remove(fname.length() - 1, 1);
    KUrl encryptedFile = KUrl::fromPath(fname + extension);
    QFile encryptedFolder(encryptedFile.path());
    if (encryptedFolder.exists())
    {
        dialog->hide();
        KIO::RenameDialog over(0, i18n("File Already Exists"), KUrl(), encryptedFile, KIO::M_OVERWRITE);
        if (over.exec() == QDialog::Rejected)
        {
	    dialog = NULL;
            return;
        }
        encryptedFile = over.newDestUrl();
        dialog->show(); // strange, but if dialog is hidden, the passive popup is not displayed...
    }

    pop = new KPassivePopup();
    pop->setView(i18n("Processing folder compression and encryption"),i18n("Please wait..."), Images::kgpg());
    pop->setAutoDelete(false);
    pop->show();
    kapp->processEvents();
    dialog->accept();
    dialog = 0L;

    KArchive *arch;
    switch (compressionScheme) {
    case 0:	arch = new KZip(kgpgfoldertmp->fileName()); break;
    case 1:	arch = new KTar(kgpgfoldertmp->fileName(), "application/x-gzip"); break;
    case 2:	arch = new KTar(kgpgfoldertmp->fileName(), "application/x-bzip"); break;
    case 3:	arch = new KTar(kgpgfoldertmp->fileName(), "application/x-tar"); break;
    default:	Q_ASSERT(1);
    }

    if (!arch->open(QIODevice::WriteOnly))
    {
        KMessageBox::sorry(0,i18n("Unable to create temporary file"));
	delete arch;
        return;
    }

    arch->addLocalDirectory(droppedUrls.first().path(), droppedUrls.first().fileName());
    arch->close();
    delete arch;

    KGpgTextInterface *folderprocess = new KGpgTextInterface();
    connect(folderprocess, SIGNAL(fileEncryptionFinished(KUrl, KGpgTextInterface*)), this, SLOT(slotFolderFinished(KUrl, KGpgTextInterface*)));
    connect(folderprocess, SIGNAL(errorMessage(const QString &, KGpgTextInterface*)), this, SLOT(slotFolderFinishedError(const QString &, KGpgTextInterface*)));
    folderprocess->encryptFile(selec, KUrl(kgpgfoldertmp->fileName()), encryptedFile, encryptOptions, symetric);
}

void KGpgUserActions::slotFolderFinished(const KUrl &, KGpgTextInterface *iface)
{
    delete pop;
    delete kgpgfoldertmp;
    iface->deleteLater();
}

void KGpgUserActions::slotFolderFinishedError(const QString &errmsge, KGpgTextInterface *iface)
{
    delete pop;
    delete kgpgfoldertmp;
    iface->deleteLater();
    KMessageBox::sorry(0, errmsge);
}

void KGpgUserActions::busyMessage(const QString &mssge, bool reset)
{
    if (reset)
        openTasks = 0;

    if (!mssge.isEmpty())
    {
        openTasks++;
        trayIcon->setToolTip(mssge);
    }
    else
        openTasks--;

    if (openTasks <= 0)
    {
        trayIcon->setIcon(KIcon("kgpg"));
        trayIcon->setToolTip(i18n("KGpg - encryption tool"));
        openTasks = 0;
    }
}

void KGpgUserActions::encryptFiles(KUrl::List urls)
{
    droppedUrls = urls;
    encryptDroppedFile();
}

void KGpgUserActions::slotVerifyFile()
{
    // check file signature
    if (droppedUrl.isEmpty())
        return;

    QString sigfile;
    // try to find detached signature.
    if (!droppedUrl.fileName().endsWith(".sig"))
    {
        sigfile = droppedUrl.path() + ".sig";
        QFile fsig(sigfile);
        if (!fsig.exists())
        {
            sigfile = droppedUrl.path() + ".asc";
            QFile fsig(sigfile);
            // if no .asc or .sig signature file included, assume the file is internally signed
            if (!fsig.exists())
                sigfile.clear();
        }
    }
    else
    {
        sigfile = droppedUrl.path();
        droppedUrl = KUrl(sigfile.left(sigfile.length() - 4));
    }

    // pipe gpg command
    KGpgTextInterface *verifyFileProcess = new KGpgTextInterface();
    connect (verifyFileProcess, SIGNAL(verifyquerykey(QString)), this, SLOT(importSignature(QString)));
    verifyFileProcess->KgpgVerifyFile(droppedUrl, KUrl(sigfile));
}

void KGpgUserActions::importSignature(const QString &ID)
{
    KeyServer *kser = new KeyServer(0, false);
    kser->slotSetText(ID);
    kser->slotImport();
}

void KGpgUserActions::signDroppedFile()
{
    // create a detached signature for a chosen file
    if (droppedUrl.isEmpty())
        return;

    QString signKeyID;
    // select a private key to sign file --> listkeys.cpp
    KgpgSelectSecretKey *opts = new KgpgSelectSecretKey(0, m_model, false);
    if (opts->exec() == QDialog::Accepted)
        signKeyID = opts->getKeyID();
    else
    {
        delete opts;
        return;
    }

    delete opts;
    QStringList Options;
    if (KGpgSettings::asciiArmor())
        Options << "--armor";
    if (KGpgSettings::pgpCompatibility())
        Options << "--pgp6";

    KGpgTextInterface *signFileProcess = new KGpgTextInterface();
    signFileProcess->signFilesBlocking(signKeyID, droppedUrls, Options);
    delete signFileProcess;
}

void KGpgUserActions::decryptDroppedFile()
{
	m_decryptionFailed.clear();

	decryptFile(new KgpgLibrary(0));
}

void KGpgUserActions::decryptFile(KgpgLibrary *lib)
{
	//bool isFolder=false;  // droppedUrls
	KUrl swapname;

	if (!droppedUrls.first().isLocalFile()) {
		showDroppedFile();
		decryptNextFile(lib, KUrl());
	}

	QString oldname = droppedUrls.first().fileName();
	if (oldname.toLower().endsWith(".gpg") || oldname.toLower().endsWith(".asc") || oldname.toLower().endsWith(".pgp"))
		oldname.truncate(oldname.length() - 4);
	else
		oldname.append(".clear");
    /*
        if (oldname.endsWith(".tar.gz")) {
                isFolder=true;
                kgpgFolderExtract=new KTemporaryFile();
                kgpgFolderExtract->setSuffix(".tar.gz");
                kgpgFolderExtract->open();
                swapname=KUrl(kgpgFolderExtract->fileName());
                if (KMessageBox::warningContinueCancel(0,i18n("<qt>The file to decrypt is an archive. KGpg will create a temporary unencrypted archive file:<br><b>%1</b> before processing the archive extraction. This temporary file will be deleted after the decryption is finished.</qt>").arg(kgpgFolderExtract->fileName()),i18n("Temporary File Creation"),KStandardGuiItem::cont(),"FolderTmpDecFile")==KMessageBox::Cancel)
                        return;
        } else*/
        {
            swapname = KUrl(droppedUrls.first().directory(KUrl::AppendTrailingSlash) + oldname);
            QFile fgpg(swapname.path());
            if (fgpg.exists())
            {
                KIO::RenameDialog over(0,i18n("File Already Exists"),KUrl(),swapname,KIO::M_OVERWRITE);
                if (over.exec()==QDialog::Rejected)
                {
			decryptNextFile(lib, KUrl());
                    return;
                }

                swapname=over.newDestUrl();
            }
        }

	QStringList custdecr;
	if (!KGpgSettings::customDecrypt().isEmpty())
		custdecr = QStringList(KGpgSettings::customDecrypt());
	lib->slotFileDec(droppedUrls.first(), swapname, custdecr);
	connect(lib, SIGNAL(importOver(KgpgLibrary *, QStringList)), SLOT(slotImportedKeys(KgpgLibrary *, QStringList)));
	connect(lib, SIGNAL(systemMessage(QString, bool)), this, SLOT(busyMessage(QString, bool)));
	connect(lib, SIGNAL(decryptionOver(KgpgLibrary *, KUrl)), this, SLOT(decryptNextFile(KgpgLibrary *, KUrl)));
}

void KGpgUserActions::decryptNextFile(KgpgLibrary *lib, const KUrl &failed)
{
	if (!failed.isEmpty())
		m_decryptionFailed << failed;

	if (droppedUrls.count() > 1) {
		droppedUrls.pop_front();
		decryptFile(lib);
	} else if ((droppedUrls.count() <= 1) && (m_decryptionFailed.count() > 0)) {
		lib->deleteLater();
		KMessageBox::errorList(NULL,
				i18np("Decryption of this file failed:", "Decryption of these files failed:", m_decryptionFailed.count()),
				m_decryptionFailed.toStringList(), i18n("Decryption failed."));
	} else {
		lib->deleteLater();
	}
}

void KGpgUserActions::showDroppedFile()
{
    KgpgEditor *kgpgtxtedit = new KgpgEditor(0, m_model, Qt::WDestructiveClose, goDefaultKey);
    kgpgtxtedit->view->editor->slotDroppedFile(droppedUrls.first());

    connect(kgpgtxtedit, SIGNAL(encryptFiles(KUrl::List)), this, SLOT(encryptFiles(KUrl::List)));
    connect(this, SIGNAL(setFont(QFont)), kgpgtxtedit, SLOT(slotSetFont(QFont)));
    connect(kgpgtxtedit, SIGNAL(refreshImported(QStringList)), this, SIGNAL(importedKeys(QStringList)));
    kgpgtxtedit->show();
}

void KGpgUserActions::droppedfile (KUrl::List url)
{
    droppedUrls = url;
    droppedUrl = url.first();
    if (KMimeType::findByUrl(droppedUrl)->name() == "inode/directory")
    {
        encryptDroppedFolder();
        //KMessageBox::sorry(0,i18n("Sorry, only file operations are currently supported."));
        return;
    }

    if (!droppedUrl.isLocalFile())
    {
        showDroppedFile();
        return;
    }

    if ((droppedUrl.path().endsWith(".asc")) || (droppedUrl.path().endsWith(".pgp")) || (droppedUrl.path().endsWith(".gpg")))
    {
        switch (KGpgSettings::encryptedDropEvent())
        {
            case KGpgSettings::EnumEncryptedDropEvent::DecryptAndSave:
                decryptDroppedFile();
                break;
            case KGpgSettings::EnumEncryptedDropEvent::DecryptAndOpen:
                showDroppedFile();
                break;
            case KGpgSettings::EnumEncryptedDropEvent::Ask:
                droppopup->exec(QCursor::pos ());
                kDebug(2100)<<"Drop menu--------";
                break;
        }
    }
    else
    if (droppedUrl.path().endsWith(".sig"))
        slotVerifyFile();
    else
        switch (KGpgSettings::unencryptedDropEvent())
        {
            case KGpgSettings::EnumUnencryptedDropEvent::Encrypt:
                encryptDroppedFile();
                break;
            case KGpgSettings::EnumUnencryptedDropEvent::Sign:
                signDroppedFile();
                break;
            case KGpgSettings::EnumUnencryptedDropEvent::Ask:
                udroppopup->exec(QCursor::pos ());
                break;
        }
}

void KGpgUserActions::droppedtext (const QString &inputText, bool allowEncrypt)
{
    if (inputText.startsWith("-----BEGIN PGP MESSAGE"))
    {
        KgpgEditor *kgpgtxtedit = new KgpgEditor(0, m_model, Qt::WDestructiveClose, goDefaultKey);
        connect(kgpgtxtedit, SIGNAL(encryptFiles(KUrl::List)), this, SLOT(encryptFiles(KUrl::List)));
        connect(this, SIGNAL(setFont(QFont)), kgpgtxtedit, SLOT(slotSetFont(QFont)));
        kgpgtxtedit->view->editor->setPlainText(inputText);
        kgpgtxtedit->view->slotDecode();
        kgpgtxtedit->show();
        return;
    }

    if (inputText.startsWith("-----BEGIN PGP PUBLIC KEY"))
    {
        int result = KMessageBox::warningContinueCancel(0,i18n("<p>The dropped text is a public key.<br />Do you want to import it ?</p>"));
        if (result == KMessageBox::Cancel)
            return;
        else
        {
            KgpgInterface *importKeyProcess = new KgpgInterface();
            importKeyProcess->importKey(inputText);
            connect(importKeyProcess, SIGNAL(importKeyFinished(KgpgInterface *, QStringList)), SLOT(slotImportedKeys(KgpgInterface *, QStringList)));
            return;
        }
    }

    if (inputText.startsWith("-----BEGIN PGP SIGNED MESSAGE"))
    {
        clipSign(false);
        return;
    }

    if (allowEncrypt)
        clipEncrypt();
    else
        KMessageBox::sorry(0, i18n("No encrypted text found."));
}

void KGpgUserActions::slotImportedKeys(KgpgInterface *iface, const QStringList &)
{
    iface->deleteLater();
}

void KGpgUserActions::slotImportedKeys(KgpgLibrary *lib, const QStringList &)
{
    lib->deleteLater();
}

void KGpgUserActions::dragEnterEvent(QDragEnterEvent *e)
{
    e->setAccepted(KUrl::List::canDecode(e->mimeData()) || Q3TextDrag::canDecode(e));
}

void KGpgUserActions::dropEvent(QDropEvent *o)
{
    QString text;
    KUrl::List uriList = KUrl::List::fromMimeData(o->mimeData());
    if (!uriList.isEmpty())
        droppedfile(uriList);
    else
    if (Q3TextDrag::decode(o, text))
    {
        QApplication::clipboard()->setText(text,clipboardMode);
        droppedtext(text);
    }
}

void KGpgUserActions::readOptions()
{
    clipboardMode = QClipboard::Clipboard;
    if (KGpgSettings::useMouseSelection() && kapp->clipboard()->supportsSelection())
        clipboardMode = QClipboard::Selection;

    if (KGpgSettings::firstRun())
        firstRun();
    else
    {
        QString path = KGpgSettings::gpgConfigPath();
        if (path.isEmpty())
        {
            if (KMessageBox::questionYesNo(0,i18n("<qt>You have not set a path to your GnuPG config file.<br />This may cause some surprising results in KGpg's execution.<br />Would you like to start KGpg's assistant to fix this problem?</qt>"), QString(), KGuiItem(i18n("Start Assistant")), KGuiItem(i18n("Do Not Start"))) == KMessageBox::Yes)
                startAssistant();
        }
        else
        {
            QStringList groups=KgpgInterface::getGpgGroupNames(path);
            if (!groups.isEmpty())
                    KGpgSettings::setGroups(groups.join(","));
        }

        trayIcon->setVisible(KGpgSettings::showSystray());
    }
}

void KGpgUserActions::firstRun()
{
    QProcess *createConfigProc = new QProcess(this);
    QStringList args;
    args << "--no-tty" << "--list-secret-keys";
    createConfigProc->start("gpg", args);// start gnupg so that it will create a config file
    createConfigProc->waitForFinished();
    startAssistant();
}

void KGpgUserActions::startAssistant()
{
	m_assistant = new KGpgFirstAssistant(m_parentWidget);

	connect(m_assistant, SIGNAL(accepted()), SLOT(slotSaveOptionsPath()));
	connect(m_assistant, SIGNAL(destroyed()), SLOT(slotAssistantClose()));
	connect(m_assistant, SIGNAL(helpClicked()), SLOT(help()));

	m_assistant->show();
}

void KGpgUserActions::slotSaveOptionsPath()
{
	KGpgSettings::setAutoStart(m_assistant->getAutoStart());
	KGpgSettings::setGpgConfigPath(m_assistant->getConfigPath());
	KGpgSettings::setFirstRun(false);

	QString defaultID = m_assistant->getDefaultKey();

	KGpgSettings::self()->writeConfig();
	emit updateDefault(defaultID);
	if (m_assistant->runKeyGenerate())
		emit createNewKey();
	m_assistant->deleteLater();
}

void KGpgUserActions::slotAssistantClose()
{
	m_assistant->deleteLater();
}

void KGpgUserActions::about()
{
    KAboutApplicationDialog dialog(KGlobal::mainComponent().aboutData());
    dialog.exec();
}

void KGpgUserActions::help()
{
    KToolInvocation::invokeHelp(0, "kgpg");
}

void KGpgUserActions::encryptClipboard(QStringList selec, QStringList encryptOptions, const bool symmetric)
{
    if (kapp->clipboard()->text(clipboardMode).isEmpty())
    {
        KPassivePopup::message(i18n("Clipboard is empty."), QString(), Images::kgpg(), trayIcon);
        return;
    }

    if (KGpgSettings::pgpCompatibility())
        encryptOptions << "--pgp6";

    encryptOptions << "--armor";

    if (symmetric)
        selec.clear();

    KGpgTextInterface *txtEncrypt = new KGpgTextInterface();
    connect (txtEncrypt, SIGNAL(txtEncryptionFinished(QString)), this, SLOT(slotSetClip(QString)));
    connect (txtEncrypt, SIGNAL(txtEncryptionStarted()), this, SLOT(slotPassiveClip()));
    txtEncrypt->encryptText(kapp->clipboard()->text(clipboardMode), selec, encryptOptions);
}

void KGpgUserActions::slotPassiveClip()
{
    QString newtxt = kapp->clipboard()->text(clipboardMode);
    if (newtxt.length() > 300)
        newtxt = QString(newtxt.left(250).simplified()) + "...\n" + QString(newtxt.right(40).simplified());

    newtxt.replace(QRegExp("<"), "&lt;"); // disable html tags
    newtxt.replace(QRegExp("\n"), "<br>");

    pop = new KPassivePopup();
    pop->setView(i18n("Encrypted following text:"), newtxt, Images::kgpg());
    pop->setTimeout(3200);
    pop->show();
    QRect qRect(QApplication::desktop()->screenGeometry());
    int iXpos = qRect.width() / 2 - pop->width() / 2;
    int iYpos = qRect.height() / 2 - pop->height() / 2;
    pop->move(iXpos, iYpos);
}

void KGpgUserActions::slotSetClip(const QString &newtxt)
{
    if (newtxt.isEmpty())
        return;

    kapp->clipboard()->setText(newtxt, clipboardMode);
}


kgpgapplet::kgpgapplet(QWidget *parent, KGpgItemModel *model)
          : KSystemTrayIcon(parent)
{
    w = new KGpgUserActions(parent, this, model);

    QMenu *conf_menu = contextMenu();

    QAction *KgpgOpenEditor = actionCollection()->addAction("kgpg_editor");
    KgpgOpenEditor->setIcon(KIcon("accessories-text-editor"));
    KgpgOpenEditor->setText(i18n("E&ditor"));
    connect(KgpgOpenEditor, SIGNAL(triggered(bool)), parent, SLOT(slotOpenEditor()));
    QAction *KgpgOpenManager = actionCollection()->addAction("kgpg_manager");
    KgpgOpenManager->setIcon(KIcon("kgpg"));
    KgpgOpenManager->setText(i18n("Ke&y Manager"));
    connect(KgpgOpenManager, SIGNAL(triggered(bool)), SLOT(slotOpenKeyManager()));

    QAction *KgpgEncryptClipboard = actionCollection()->addAction("clip_encrypt");
    KgpgEncryptClipboard->setText(i18n("&Encrypt Clipboard"));
    connect(KgpgEncryptClipboard, SIGNAL(triggered(bool)), w, SLOT(clipEncrypt()));
    QAction *KgpgDecryptClipboard = actionCollection()->addAction("clip_decrypt");
    KgpgDecryptClipboard->setText(i18n("&Decrypt Clipboard"));
    connect(KgpgDecryptClipboard, SIGNAL(triggered(bool)), w, SLOT(clipDecrypt()));
    QAction *KgpgSignClipboard = actionCollection()->addAction("clip_sign");
    KgpgSignClipboard->setText(i18n("&Sign/Verify Clipboard"));
    connect(KgpgSignClipboard, SIGNAL(triggered(bool)), w, SLOT(clipSign()));

    QAction *KgpgOpenServer = actionCollection()->addAction("kgpg_server");
    KgpgOpenServer->setIcon(KIcon("network-server"));
    KgpgOpenServer->setText(i18n("&Key Server Dialog"));
    connect(KgpgOpenServer, SIGNAL(triggered(bool)), SLOT(slotOpenServerDialog()));
    QAction *KgpgPreferences = KStandardAction::preferences(this, SLOT(showOptions()), actionCollection());

    conf_menu->addAction( KgpgEncryptClipboard );
    conf_menu->addAction( KgpgDecryptClipboard );
    conf_menu->addAction( KgpgSignClipboard );
    conf_menu->addAction( KgpgOpenManager );
    conf_menu->addAction( KgpgOpenEditor );
    conf_menu->addAction( KgpgOpenServer );
    conf_menu->addSeparator();
    conf_menu->addAction( KgpgPreferences );
    setIcon( KIcon("kgpg") );
}

void kgpgapplet::showOptions()
{
    OrgKdeKgpgKeyInterface kgpg("org.kde.kgpg", "/KeyInterface",QDBusConnection::sessionBus());
    QDBusReply<void> reply =kgpg.showOptions();
    if (!reply.isValid())
        kDebug(2100) << "there was some error using dbus." ;
}

void kgpgapplet::slotOpenKeyManager()
{
    OrgKdeKgpgKeyInterface kgpg("org.kde.kgpg", "/KeyInterface",QDBusConnection::sessionBus());
    QDBusReply<void> reply =kgpg.showKeyManager();
    if (!reply.isValid())
        kDebug(2100) << "there was some error using dbus." ;
}

void kgpgapplet::slotOpenServerDialog()
{
    OrgKdeKgpgKeyInterface kgpg("org.kde.kgpg", "/KeyInterface",QDBusConnection::sessionBus());
    QDBusReply<void> reply = kgpg.showKeyServer();
    if (!reply.isValid())
        kDebug(2100) << "there was some error using dbus." ;
}

kgpgapplet::~kgpgapplet()
{
    delete w;
}

KgpgAppletApp::KgpgAppletApp()
             : KUniqueApplication()
{
    running = false;
    kgpg_applet = NULL;
}

KgpgAppletApp::~KgpgAppletApp()
{
    delete s_keyManager;
    delete kgpg_applet;
}

void KgpgAppletApp::slotHandleQuit()
{
    s_keyManager->saveToggleOpts();
    quit();
}

void KgpgAppletApp::assistantOver(const QString &defaultKeyId)
{
    if (defaultKeyId.length() >= 8)
        s_keyManager->slotSetDefaultKey(defaultKeyId);

    s_keyManager->show();
    s_keyManager->raise();
}

int KgpgAppletApp::newInstance()
{
    args = KCmdLineArgs::parsedArgs();
    if (running)
    {
        kgpg_applet->show();
    }
    else
    {
        running = true;

        s_keyManager = new KeysManager();

        QString gpgPath = KGpgSettings::gpgConfigPath();
        if (!gpgPath.isEmpty())
            if (KUrl::fromPath(gpgPath).directory(KUrl::AppendTrailingSlash) != (QDir::homePath() + "/.gnupg/"))
                setenv("GNUPGHOME", KUrl::fromPath(gpgPath).directory(KUrl::AppendTrailingSlash).toAscii(), 1);

        s_keyManager->refreshkey();

        if (KGpgSettings::leftClick() == KGpgSettings::EnumLeftClick::KeyManager)
            kgpg_applet = new kgpgapplet(s_keyManager, s_keyManager->getModel());
        else
            kgpg_applet = new kgpgapplet(s_keyManager->s_kgpgEditor, s_keyManager->getModel());

        connect(s_keyManager,SIGNAL(encryptFiles(KUrl::List)),kgpg_applet->w,SLOT(encryptFiles(KUrl::List)));
        connect(s_keyManager->s_kgpgEditor,SIGNAL(encryptFiles(KUrl::List)),kgpg_applet->w,SLOT(encryptFiles(KUrl::List)));

        connect( kgpg_applet, SIGNAL(quitSelected()), this, SLOT(slotHandleQuit()));
        connect(s_keyManager,SIGNAL(readAgainOptions()),kgpg_applet->w,SLOT(readOptions()));
        connect(kgpg_applet->w, SIGNAL(updateDefault(QString)), SLOT(assistantOver(QString)));
        connect(kgpg_applet->w,SIGNAL(createNewKey()),s_keyManager,SLOT(slotGenerateKey()));
        connect(s_keyManager,SIGNAL(fontChanged(QFont)),kgpg_applet->w,SIGNAL(setFont(QFont)));

        if (KGpgSettings::showSystray()) {
            kgpg_applet->show();
        } else {
            kgpg_applet->setVisible(false);
            kgpg_applet->parentWidget()->show();
        }

        if (!gpgPath.isEmpty())
        {
            if ((KgpgInterface::getGpgBoolSetting("use-agent",gpgPath)) && (qgetenv("GPG_AGENT_INFO").isEmpty()))
                KMessageBox::sorry(0,i18n("<qt>The use of <b>GnuPG Agent</b> is enabled in GnuPG's configuration file (%1).<br />"
                        "However, the agent does not seem to be running. This could result in problems with signing/decryption.<br />"
                        "Please disable GnuPG Agent from KGpg settings, or fix the agent.</qt>", gpgPath));
        }
    }

    goHome = qobject_cast<KAction *>(s_keyManager->actionCollection()->action("go_default_key"))->shortcut();
    kgpg_applet->w->goDefaultKey = goHome;

    // parsing of command line args
    if (args->isSet("k") != 0)
    {
        s_keyManager->show();
        KWindowSystem::setOnDesktop(s_keyManager->winId(), KWindowSystem::currentDesktop());  //set on the current desktop
        KWindowSystem::unminimizeWindow(s_keyManager->winId());  //de-iconify window
        s_keyManager->raise();  // set on top
    }
    else
    if (args->isSet("d") != 0)
    {
       s_keyManager->slotOpenEditor();
       s_keyManager->hide();
    }
    else
    if (args->count() > 0)
    {
        urlList.clear();
        for (int ct = 0; ct < args->count(); ct++)
            urlList.append(args->url(ct));

        if (urlList.empty())
            return 0;

        kgpg_applet->w->droppedUrl = urlList.first();

        bool directoryInside = false;
        QStringList lst = urlList.toStringList();
        for (QStringList::const_iterator it = lst.begin(); it != lst.end(); ++it)
            if (KMimeType::findByUrl(KUrl(*it))->name() == "inode/directory")
                directoryInside = true;

        if ((directoryInside) && (lst.count() > 1))
        {
            KMessageBox::sorry(0, i18n("Unable to perform requested operation.\nPlease select only one folder, or several files, but do not mix files and folders."));
            return 0;
        }

        kgpg_applet->w->droppedUrls = urlList;

        if (args->isSet("e") != 0)
        {
            if (!directoryInside)
                kgpg_applet->w->encryptDroppedFile();
            else
                kgpg_applet->w->encryptDroppedFolder();
        }
        else
        if (args->isSet("s") != 0)
        {
            if (!directoryInside)
                kgpg_applet->w->showDroppedFile();
            else
                KMessageBox::sorry(0, i18n("Cannot decrypt and show folder."));
        }
        else
        if (args->isSet("S") != 0)
        {
            if (!directoryInside)
                kgpg_applet->w->signDroppedFile();
            else
                KMessageBox::sorry(0, i18n("Cannot sign folder."));
        }
        else
        if (args->isSet("V") != 0)
        {
            if (!directoryInside)
                kgpg_applet->w->slotVerifyFile();
            else
                KMessageBox::sorry(0, i18n("Cannot verify folder."));
        }
        else
        if (kgpg_applet->w->droppedUrl.fileName().endsWith(".sig"))
            kgpg_applet->w->slotVerifyFile();
        else
            kgpg_applet->w->decryptDroppedFile();
    }
    return 0;
}

#include "kgpg.moc"
