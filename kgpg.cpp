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

#include <stdlib.h>

#include <QDragEnterEvent>
#include <QDesktopWidget>
#include <QTextStream>
#include <QDropEvent>
#include <QBoxLayout>
#include <QClipboard>
#include <QCheckBox>
#include <QToolTip>
#include <QWidget>
#include <QRegExp>
#include <QLayout>
#include <QCursor>
#include <QLabel>
#include <QMovie>
#include <QFile>

#include <Q3TextDrag>

#include <kaboutapplication.h>
#include <kurlrequesterdlg.h>
#include <ktoolinvocation.h>
#include <kio/renamedlg.h>
#include <kpassivepopup.h>
#include <kurlrequester.h>
#include <kstandarddirs.h>
#include <kdesktopfile.h>
#include <kactivelabel.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kdeversion.h>
#include <kshortcut.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <ktempfile.h>
#include <kprocess.h>
#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>
#include <kprocio.h>
#include <kaction.h>
#include <kdebug.h>
#include <kmenu.h>
#include <kwin.h>
#include <ktip.h>
#include <ktar.h>
#include <kzip.h>
#include <klistbox.h>
#include <kactioncollection.h>
#include <kstdaction.h>
#include <QtDBus>
#include <kselectaction.h>
#include <ktoggleaction.h>
#include "selectsecretkey.h"
#include "kgpgeditor.h"
#include "kgpg.h"
#include "kgpgsettings.h"   // automatically created by compilation
#include "listkeys.h"
#include "keyserver.h"
#include "keyservers.h"
#include "popuppublic.h"
#include "kgpgview.h"
#include "kgpglibrary.h"
#include "kgpgwizard.h"

static QString getGpgHome()
{
    char *env = getenv("GNUPGHOME");
    QString gpgHome;
    if (env != 0)
        gpgHome = env;
    else
        gpgHome = QDir::homePath() + "/.gnupg/";

    gpgHome.replace("//", "/");

    if(!gpgHome.endsWith("/"))
        gpgHome.append('/');

    KStandardDirs::makeDir(gpgHome, 0700);
    return gpgHome;
}

