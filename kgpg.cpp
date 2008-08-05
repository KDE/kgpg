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
#include <QBoxLayout>
#include <QClipboard>
#include <QCheckBox>
#include <QWidget>
#include <QRegExp>
#include <QLayout>
#include <QCursor>
#include <QLabel>
#include <QMovie>
#include <QFile>
#include <Q3TextDrag>
#include <QtDBus>
#include <QProcess>

#include <KAboutApplicationDialog>
#include <KUrlRequesterDialog>
#include <KToolInvocation>
#include <kio/renamedialog.h>
#include <KPassivePopup>
#include <KUrlRequester>
#include <KStandardDirs>
#include <KDesktopFile>
#include <k3activelabel.h>
#include <KCmdLineArgs>
#include <KMessageBox>
#include <KFileDialog>
#include <KShortcut>
#include <KComboBox>
#include <KLineEdit>
#include <KTemporaryFile>
#include <KGlobal>
#include <KLocale>
#include <KConfig>
#include <KAction>
#include <KDebug>
#include <KMenu>
#include <KWindowSystem>
#include <ktip.h>
#include <KTar>
#include <KZip>
#include <K3ListBox>
#include <KActionCollection>
#include <KStandardAction>
#include <KSelectAction>
#include <KToggleAction>
#include <KIcon>
#include <kdefakes.h>
#include <KHBox>

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

using namespace KgpgCore;

static QString getGpgHome()
{
    char *env = getenv("GNUPGHOME");
    QString gpgHome;
    if (env != 0)
        gpgHome = env;
    else
        gpgHome = QDir::homePath() + "/.gnupg/";

    gpgHome.replace("//", "/");

    if(!gpgHome.endsWith('/'))
        gpgHome.append('/');

    KStandardDirs::makeDir(gpgHome, 0700);
    return gpgHome;
}

MyView::MyView(QWidget *parent, KSystemTrayIcon *parentTrayIcon)
      : QObject(parent)
{
    trayIcon = parentTrayIcon;

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
}

MyView::~MyView()
{
    delete droppopup;
    delete udroppopup;
    delete saveDecrypt;
    delete showDecrypt;
    delete encrypt;
    delete sign;
}

void MyView::clipEncrypt()
{
    if (kapp->clipboard()->text(clipboardMode).isEmpty())
        KPassivePopup::message(i18n("Clipboard is empty."), QString(), Images::kgpg(), trayIcon);
    else
    {
        KgpgSelectPublicKeyDlg *dialog = new KgpgSelectPublicKeyDlg(0, 0, false, goDefaultKey);
        if (dialog->exec() == KDialog::Accepted)
        {
            QStringList options;
            if (dialog->getUntrusted()) options << "--always-trust";
            if (dialog->getArmor())     options << "--armor";
            if (dialog->getHideId())    options << "--throw-keyid";

            if (!dialog->getCustomOptions().isEmpty())
                if (KGpgSettings::allowCustomEncryptionOptions())
                    options << dialog->getCustomOptions().split(" ", QString::SkipEmptyParts);

            encryptClipboard(dialog->selectedKeys(), options, dialog->getSymmetric());
        }

        delete dialog;
    }
}

void MyView::clipDecrypt()
{
    QString clippie = kapp->clipboard()->text(clipboardMode).simplified();
    droppedtext(clippie, false);
}

void MyView::clipSign(bool openEditor)
{
    QString clippie = kapp->clipboard()->text(clipboardMode).simplified();
    if (!clippie.isEmpty())
    {
        KgpgEditor *kgpgtxtedit = new KgpgEditor(0, Qt::WDestructiveClose, goDefaultKey);
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

void MyView::encryptDroppedFile()
{
    QStringList opts;
    KgpgLibrary *lib = new KgpgLibrary(0, KGpgSettings::pgpExtension());
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

        lib->slotFileEnc(droppedUrls, opts, QStringList(KGpgSettings::fileEncryptionKey().left(8)), goDefaultKey);
    }
    else
        lib->slotFileEnc(droppedUrls, opts, QStringList(), goDefaultKey);
}

void MyView::encryptDroppedFolder()
{
    compressionScheme = 0;
    kgpgfoldertmp = new KTemporaryFile();
    kgpgfoldertmp->open();

    if (KMessageBox::warningContinueCancel(0, i18n("<qt>KGpg will now create a temporary archive file:<br /><b>%1</b> to process the encryption. The file will be deleted after the encryption is finished.</qt>", kgpgfoldertmp->fileName()), i18n("Temporary File Creation"), KStandardGuiItem::cont(), KStandardGuiItem::cancel(), "FolderTmpFile") == KMessageBox::Cancel)
        return;

    /*
    KgpgSelectPublicKeyDlg *dialog = new KgpgSelectPublicKeyDlg(0, 0, false, true, goDefaultKey);
    if (dialog->exec() == KDialog::Accepted)
    {
        QStringList options;
        if (dialog->getUntrusted()) options << "--always-trust";
        if (dialog->getArmor())     options << "--armor";
        if (dialog->getHideId())    options << "--throw-keyid";

        if (!dialog->getCustomOptions().isEmpty())
            if (KGpgSettings::allowCustomEncryptionOptions())
                options << dialog->getCustomOptions().split(" ", QString::SkipEmptyParts);

        encryptClipboard(dialog->selectedKeys(), options, dialog->getSymmetric());
    }
    */



    // TODO !!! CHANGE dialog, remove connect
    dialog = new KgpgSelectPublicKeyDlg(0, droppedUrls.first().fileName(), true, goDefaultKey);

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

void MyView::slotAbortEnc()
{
    dialog = NULL;
}

void MyView::slotSetCompression(int cp)
{
    compressionScheme = cp;
}

void MyView::startFolderEncode()
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
    default:	Q_ASSERT(1); return;
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
    default:	Q_ASSERT(1); return;
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

    KgpgInterface *folderprocess = new KgpgInterface();
    connect(folderprocess, SIGNAL(fileEncryptionFinished(KUrl, KgpgInterface*)), this, SLOT(slotFolderFinished(KUrl, KgpgInterface*)));
    connect(folderprocess, SIGNAL(errorMessage(const QString &, KgpgInterface*)), this, SLOT(slotFolderFinishedError(const QString &, KgpgInterface*)));
    folderprocess->encryptFile(selec, KUrl(kgpgfoldertmp->fileName()), encryptedFile, encryptOptions, symetric);
}

void MyView::slotFolderFinished(const KUrl &, KgpgInterface *iface)
{
    delete pop;
    delete kgpgfoldertmp;
    iface->deleteLater();
}

void MyView::slotFolderFinishedError(const QString &errmsge, KgpgInterface *iface)
{
    delete pop;
    delete kgpgfoldertmp;
    iface->deleteLater();
    KMessageBox::sorry(0, errmsge);
}

void MyView::busyMessage(const QString &mssge, bool reset)
{
    if (reset)
        openTasks = 0;

    if (!mssge.isEmpty())
    {
        openTasks++;
        trayIcon->setToolTip(mssge);

#if 0
//TODO: kgpg_docked.gif is gone, replace this with the
//     "kgpg" icon overlayed by "user-busy", if necessary at all.
        QMovie *movie = new QMovie(KStandardDirs::locate("appdata", "pics/kgpg_docked.gif"));
        setMovie(movie);
        delete movie;
#endif
    }
    else
        openTasks--;

    //kDebug(2100) << "Emit message: " << openTasks ;

    if (openTasks <= 0)
    {
        trayIcon->setIcon(KIcon("kgpg"));
        trayIcon->setToolTip(i18n("KGpg - encryption tool"));
        openTasks = 0;
    }
}

void MyView::encryptFiles(KUrl::List urls)
{
    droppedUrls = urls;
    encryptDroppedFile();
}

void MyView::slotVerifyFile()
{
    // check file signature
    if (droppedUrl.isEmpty())
        return;

    QString sigfile = QString();
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
    KgpgInterface *verifyFileProcess=new KgpgInterface();
    connect (verifyFileProcess, SIGNAL(verifyquerykey(QString)), this, SLOT(importSignature(QString)));
    verifyFileProcess->KgpgVerifyFile(droppedUrl, KUrl(sigfile));
}

void MyView::importSignature(const QString &ID)
{
    KeyServer *kser = new KeyServer(0, false);
    kser->slotSetText(ID);
    kser->slotImport();
}

void MyView::signDroppedFile()
{
    // create a detached signature for a chosen file
    if (droppedUrl.isEmpty())
        return;

    QString signKeyID;
    // select a private key to sign file --> listkeys.cpp
    KgpgSelectSecretKey *opts = new KgpgSelectSecretKey(0, "select_secret");
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

    KgpgInterface *signFileProcess = new KgpgInterface();
    signFileProcess->KgpgSignFile(signKeyID, droppedUrl, Options);
}

void MyView::decryptDroppedFile()
{
    //bool isFolder=false;  // droppedUrls
    KUrl swapname;

    if (!droppedUrls.first().isLocalFile())
    {
        showDroppedFile();
        decryptNextFile();
    }

    QString oldname = droppedUrls.first().fileName();
    if (oldname.endsWith(".gpg") || oldname.endsWith(".asc") || oldname.endsWith(".pgp"))
        oldname.truncate(oldname.length()-4);
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
                    decryptNextFile();
                    return;
                }

                swapname=over.newDestUrl();
            }
        }

        KgpgLibrary *lib=new KgpgLibrary(0);
	QStringList custdecr;
	if (!KGpgSettings::customDecrypt().isEmpty())
		custdecr = QStringList(KGpgSettings::customDecrypt());
        lib->slotFileDec(droppedUrls.first(), swapname, custdecr);
        connect(lib,SIGNAL(importOver(QStringList)),this,SIGNAL(importedKeys(QStringList)));
        connect(lib,SIGNAL(systemMessage(QString,bool)),this,SLOT(busyMessage(QString,bool)));