MyView::MyView(QWidget *parent, const char *name)
      : QLabel(parent, name)
{
    //setBackgroundMode( X11ParentRelative );
    openTasks = 0;

    saveDecrypt = new KAction(KIcon(QString("decrypted")), i18n("&Decrypt && Save File"), 0, "decrypt_file");
    connect(saveDecrypt, SIGNAL(triggered(bool)), SLOT(decryptDroppedFile()));
    showDecrypt = new KAction(KIcon(QString("edit")), i18n("&Show Decrypted File"), 0, "show_file");
    connect(showDecrypt, SIGNAL(triggered(bool)), SLOT(showDroppedFile()));
    encrypt = new KAction(KIcon(QString("encrypted")), i18n("&Encrypt File"), 0, "encrypt_file");
    connect(encrypt, SIGNAL(triggered(bool)), SLOT(encryptDroppedFile()));
    sign = new KAction(KIcon(QString("signature")), i18n("&Sign File"), 0, "sign_file");
    connect(sign, SIGNAL(triggered(bool)), SLOT(signDroppedFile()));

    readOptions();
    resize(24,24);
    setPixmap(KSystemTray::loadIcon("kgpg_docked"));
    setAcceptDrops(true);

    droppopup = new KMenu();
    droppopup->addAction( showDecrypt );
    droppopup->addAction( saveDecrypt );

    udroppopup = new KMenu();
    udroppopup->addAction( encrypt );
    udroppopup->addAction( sign );

    this->setToolTip( i18n("KGpg - encryption tool"));
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
        KPassivePopup::message(i18n("Clipboard is empty."), QString::null, KGlobal::iconLoader()->loadIcon("kgpg", K3Icon::Desktop), this);
    else
    {
        KgpgSelectPublicKeyDlg *dialog = new KgpgSelectPublicKeyDlg(0, 0, false, true, goDefaultKey);
        if (dialog->exec() == KDialog::Accepted)
        {
            QStringList options;
            if (dialog->getUntrusted()) options << "--always-trust";
            if (dialog->getArmor())     options << "--armor";
            if (dialog->getHideId())    options << "--throw-keyid";

            if (!dialog->getCustomOptions().isEmpty())
                if (KGpgSettings::allowCustomEncryptionOptions())
                    options << dialog->getCustomOptions().split(" ");

            encryptClipboard(dialog->selectedKeys(), options, dialog->getShred(), dialog->getSymmetric());
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
        KgpgEditor *kgpgtxtedit = new KgpgEditor(0, "editor", Qt::WDestructiveClose, goDefaultKey);
        connect(this,SIGNAL(setFont(QFont)), kgpgtxtedit, SLOT(slotSetFont(QFont)));
        connect(kgpgtxtedit, SIGNAL(encryptFiles(KUrl::List)), this, SLOT(encryptFiles(KUrl::List)));

        if (!openEditor)
            connect(kgpgtxtedit->view, SIGNAL(verifyFinished()), kgpgtxtedit, SLOT(closeWindow()));

        kgpgtxtedit->view->editor->setPlainText(clippie);
        kgpgtxtedit->view->slotSignVerify();
        kgpgtxtedit->show();
    }
    else
        KPassivePopup::message(i18n("Clipboard is empty."), QString::null, KGlobal::iconLoader()->loadIcon("kgpg", K3Icon::Desktop), this);
}

void MyView::encryptDroppedFile()
{
    QStringList opts;
    KgpgLibrary *lib = new KgpgLibrary(this, KGpgSettings::pgpExtension());
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
    kgpgfoldertmp = new KTempFile(QString::null);
    kgpgfoldertmp->setAutoDelete(true);

    if (KMessageBox::warningContinueCancel(0, i18n("<qt>KGpg will now create a temporary archive file:<br><b>%1</b> to process the encryption. The file will be deleted after the encryption is finished.</qt>", kgpgfoldertmp->name()), i18n("Temporary File Creation"), KStdGuiItem::cont(), "FolderTmpFile") == KMessageBox::Cancel)
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
                options << dialog->getCustomOptions().split(" ");

        encryptClipboard(dialog->selectedKeys(), options, dialog->getShred(), dialog->getSymmetric());
    }
    */



    // TODO !!! CHANGE dialog, remove connect
    dialogue = new KgpgSelectPublicKeyDlg(0, droppedUrls.first().fileName(), true, false, goDefaultKey);

    QGroupBox *bGroup = new QGroupBox(dialogue->mainWidget());

    (void) new QLabel(i18n("Compression method for archive:"),bGroup);

    KComboBox *optionbx = new KComboBox(bGroup);
    optionbx->addItem(i18n("Zip"));
    optionbx->addItem(i18n("Gzip"));
    optionbx->addItem(i18n("Bzip2"));

    bGroup->show();
    connect(optionbx,SIGNAL(activated (int)),this,SLOT(slotSetCompression(int)));
    connect(dialogue,SIGNAL(selectedKey(QStringList,QStringList,bool,bool)),this,SLOT(startFolderEncode(QStringList,QStringList,bool,bool)));

    dialogue->exec();
    dialogue=0L;
}

void MyView::slotSetCompression(int cp)
{
    compressionScheme = cp;
}