//        if (isFolder)
        connect(lib,SIGNAL(decryptionOver()),this,SLOT(decryptNextFile()));
}

void MyView::decryptNextFile()
{
    if (droppedUrls.count() > 1)
    {
        droppedUrls.pop_front();
        decryptDroppedFile();
    }
}

void MyView::unArchive()
{
    KTar compressedFolder(kgpgFolderExtract->fileName(), "application/x-gzip");
    if (!compressedFolder.open(QIODevice::ReadOnly))
    {
        KMessageBox::sorry(0, i18n("Unable to read temporary archive file"));
        return;
    }

    const KArchiveDirectory *archiveDirectory = compressedFolder.directory();
    //KUrl savePath=KUrl::getURL(droppedUrl,this,i18n(""));
    KUrlRequesterDialog *savePath = new KUrlRequesterDialog(droppedUrl.directory(KUrl::AppendTrailingSlash), i18n("Extract to: "),0);
    savePath->fileDialog()->setMode(KFile::Directory);
    if (!savePath->exec() == QDialog::Accepted)
    {
        delete kgpgFolderExtract;
        return;
    }
    archiveDirectory->KArchiveDirectory::copyTo(savePath->selectedUrl().path());
    compressedFolder.close();
    delete savePath;
    delete kgpgFolderExtract;
}

void MyView::showDroppedFile()
{
    kDebug(2100) << "------Show dropped file" ;

    KgpgEditor *kgpgtxtedit = new KgpgEditor(0, Qt::WDestructiveClose, goDefaultKey);
    kgpgtxtedit->view->editor->slotDroppedFile(droppedUrls.first());

    connect(kgpgtxtedit, SIGNAL(encryptFiles(KUrl::List)), this, SLOT(encryptFiles(KUrl::List)));
    connect(this, SIGNAL(setFont(QFont)), kgpgtxtedit, SLOT(slotSetFont(QFont)));
    connect(kgpgtxtedit, SIGNAL(refreshImported(QStringList)), this, SIGNAL(importedKeys(QStringList)));
    kgpgtxtedit->show();
}

void MyView::droppedfile (KUrl::List url)
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