void MyView::startFolderEncode(QStringList selec,QStringList encryptOptions,bool ,bool symetric)
{
    QString extension;

    if (compressionScheme == 0)
        extension = ".zip";
    else
    if (compressionScheme == 1)
        extension = ".tar.gz";
    else
        extension = ".tar.bz2";

    if (encryptOptions.contains("armor"))
        extension += ".asc";
    else
    if (KGpgSettings::pgpExtension())
        extension += ".pgp";
    else
        extension += ".gpg";

    KUrl encryptedFile(droppedUrls.first().path() + extension);
    QFile encryptedFolder(droppedUrls.first().path() + extension);
    if (encryptedFolder.exists())
    {
        dialogue->hide();
        KIO::RenameDlg *over = new KIO::RenameDlg(0, i18n("File Already Exists"), QString::null, encryptedFile.path(), KIO::M_OVERWRITE);
        if (over->exec() == QDialog::Rejected)
        {
            delete over;
            return;
        }
        encryptedFile = over->newDestURL();
        delete over;
        dialogue->show(); // strange, but if dialogue is hidden, the passive popup is not displayed...
    }

    pop = new KPassivePopup();
    pop->setView(i18n("Processing folder compression and encryption"),i18n("Please wait..."), KGlobal::iconLoader()->loadIcon("kgpg", K3Icon::Desktop));
    pop->setAutoDelete(false);
    pop->show();
    kapp->processEvents();
    dialogue->accept();
    dialogue = 0L;

    KArchive *arch;
    if (compressionScheme == 0)
        arch = new KZip(kgpgfoldertmp->name());
    else
    if (compressionScheme == 1)
        arch = new KTar(kgpgfoldertmp->name(), "application/x-gzip");
    else
        arch = new KTar(kgpgfoldertmp->name(), "application/x-bzip2");

    if (!arch->open(QIODevice::WriteOnly))
    {
        KMessageBox::sorry(0,i18n("Unable to create temporary file"));
        return;
    }

    arch->addLocalDirectory(droppedUrls.first().path(), droppedUrls.first().fileName());
    arch->close();

    KgpgInterface *folderprocess = new KgpgInterface();
    connect(folderprocess, SIGNAL(fileEncryptionFinished(KUrl)), this, SLOT(slotFolderFinished(KUrl, KgpgInterface*)));
    connect(folderprocess, SIGNAL(errorMessage(QString)), this, SLOT(slotFolderFinishedError(QString, KgpgInterface*)));
    folderprocess->encryptFile(selec, KUrl(kgpgfoldertmp->name()), encryptedFile, encryptOptions, symetric);
}

void MyView::slotFolderFinished(KUrl, KgpgInterface*)
{
    delete pop;
    delete kgpgfoldertmp;
}

void MyView::slotFolderFinishedError(QString errmsge, KgpgInterface*)
{
    delete pop;
    delete kgpgfoldertmp;
    KMessageBox::sorry(0, errmsge);
}

void MyView::busyMessage(QString mssge, bool reset)
{
    if (reset)
        openTasks = 0;

    if (!mssge.isEmpty())
    {
        openTasks++;
        this->setToolTip(mssge);

        QMovie *movie = new QMovie(KStandardDirs::locate("appdata", "pics/kgpg_docked.gif"));
        setMovie(movie);
        delete movie;
    }
    else
        openTasks--;

    //kDebug(2100) << "Emit message: " << openTasks << endl;

    if (openTasks <= 0)
    {
        setPixmap(KSystemTray::loadIcon("kgpg_docked"));
        this->setToolTip(i18n("KGpg - encryption tool"));
        openTasks = 0;
    }
}

void MyView::encryptFiles(KUrl::List urls)
{
    droppedUrls = urls;
    encryptDroppedFile();
}

void MyView::shredDroppedFile()
{
    KDialog *shredConfirm = new KDialog(this );
    shredConfirm->setCaption( i18n("Shred Files") );
    shredConfirm->setButtons( KDialog::Ok | KDialog::Cancel);
    shredConfirm->setDefaultButton( KDialog::Ok );
    shredConfirm->setModal(true);
    QWidget *page = new QWidget(shredConfirm);
    shredConfirm->setMainWidget(page);
    QBoxLayout *layout = new QBoxLayout(QBoxLayout::TopToBottom, page);
    layout->setAutoAdd(true);

    QString mess = i18n("Do you really want to <a href=\"whatsthis:%1\">shred</a> these files?",
                        i18n("<qt><p>You must be aware that <b>shredding is not secure</b> on all file systems, and that parts of the file may have been saved in a temporary file or in the spooler of your printer if you previously opened it in an editor or tried to print it. Only works on files (not on folders).</p></qt>"));

    (void) new KActiveLabel(mess, page);
    KListBox *lb = new KListBox(page);
    lb->insertStringList(droppedUrls.toStringList());
    if (shredConfirm->exec() == QDialog::Accepted)
    {
        KgpgLibrary *lib = new KgpgLibrary(this);
        connect(lib, SIGNAL(systemMessage(QString, bool)), this, SLOT(busyMessage(QString, bool)));
        lib->shredProcessEnc(droppedUrls);
    }
    delete shredConfirm;
}

void MyView::slotVerifyFile()
{
    // check file signature
    if (droppedUrl.isEmpty())
        return;

    QString sigfile = QString::null;
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
                sigfile = QString::null;
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

void MyView::importSignature(QString ID)
{
    keyServer *kser = new keyServer(0, "server_dialog", false);
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
                kgpgFolderExtract=new KTempFile(QString::null,".tar.gz");
                kgpgFolderExtract->setAutoDelete(true);
                swapname=KUrl(kgpgFolderExtract->name());
                if (KMessageBox::warningContinueCancel(0,i18n("<qt>The file to decrypt is an archive. KGpg will create a temporary unencrypted archive file:<br><b>%1</b> before processing the archive extraction. This temporary file will be deleted after the decryption is finished.</qt>").arg(kgpgFolderExtract->name()),i18n("Temporary File Creation"),KStdGuiItem::cont(),"FolderTmpDecFile")==KMessageBox::Cancel)
                        return;
        } else*/
        {
            swapname=KUrl(droppedUrls.first().directory(KUrl::IgnoreTrailingSlash)+oldname);
            QFile fgpg(swapname.path());
            if (fgpg.exists())
            {
                KIO::RenameDlg *over=new KIO::RenameDlg(0,i18n("File Already Exists"),QString::null,swapname.path(),KIO::M_OVERWRITE);
                if (over->exec()==QDialog::Rejected)
                {
                    delete over;
                    decryptNextFile();
                    return;
                }

                swapname=over->newDestURL();
                delete over;
            }
        }

        KgpgLibrary *lib=new KgpgLibrary(this);
        lib->slotFileDec(droppedUrls.first(), swapname, QStringList(KGpgSettings::customDecrypt()));
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
    KTar compressedFolder(kgpgFolderExtract->name(), "application/x-gzip");
    if (!compressedFolder.open(QIODevice::ReadOnly))
    {
        KMessageBox::sorry(0, i18n("Unable to read temporary archive file"));
        return;
    }

    const KArchiveDirectory *archiveDirectory = compressedFolder.directory();
    //KUrl savePath=KUrl::getURL(droppedUrl,this,i18n(""));
    KUrlRequesterDlg *savePath = new KUrlRequesterDlg(droppedUrl.directory(false), i18n("Extract to: "),0);
    savePath->fileDialog()->setMode(KFile::Directory);
    if (!savePath->exec() == QDialog::Accepted)
    {
        delete kgpgFolderExtract;
        return;
    }
    archiveDirectory->KArchiveDirectory::copyTo(savePath->selectedURL().path());
    compressedFolder.close();
    delete savePath;
    delete kgpgFolderExtract;
}

void MyView::showDroppedFile()
{
    kDebug(2100) << "------Show dropped file" << endl;

    KgpgEditor *kgpgtxtedit = new KgpgEditor(0, "editor", Qt::WDestructiveClose, goDefaultKey);
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
    if (KMimeType::findByURL(droppedUrl)->name() == "inode/directory")
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
                kDebug(2100)<<"Drop menu--------"<<endl;
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

void MyView::droppedtext (QString inputText, bool allowEncrypt)
{
    if (inputText.startsWith("-----BEGIN PGP MESSAGE"))
    {
        KgpgEditor *kgpgtxtedit = new KgpgEditor(0, "editor", Qt::WDestructiveClose, goDefaultKey);
        connect(kgpgtxtedit, SIGNAL(encryptFiles(KUrl::List)), this, SLOT(encryptFiles(KUrl::List)));
        connect(this, SIGNAL(setFont(QFont)), kgpgtxtedit, SLOT(slotSetFont(QFont)));
        kgpgtxtedit->view->editor->setPlainText(inputText);
        kgpgtxtedit->view->slotDecode();
        kgpgtxtedit->show();
        return;
    }

    if (inputText.startsWith("-----BEGIN PGP PUBLIC KEY"))
    {
        int result = KMessageBox::warningContinueCancel(0,i18n("<p>The dropped text is a public key.<br>Do you want to import it ?</p>"), i18n("Warning"));
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
        KMessageBox::sorry(this, i18n("No encrypted text found."));
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
            if (KMessageBox::questionYesNo(0,i18n("<qt>You have not set a path to your GnuPG config file.<br>This may cause some surprising results in KGpg's execution.<br>Would you like to start KGpg's Wizard to fix this problem?</qt>"),QString::null,i18n("Start Wizard"),i18n("Do Not Start"))==KMessageBox::Yes)
                startWizard();
        }
        else
        {
            QStringList groups=KgpgInterface::getGpgGroupNames(path);
            if (!groups.isEmpty())
                    KGpgSettings::setGroups(groups.join(","));
        }
    }
}