void MyView::droppedtext (const QString &inputText, bool allowEncrypt)
{
    if (inputText.startsWith("-----BEGIN PGP MESSAGE"))
    {
        KgpgEditor *kgpgtxtedit = new KgpgEditor(0, Qt::WDestructiveClose, goDefaultKey);
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
            connect(importKeyProcess, SIGNAL(importfinished(QStringList)), this, SIGNAL(importedKeys(QStringList)));
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

void MyView::dragEnterEvent(QDragEnterEvent *e)
{
    e->setAccepted(KUrl::List::canDecode(e->mimeData()) || Q3TextDrag::canDecode(e));
}

void MyView::dropEvent(QDropEvent *o)
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

void MyView::readOptions()
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
            if (KMessageBox::questionYesNo(0,i18n("<qt>You have not set a path to your GnuPG config file.<br />This may cause some surprising results in KGpg's execution.<br />Would you like to start KGpg's Wizard to fix this problem?</qt>"), QString(), KGuiItem(i18n("Start Wizard")), KGuiItem(i18n("Do Not Start"))) == KMessageBox::Yes)
                startWizard();
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

void MyView::firstRun()
{
    QProcess *createConfigProc = new QProcess(this);
    QStringList args;
    args << "--no-tty" << "--list-secret-keys";
    createConfigProc->start("gpg", args);// start gnupg so that it will create a config file
    createConfigProc->waitForFinished();
    startWizard();
}

void MyView::startWizard()
{
    kDebug(2100) << "Starting Wizard" ;

    wiz = new KgpgWizard(0);

    QString gpgHome(getGpgHome());
    QString confPath = gpgHome + "options";

    if (!QFile(confPath).exists())
    {
        confPath = gpgHome + "gpg.conf";
        if (!QFile(confPath).exists())
        {
            if (KMessageBox::questionYesNo(0, i18n("<qt><b>The GnuPG configuration file was not found</b>. Please make sure you have GnuPG installed. Should KGpg try to create a config file ?</qt>"), QString(), KGuiItem(i18n("Create Config")), KGuiItem(i18n("Do Not Create"))) == KMessageBox::Yes)
            {
                confPath = gpgHome + "gpg.conf";
                QFile file(confPath);
                if (file.open(QIODevice::WriteOnly))
                {
                    QTextStream stream(&file);
                    stream << "# GnuPG config file created by KGpg" << "\n";
                    file.close();
                }
            }
            else
            {
                wiz->text_optionsfound->setText(i18n("<qt><b>The GnuPG configuration file was not found</b>. Please make sure you have GnuPG installed and give the path to the config file.</qt>"));
                confPath.clear();
            }
        }
    }

    int gpgVersion = KgpgInterface::gpgVersion();
    if (gpgVersion < 120)
        wiz->txtGpgVersion->setText(i18n("Your GnuPG version seems to be older than 1.2.0. Photo Id's and Key Groups will not work properly. Please consider upgrading GnuPG (http://gnupg.org)."));
    else
        wiz->txtGpgVersion->setText(QString());

    wiz->kURLRequester1->setUrl(confPath);
        /*
    wiz->kURLRequester2->setURL(KGlobalSettings::desktopPath());
        wiz->kURLRequester2->setMode(2);*/

    QString firstKey = QString();

    KgpgInterface *interface = new KgpgInterface();
    KgpgKeyList secretlist = interface->readSecretKeys();

    KgpgKeyList publiclist = interface->readPublicKeys(true, secretlist);
    delete interface;

    for (int i = 0; i < publiclist.size(); ++i) {
        KgpgKey k = publiclist.at(i);

        QString s = k.id() + ": " + k.name() + " <" + k.email() + '>';

        wiz->CBdefault->addItem(s);
        if (firstKey.isEmpty())
            firstKey = s;
    }

    wiz->CBdefault->setCurrentItem(firstKey);

    //connect(wiz->pushButton4,SIGNAL(clicked()),this,SLOT(slotGenKey()));
    if (firstKey.isEmpty())
        connect(wiz->finishButton(),SIGNAL(clicked()),this,SLOT(slotGenKey()));
    else
    {
        wiz->textGenerate->hide();
        wiz->setTitle(wiz->page2, i18n("Step Three: Select your Default Private Key"));
        connect(wiz->finishButton(), SIGNAL(clicked()), this, SLOT(slotSaveOptionsPath()));
    }

    connect(wiz->nextButton(), SIGNAL(clicked()), this, SLOT(slotWizardChange()));
    connect(wiz, SIGNAL(destroyed()), this, SLOT(slotWizardClose()));
    connect(wiz, SIGNAL(helpClicked()), this, SLOT(help()));

    wiz->setFinishEnabled(wiz->page2, true);
    wiz->show();
}

void MyView::slotWizardChange()
{
    if (wiz->indexOf(wiz->currentPage()) == 2)
    {
        QString tst,name;
        KgpgInterface *iface = new KgpgInterface();
        QString defaultID = iface->getGpgSetting("default-key", wiz->kURLRequester1->url().path());

        if (defaultID.isEmpty()) {
            delete iface;
            return;
        }
        KgpgKeyList secl = iface->readSecretKeys(QStringList(defaultID));
        delete iface;

        if (secl.isEmpty())
            return;

        KgpgKey k = secl.at(0);

        wiz->CBdefault->setCurrentItem(k.id() + ": " + k.name() + " <" + k.email() + '>');
    }
}

void MyView::slotSaveOptionsPath()
{
    qWarning("Save wizard settings...");

    KGpgSettings::setAutoStart(wiz->checkBox2->isChecked());
    KGpgSettings::setGpgConfigPath(wiz->kURLRequester1->url().path());
    KGpgSettings::setFirstRun(false);

    QString defaultID = wiz->CBdefault->currentText().section(':', 0, 0);
/* if (!defaultID.isEmpty())
   {
            KGpgSettings::setDefaultKey(defaultID);
        }*/

    KGpgSettings::self()->writeConfig();
    emit updateDefault(defaultID);
    wiz->deleteLater();
}

void MyView::slotWizardClose()
{
    wiz = 0L;
}

void MyView::slotGenKey()
{
    slotSaveOptionsPath();
    emit createNewKey();
}

void MyView::about()
{
    KAboutApplicationDialog dialog(KGlobal::mainComponent().aboutData());//_aboutData);
    dialog.exec();
}

void MyView::help()
{
    KToolInvocation::invokeHelp(0, "kgpg");
}

void MyView::encryptClipboard(QStringList selec, QStringList encryptOptions, const bool symmetric)
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

    KgpgInterface *txtEncrypt = new KgpgInterface();
    connect (txtEncrypt, SIGNAL(txtEncryptionFinished(QString)), this, SLOT(slotSetClip(QString)));
    connect (txtEncrypt, SIGNAL(txtEncryptionStarted()), this, SLOT(slotPassiveClip()));
    txtEncrypt->encryptText(kapp->clipboard()->text(clipboardMode), selec, encryptOptions);
}

void MyView::slotPassiveClip()
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

void MyView::slotSetClip(const QString &newtxt)
{
    if (newtxt.isEmpty())
        return;

    kDebug(2100) << "Encrypted is " << newtxt ;
    kapp->clipboard()->setText(newtxt, clipboardMode);
}


kgpgapplet::kgpgapplet(QWidget *parent)
          : KSystemTrayIcon(parent)
{
    w = new MyView(parent, this);

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

void KgpgAppletApp::wizardOver(const QString &defaultKeyId)
{
    if (defaultKeyId.length() >= 8)
        s_keyManager->slotSetDefaultKey(defaultKeyId);

    s_keyManager->show();
    s_keyManager->raise();
}

int KgpgAppletApp::newInstance()
{
    kDebug(2100) << "New instance" ;

    args = KCmdLineArgs::parsedArgs();
    if (running)
    {
        kDebug(2100) << "Already running" ;
        kgpg_applet->show();
    }
    else
    {
        kDebug(2100) << "Starting KGpg" ;
        running = true;

        s_keyManager = new KeysManager();

        QString gpgPath = KGpgSettings::gpgConfigPath();
        if (!gpgPath.isEmpty())
            if (KUrl::fromPath(gpgPath).directory(KUrl::AppendTrailingSlash) != (QDir::homePath() + "/.gnupg/"))
                setenv("GNUPGHOME", KUrl::fromPath(gpgPath).directory(KUrl::AppendTrailingSlash).toAscii(), 1);

        s_keyManager->refreshkey();

        if (KGpgSettings::leftClick() == KGpgSettings::EnumLeftClick::KeyManager)
            kgpg_applet = new kgpgapplet(s_keyManager);
        else
            kgpg_applet = new kgpgapplet(s_keyManager->s_kgpgEditor);

        connect(s_keyManager,SIGNAL(encryptFiles(KUrl::List)),kgpg_applet->w,SLOT(encryptFiles(KUrl::List)));
        connect(s_keyManager->s_kgpgEditor,SIGNAL(encryptFiles(KUrl::List)),kgpg_applet->w,SLOT(encryptFiles(KUrl::List)));

        connect( kgpg_applet, SIGNAL(quitSelected()), this, SLOT(slotHandleQuit()));
        connect(s_keyManager,SIGNAL(readAgainOptions()),kgpg_applet->w,SLOT(readOptions()));
        connect(kgpg_applet->w,SIGNAL(updateDefault(QString)),this,SLOT(wizardOver(QString)));
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
            if ((KgpgInterface::getGpgBoolSetting("use-agent",gpgPath)) && (!getenv("GPG_AGENT_INFO")))
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
#ifdef Q_OS_UNIX
        KWindowSystem::setOnDesktop(s_keyManager->winId(), KWindowSystem::currentDesktop());  //set on the current desktop
        KWindowSystem::unminimizeWindow(s_keyManager->winId());  //de-iconify window
#endif
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
        kDebug(2100) << "KGpg: found files" ;

        urlList.clear();
        for (int ct = 0; ct < args->count(); ct++)
            urlList.append(args->url(ct));

        if (urlList.empty())
            return 0;

        kgpg_applet->w->droppedUrl = urlList.first();

        bool directoryInside = false;
        QStringList lst = urlList.toStringList();
        for (QStringList::Iterator it = lst.begin(); it != lst.end(); ++it)
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