void MyView::firstRun()
{
    KProcIO *p = new KProcIO();
    *p << "gpg" << "--no-tty" << "--list-secret-keys";
    p->start(KProcess::Block); // start gnupg so that it will create a config file
    startWizard();
}

void MyView::startWizard()
{
    kDebug(2100) << "Starting Wizard" << endl;

    wiz = new KgpgWizard(0, "wizard");

    QString gpgHome(getGpgHome());
    QString confPath = gpgHome + "options";

    if (!QFile(confPath).exists())
    {
        confPath = gpgHome + "gpg.conf";
        if (!QFile(confPath).exists())
        {
            if (KMessageBox::questionYesNo(this, i18n("<qt><b>The GnuPG configuration file was not found</b>. Please make sure you have GnuPG installed. Should KGpg try to create a config file ?</qt>"), QString::null, i18n("Create Config"), i18n("Do Not Create")) == KMessageBox::Yes)
            {
                confPath = gpgHome + "options";
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
                confPath = QString::null;
            }
        }
    }

    int gpgVersion = KgpgInterface::getGpgVersion();
    if (gpgVersion < 120)
        wiz->txtGpgVersion->setPlainText(i18n("Your GnuPG version seems to be older than 1.2.0. Photo Id's and Key Groups will not work properly. Please consider upgrading GnuPG (http://gnupg.org)."));
    else
        wiz->txtGpgVersion->setPlainText(QString::null);

    wiz->kURLRequester1->setUrl(confPath);
        /*
    wiz->kURLRequester2->setURL(KGlobalSettings::desktopPath());
        wiz->kURLRequester2->setMode(2);*/

    FILE *fp,*fp2;
    QString tst;
    QString tst2;
    QString name;
    QString trustedvals = "idre-";
    QString firstKey = QString::null;
    char line[300];
    bool counter = false;

    fp = popen("gpg --no-tty --with-colon --list-secret-keys", "r");
    while (fgets(line, sizeof(line), fp))
    {
        tst = line;
        if (tst.startsWith("sec"))
        {
            name = KgpgInterface::checkForUtf8(tst.section(':', 9, 9));
            if (!name.isEmpty())
            {
                fp2 = popen("gpg --no-tty --with-colon --list-keys " + QFile::encodeName(tst.section(':',4,4)), "r");
                while (fgets( line, sizeof(line), fp2))
                {
                    tst2 = line;
                    if (tst2.startsWith("pub") && !trustedvals.contains(tst2.section(':',1,1)))
                    {
                        counter = true;
                        wiz->CBdefault->addItem(tst.section(':', 4, 4).right(8) + ": " + name);
                        if (firstKey.isEmpty())
                            firstKey=tst.section(':',4,4).right(8)+": "+name;
                        break;
                    }
                }
                pclose(fp2);
            }
        }
    }
    pclose(fp);
    wiz->CBdefault->setCurrentItem(firstKey);

    //connect(wiz->pushButton4,SIGNAL(clicked()),this,SLOT(slotGenKey()));
    if (!counter)
        connect(wiz->finishButton(),SIGNAL(clicked()),this,SLOT(slotGenKey()));
    else
    {
        wiz->textGenerate->hide();
        wiz->setTitle(wiz->page3, i18n("Step Three: Select your Default Private Key"));
        connect(wiz->finishButton(), SIGNAL(clicked()), this, SLOT(slotSaveOptionsPath()));
    }

    connect(wiz->nextButton(), SIGNAL(clicked()), this, SLOT(slotWizardChange()));
    connect(wiz, SIGNAL(destroyed()), this, SLOT(slotWizardClose()));
    connect(wiz, SIGNAL(helpClicked()), this, SLOT(help()));

    wiz->setFinishEnabled(wiz->page3, true);
    wiz->show();
}

void MyView::slotWizardChange()
{
    QString tst,name;
    char line[300];
    FILE *fp;

    if (wiz->indexOf(wiz->currentPage()) == 2)
    {
        QString defaultID = KgpgInterface::getGpgSetting("default-key", wiz->kURLRequester1->url().path());
        if (defaultID.isEmpty())
            return;
        fp = popen("gpg --no-tty --with-colon --list-secret-keys " + QFile::encodeName(defaultID), "r");
        while (fgets( line, sizeof(line), fp))
        {
            tst = line;
            if (tst.startsWith("sec"))
            {
                name = KgpgInterface::checkForUtf8(tst.section(':', 9, 9));
                wiz->CBdefault->setCurrentItem(tst.section(':', 4, 4).right(8) + ": " + name);
            }
        }
        pclose(fp);
    }
}

void MyView::installShred()
{
    KUrl path;
    path.setPath(KGlobalSettings::desktopPath());
    path.addPath("shredder.desktop");
    KDesktopFile configl2(path.path(), false);
    if (configl2.isImmutable() == false)
    {
        configl2.setGroup("Desktop Entry");
        configl2.writeEntry("Type", "Application");
        configl2.writeEntry("Name", i18n("Shredder"));
        configl2.writeEntry("Icon", "editshred");
        configl2.writeEntry("Exec", "kgpg -X %U");
    }
}

void MyView::slotSaveOptionsPath()
{
    qWarning("Save wizard settings...");
    if (wiz->checkBox1->isChecked())
        installShred();

    KGpgSettings::setAutoStart(wiz->checkBox2->isChecked());
    KGpgSettings::setGpgConfigPath(wiz->kURLRequester1->url().path());
    KGpgSettings::setFirstRun(false);

    QString defaultID = wiz->CBdefault->currentText().section(':', 0, 0);
/* if (!defaultID.isEmpty())
   {
            KGpgSettings::setDefaultKey(defaultID);
        }*/

    KGpgSettings::writeConfig();
    emit updateDefault("0x" + defaultID);
    delete wiz;
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
    KAboutApplication dialog(kapp->aboutData());//_aboutData);
    dialog.exec();
}

void MyView::help()
{
    KToolInvocation::invokeHelp(0, "kgpg");
}

void MyView::encryptClipboard(QStringList selec,QStringList encryptOptions,bool,bool symmetric)
{
    if (kapp->clipboard()->text(clipboardMode).isEmpty())
    {
        KPassivePopup::message(i18n("Clipboard is empty."), QString::null, KGlobal::iconLoader()->loadIcon("kgpg", K3Icon::Desktop), this);
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

    pop = new KPassivePopup( this);
    pop->setView(i18n("Encrypted following text:"), newtxt, KGlobal::iconLoader()->loadIcon("kgpg", K3Icon::Desktop));
    pop->setTimeout(3200);
    pop->show();
    QRect qRect(QApplication::desktop()->screenGeometry());
    int iXpos = qRect.width() / 2 - pop->width() / 2;
    int iYpos = qRect.height() / 2 - pop->height() / 2;
    pop->move(iXpos, iYpos);
}

void MyView::slotSetClip(QString newtxt)
{
    if (newtxt.isEmpty())
        return;

    kDebug(2100) << "Encrypted is " << newtxt << endl;
    kapp->clipboard()->setText(newtxt, clipboardMode); //,QClipboard::Clipboard);    QT 3.1 only
}


kgpgapplet::kgpgapplet(QWidget *parent)
          : KSystemTray(parent)
{
    w = new MyView(this);
    w->show();

    KMenu *conf_menu = contextMenu();

    KAction *KgpgOpenEditor;

    if (KGpgSettings::leftClick() == KGpgSettings::EnumLeftClick::KeyManager)
    {
        KgpgOpenEditor = new KAction(KIcon("edit"), i18n("&Open Editor"), actionCollection(), "kgpg_editor");
        connect(KgpgOpenEditor, SIGNAL(triggered(bool)), parent, SLOT(slotOpenEditor()));
    } else {
        KgpgOpenEditor = new KAction(KIcon("kgpg"), i18n("&Open Key Manager"), actionCollection(), "kgpg_editor");
        connect(KgpgOpenEditor, SIGNAL(triggered(bool)), SLOT(slotOpenKeyManager()));
    }

    KAction *KgpgEncryptClipboard = new KAction(i18n("&Encrypt Clipboard"), actionCollection(), "clip_encrypt");
    connect(KgpgEncryptClipboard, SIGNAL(triggered(bool)), w, SLOT(clipEncrypt()));
    KAction *KgpgDecryptClipboard = new KAction(i18n("&Decrypt Clipboard"), actionCollection(), "clip_decrypt");
    connect(KgpgDecryptClipboard, SIGNAL(triggered(bool)), w, SLOT(clipDecrypt()));
    KAction *KgpgSignClipboard = new KAction(i18n("&Sign/Verify Clipboard"), actionCollection(), "clip_sign");
    connect(KgpgSignClipboard, SIGNAL(triggered(bool)), w, SLOT(clipSign()));

    KAction *KgpgOpenServer = new KAction(KIcon("network"), i18n("&Key Server Dialog"), actionCollection(), "kgpg_server");
    connect(KgpgOpenServer, SIGNAL(triggered(bool)), SLOT(slotOpenServerDialog()));
    KAction *KgpgPreferences = KStdAction::preferences(this, SLOT(showOptions()), actionCollection());

    conf_menu->addAction( KgpgEncryptClipboard );
    conf_menu->addAction( KgpgDecryptClipboard );
    conf_menu->addAction( KgpgSignClipboard );
    conf_menu->addAction( KgpgOpenEditor );
    conf_menu->addAction( KgpgOpenServer );
    conf_menu->addSeparator();
    conf_menu->addAction( KgpgPreferences );
}

void kgpgapplet::showOptions()
{
    QDBusInterface kgpg( "org.kde.kgpg", "/KeyInterface", "org.kde.kgpg.KeyInterface" );
    QDBusReply<void> reply =kgpg.call( "showOptions" );
    if (!reply.isSuccess())
        kDebug(2100) << "there was some error using dbus." << endl;
}

void kgpgapplet::slotOpenKeyManager()
{
    QDBusInterface kgpg( "org.kde.kgpg", "/KeyInterface", "org.kde.kgpg.KeyInterface" );
    QDBusReply<void> reply =kgpg.call( "showKeyManager" );
    if (!reply.isSuccess())
        kDebug(2100) << "there was some error using dbus." << endl;
}

void kgpgapplet::slotOpenServerDialog()
{
    QDBusInterface kgpg( "org.kde.kgpg", "/KeyInterface", "org.kde.kgpg.KeyInterface" );
    QDBusReply<void> reply =kgpg.call( "showKeyServer" );
    if (!reply.isSuccess())
        kDebug(2100) << "there was some error using dbus." << endl;
}

kgpgapplet::~kgpgapplet()
{
    delete w;
}

KgpgAppletApp::KgpgAppletApp()
             : KUniqueApplication()
{
    running = false;
}

KgpgAppletApp::~KgpgAppletApp()
{
    delete s_keyManager;
    delete kgpg_applet;
}

void KgpgAppletApp::slotHandleQuit()
{
    s_keyManager->keysList2->saveLayout(KGlobal::config(), "KeyView");
    KGpgSettings::setPhotoProperties(s_keyManager->photoProps->currentItem());
    KGpgSettings::setShowTrust(s_keyManager->sTrust->isChecked());
    KGpgSettings::setShowExpi(s_keyManager->sExpi->isChecked());
    KGpgSettings::setShowCreat(s_keyManager->sCreat->isChecked());
    KGpgSettings::setShowSize(s_keyManager->sSize->isChecked());
    KGpgSettings::writeConfig();
    KSimpleConfig ("kgpgrc").sync();
    quit();
}

void KgpgAppletApp::wizardOver(QString defaultKeyId)
{
    if (defaultKeyId.length() == 10)
        s_keyManager->slotSetDefaultKey(defaultKeyId);

    s_keyManager->show();
    s_keyManager->raise();
}

int KgpgAppletApp::newInstance()
{
    kDebug(2100) << "New instance" << endl;

    args = KCmdLineArgs::parsedArgs();
    if (running)
    {
        kDebug(2100) << "Already running" << endl;
        kgpg_applet->show();
    }
    else
    {
        kDebug(2100) << "Starting KGpg" << endl;
        running = true;

        s_keyManager = new KeysManager();

        QString gpgPath = KGpgSettings::gpgConfigPath();
        if (!gpgPath.isEmpty())
            if (KUrl::fromPath(gpgPath).directory(false) != (QDir::homePath() + "/.gnupg/"))
                setenv("GNUPGHOME", KUrl::fromPath(gpgPath).directory(false).toAscii(), 1);

        s_keyManager->refreshkey();

        if (KGpgSettings::leftClick() == KGpgSettings::EnumLeftClick::KeyManager)
            kgpg_applet = new kgpgapplet(s_keyManager);
        else
            kgpg_applet = new kgpgapplet(s_keyManager->s_kgpgEditor);

        connect(s_keyManager,SIGNAL(encryptFiles(KUrl::List)),kgpg_applet->w,SLOT(encryptFiles(KUrl::List)));
        connect(s_keyManager,SIGNAL(installShredder()),kgpg_applet->w,SLOT(installShred()));
        connect(s_keyManager->s_kgpgEditor,SIGNAL(encryptFiles(KUrl::List)),kgpg_applet->w,SLOT(encryptFiles(KUrl::List)));

        connect( kgpg_applet, SIGNAL(quitSelected()), this, SLOT(slotHandleQuit()));
        connect(s_keyManager,SIGNAL(readAgainOptions()),kgpg_applet->w,SLOT(readOptions()));
        connect(kgpg_applet->w,SIGNAL(updateDefault(QString)),this,SLOT(wizardOver(QString)));
        connect(kgpg_applet->w,SIGNAL(createNewKey()),s_keyManager,SLOT(slotGenerateKey()));
        connect(s_keyManager,SIGNAL(fontChanged(QFont)),kgpg_applet->w,SIGNAL(setFont(QFont)));
        connect(kgpg_applet->w,SIGNAL(importedKeys(QStringList)),s_keyManager->keysList2,SLOT(slotReloadKeys(QStringList)));
        kgpg_applet->show();


        if (!gpgPath.isEmpty())
        {
            if ((KgpgInterface::getGpgBoolSetting("use-agent",gpgPath)) && (!getenv("GPG_AGENT_INFO")))
                KMessageBox::sorry(0,i18n("<qt>The use of <b>GnuPG Agent</b> is enabled in GnuPG's configuration file (%1).<br>"
                        "However, the agent does not seem to be running. This could result in problems with signing/decryption.<br>"
                        "Please disable GnuPG Agent from KGpg settings, or fix the agent.</qt>", gpgPath));
        }
    }

    goHome = s_keyManager->actionCollection()->action("go_default_key")->shortcut();
    kgpg_applet->w->goDefaultKey = goHome;

    // parsing of command line args
    if (args->isSet("k") != 0)
    {
        s_keyManager->show();
        KWin::setOnDesktop(s_keyManager->winId(), KWin::currentDesktop());  //set on the current desktop
        KWin::deIconifyWindow(s_keyManager->winId());  //de-iconify window
        s_keyManager->raise();  // set on top
    }
    else
    if (args->count() > 0)
    {
        kDebug(2100) << "KGpg: found files" << endl;

        urlList.clear();
        for (int ct = 0; ct < args->count(); ct++)
            urlList.append(args->url(ct));

        if (urlList.empty())
            return 0;

        kgpg_applet->w->droppedUrl = urlList.first();

        bool directoryInside = false;
        QStringList lst = urlList.toStringList();
        for (QStringList::Iterator it = lst.begin(); it != lst.end(); ++it)
            if (KMimeType::findByURL(KUrl(*it))->name() == "inode/directory")
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
        if (args->isSet("X") != 0)
        {
            if (!directoryInside)
                kgpg_applet->w->shredDroppedFile();
            else
                KMessageBox::sorry(0, i18n("Cannot shred folder."));
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
