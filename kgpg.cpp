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

#include <qlabel.h>
#include <qclipboard.h>
#include <qfile.h>
#include <qpopupmenu.h>
#include <qwidget.h>
#include <qcheckbox.h>
#include <qmovie.h>
#include <qcstring.h>
#include <qhbuttongroup.h>
#include <kglobal.h>
#include <kactivelabel.h>
#include <kdeversion.h>
#include <klocale.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kcombobox.h>
#include <qregexp.h>
#include <kcmdlineargs.h>
#include <qtooltip.h>
#include <kdebug.h>
#include <kurlrequesterdlg.h>
#include <klineedit.h>
#include <kio/renamedlg.h>
#include <kpassivepopup.h>
#include <qlayout.h>
#include <qbuttongroup.h>
#include <kiconloader.h>
#include <ktempfile.h>
#include <kwin.h>
#include <kprocess.h>
#include <kprocio.h>
#include <kaboutapplication.h>
#include <kurlrequester.h>
#include <ktip.h>
#include <kurldrag.h>
#include <ktar.h>
#include <kzip.h>
#include <dcopclient.h>
#include <kstandarddirs.h>
#include <kfiledialog.h>
#include <kpopupmenu.h>
#include <qcursor.h>
#include <kdesktopfile.h>

#include "kgpgeditor.h"

#include "kgpg.h"
#include "kgpgsettings.h"
#include "listkeys.h"
#include "keyserver.h"
#include "keyservers.h"
#include "popuppublic.h"
#include "kgpgview.h"
#include "kgpglibrary.h"
#include "kgpgwizard.h"

MyView::MyView( QWidget *parent, const char *name )
                : QLabel( parent, name )
{
        setBackgroundMode( X11ParentRelative );
	openTasks=0;

	KAction *saveDecrypt=new KAction(i18n("&Decrypt && Save File"),"decrypted",0,this, SLOT(decryptDroppedFile()),this,"decrypt_file");
        KAction *showDecrypt=new KAction(i18n("&Show Decrypted File"),"edit",0,this, SLOT(showDroppedFile()),this,"show_file");
        KAction *encrypt=new KAction(i18n("&Encrypt File"),"encrypted",0,this, SLOT(encryptDroppedFile()),this,"encrypt_file");
        KAction *sign=new KAction(i18n("&Sign File"), "signature",0,this, SLOT(signDroppedFile()),this,"sign_file");
        //QToolTip::add(this,i18n("KGpg drag & drop encryption applet"));

        readOptions();
	resize(24,24);
	setPixmap( KSystemTray::loadIcon("kgpg_docked"));
        setAcceptDrops(true);

        droppopup=new QPopupMenu();
        showDecrypt->plug(droppopup);
        saveDecrypt->plug(droppopup);

        udroppopup=new QPopupMenu();
        encrypt->plug(udroppopup);
        sign->plug(udroppopup);
	QToolTip::add(this, i18n("KGpg - encryption tool"));
}

MyView::~MyView()
{

        delete droppopup;
        droppopup = 0;
        delete udroppopup;
        udroppopup = 0;
}


void  MyView::clipEncrypt()
{
        popupPublic *dialoguec=new popupPublic(0, "public_keys", 0,false,goDefaultKey);
        connect(dialoguec,SIGNAL(selectedKey(QStringList,QStringList,bool,bool)),this,SLOT(encryptClipboard(QStringList,QStringList,bool,bool)));
        dialoguec->exec();
        delete dialoguec;
}

void  MyView::clipDecrypt()
{
        QString clippie=kapp->clipboard()->text(clipboardMode).stripWhiteSpace();
	droppedtext(clippie,false);
}

void  MyView::clipSign(bool openEditor)
{
        QString clippie=kapp->clipboard()->text(clipboardMode).stripWhiteSpace();
        if (!clippie.isEmpty()) {
                KgpgApp *kgpgtxtedit = new KgpgApp(0, "editor",WDestructiveClose,goDefaultKey);
		connect(this,SIGNAL(setFont(QFont)),kgpgtxtedit,SLOT(slotSetFont(QFont)));
		connect(kgpgtxtedit,SIGNAL(encryptFiles(KURL::List)),this,SLOT(encryptFiles(KURL::List)));
		if (!openEditor)
		connect(kgpgtxtedit->view,SIGNAL(verifyFinished()),kgpgtxtedit,SLOT(closeWindow()));
                kgpgtxtedit->view->editor->setText(clippie);
                kgpgtxtedit->view->clearSign();
		kgpgtxtedit->show();

        } else
                KMessageBox::sorry(this,i18n("Clipboard is empty."));
}

void MyView::encryptDroppedFolder()
{
	compressionScheme=0;
        kgpgfoldertmp=new KTempFile(QString::null);
        kgpgfoldertmp->setAutoDelete(true);
        if (KMessageBox::warningContinueCancel(0,i18n("<qt>KGpg will now create a temporary archive file:<br><b>%1</b> to process the encryption. The file will be deleted after the encryption is finished.</qt>").arg(kgpgfoldertmp->name()),i18n("Temporary File Creation"),KStdGuiItem::cont(),"FolderTmpFile")==KMessageBox::Cancel)
                return;

	dialogue=new popupPublic(0,"Public keys",droppedUrls.first().fileName(),true,goDefaultKey);

	QHButtonGroup *bGroup = new QHButtonGroup(dialogue->plainPage());
                (void) new QLabel(i18n("Compression method for archive:"),bGroup);
                KComboBox *optionbx=new KComboBox(bGroup);
		optionbx->insertItem(i18n("Zip"));
		optionbx->insertItem(i18n("Gzip"));
		optionbx->insertItem(i18n("Bzip2"));
	bGroup->show();
	connect(dialogue,SIGNAL(keyListFilled ()),dialogue,SLOT(slotSetVisible()));
	connect(optionbx,SIGNAL(activated (int)),this,SLOT(slotSetCompression(int)));
        connect(dialogue,SIGNAL(selectedKey(QStringList,QStringList,bool,bool)),this,SLOT(startFolderEncode(QStringList,QStringList,bool,bool)));
        dialogue->CBshred->setEnabled(false);
        dialogue->exec();
	dialogue=0L;
}

void MyView::slotSetCompression(int cp)
{
compressionScheme=cp;
}

void MyView::startFolderEncode(QStringList selec,QStringList encryptOptions,bool ,bool symetric)
{
QString extension;

if (compressionScheme==0)
	extension=".zip";
	else if (compressionScheme==1)
	extension=".tar.gz";
	else
	extension=".tar.bz2";

if (encryptOptions.find("armor")!=encryptOptions.end () )
                extension+=".asc";
        else if (KGpgSettings::pgpExtension())
                extension+=".pgp";
        else
                extension+=".gpg";

KURL encryptedFile(droppedUrls.first().path()+extension);
QFile encryptedFolder(droppedUrls.first().path()+extension);
if (encryptedFolder.exists()) {
			dialogue->hide();
			KIO::RenameDlg *over=new KIO::RenameDlg(0,i18n("File Already Exists"),QString::null,encryptedFile.path(),KIO::M_OVERWRITE);
		    	if (over->exec()==QDialog::Rejected)
	    		{
                	delete over;
                	return;
            		}
	    		encryptedFile=over->newDestURL();
	    		delete over;
			dialogue->show();   /////// strange, but if dialogue is hidden, the passive popup is not displayed...
                }

pop = new KPassivePopup();
	pop->setView(i18n("Processing folder compression and encryption"),i18n("Please wait..."),KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop));
	pop->setAutoDelete(false);
	pop->show();
	kapp->processEvents();
	dialogue->slotAccept();
	dialogue=0L;

	KArchive *arch;
	if (compressionScheme==0)
	arch=new KZip(kgpgfoldertmp->name());
	else if (compressionScheme==1)
	arch=new KTar(kgpgfoldertmp->name(), "application/x-gzip");
	else
	arch=new KTar(kgpgfoldertmp->name(), "application/x-bzip2");

        if (!arch->open( IO_WriteOnly )) {
                KMessageBox::sorry(0,i18n("Unable to create temporary file"));
		delete arch;
                return;
        }
        arch->addLocalDirectory (droppedUrls.first().path(),droppedUrls.first().fileName());
        arch->close();
        delete arch;

        KgpgInterface *folderprocess=new KgpgInterface();
        folderprocess->KgpgEncryptFile(selec,KURL(kgpgfoldertmp->name()),encryptedFile,encryptOptions,symetric);
        connect(folderprocess,SIGNAL(encryptionfinished(KURL)),this,SLOT(slotFolderFinished(KURL)));
        connect(folderprocess,SIGNAL(errormessage(QString)),this,SLOT(slotFolderFinishedError(QString)));
}

void  MyView::slotFolderFinished(KURL)
{
        delete pop;
        delete kgpgfoldertmp;
}

void  MyView::slotFolderFinishedError(QString errmsge)
{
        delete pop;
        delete kgpgfoldertmp;
        KMessageBox::sorry(0,errmsge);
}

void MyView::busyMessage(QString mssge,bool reset)
{
if (reset) openTasks=0;
if (!mssge.isEmpty())
{
openTasks++;
QToolTip::remove(this);
QToolTip::add(this, mssge);
setMovie(QMovie(locate("appdata","pics/kgpg_docked.gif")));
}
else openTasks--;

//kdDebug(2100) << "Emit message: "<<openTasks<<endl;

if (openTasks<=0)
{
setPixmap( KSystemTray::loadIcon("kgpg_docked"));
QToolTip::remove(this);
QToolTip::add(this, i18n("KGpg - encryption tool"));
openTasks=0;
}
}

void  MyView::encryptDroppedFile()
{
        QStringList opts;
        KgpgLibrary *lib=new KgpgLibrary(this,KGpgSettings::pgpExtension());
	connect(lib,SIGNAL(systemMessage(QString,bool)),this,SLOT(busyMessage(QString,bool)));
        if (KGpgSettings::fileKey()!=QString::null) {
                if (KGpgSettings::allowUntrustedKeys())
                        opts<<"--always-trust";
                if (KGpgSettings::asciiArmor())
                        opts<<"--armor";
                if (KGpgSettings::hideUserID())
                        opts<<"--throw-keyid";
                if (KGpgSettings::pgpCompatibility())
                        opts<<"--pgp6";
                lib->slotFileEnc(droppedUrls,opts,QStringList::split(" ",KGpgSettings::fileKey()),goDefaultKey);
        } else
                lib->slotFileEnc(droppedUrls,QString::null,QString::null,goDefaultKey);
}

void  MyView::encryptFiles(KURL::List urls)
{
droppedUrls=urls;
encryptDroppedFile();
}

void  MyView::shredDroppedFile()
{
KDialogBase *shredConfirm=new KDialogBase( this, "confirm_shred", true,i18n("Shred Files"),KDialogBase::Ok | KDialogBase::Cancel);
QWidget *page = new QWidget(shredConfirm);
shredConfirm->setMainWidget(page);
QBoxLayout *layout=new QBoxLayout(page,QBoxLayout::TopToBottom,0);
layout->setAutoAdd(true);

(void) new KActiveLabel( i18n("Do you really want to <a href=\"whatsthis:%1\">shred</a> these files?").arg(i18n( "<qt><p>You must be aware that <b>shredding is not secure</b> on all file systems, and that parts of the file may have been saved in a temporary file or in the spooler of your printer if you previously opened it in an editor or tried to print it. Only works on files (not on folders).</p></qt>")),page);
KListBox *lb=new KListBox(page);
lb->insertStringList(droppedUrls.toStringList());
if (shredConfirm->exec()==QDialog::Accepted)
	{
	KgpgLibrary *lib=new KgpgLibrary(this);
	connect(lib,SIGNAL(systemMessage(QString,bool)),this,SLOT(busyMessage(QString,bool)));
	lib->shredprocessenc(droppedUrls);
	}
delete shredConfirm;
}


void  MyView::slotVerifyFile()
{
        ///////////////////////////////////   check file signature
        if (droppedUrl.isEmpty())
                return;

        QString sigfile=QString::null;
        //////////////////////////////////////       try to find detached signature.
        if (!droppedUrl.fileName().endsWith(".sig")) {
                sigfile=droppedUrl.path()+".sig";
                QFile fsig(sigfile);
                if (!fsig.exists()) {
                        sigfile=droppedUrl.path()+".asc";
                        QFile fsig(sigfile);
                        //////////////   if no .asc or .sig signature file included, assume the file is internally signed
                        if (!fsig.exists())
                                sigfile=QString::null;
                }
        } else {
                sigfile=droppedUrl.path();
                droppedUrl=KURL(sigfile.left(sigfile.length()-4));
        }

        ///////////////////////// pipe gpg command
        KgpgInterface *verifyFileProcess=new KgpgInterface();
        verifyFileProcess->KgpgVerifyFile(droppedUrl,KURL(sigfile));
        connect (verifyFileProcess,SIGNAL(verifyquerykey(QString)),this,SLOT(importSignature(QString)));
}

void  MyView::importSignature(QString ID)
{
        keyServer *kser=new keyServer(0,"server_dialog",false);
        kser->page->kLEimportid->setText(ID);
        kser->slotImport();
}

void  MyView::signDroppedFile()
{
        //////////////////////////////////////   create a detached signature for a chosen file
        if (droppedUrl.isEmpty())
                return;

        QString signKeyID;
        //////////////////   select a private key to sign file --> listkeys.cpp
        KgpgSelKey *opts=new KgpgSelKey(0,"select_secret");
        if (opts->exec()==QDialog::Accepted)
                signKeyID=opts->getkeyID();
        else {
                delete opts;
                return;
        }
        delete opts;
        QStringList Options;
        if (KGpgSettings::asciiArmor())
                Options<<"--armor";
        if (KGpgSettings::pgpCompatibility())
                Options<<"--pgp6";
        KgpgInterface *signFileProcess=new KgpgInterface();
        signFileProcess->KgpgSignFile(signKeyID,droppedUrl,Options);
}

void  MyView::decryptDroppedFile()
{
        //bool isFolder=false;  // droppedUrls
        KURL swapname;

        if (!droppedUrls.first().isLocalFile()) {
                showDroppedFile();
                decryptNextFile();
        }

        QString oldname=droppedUrls.first().fileName();
        if (oldname.endsWith(".gpg") || oldname.endsWith(".asc") || oldname.endsWith(".pgp"))
                oldname.truncate(oldname.length()-4);
        else
                oldname.append(".clear");
	/*
        if (oldname.endsWith(".tar.gz")) {
                isFolder=true;
                kgpgFolderExtract=new KTempFile(QString::null,".tar.gz");
                kgpgFolderExtract->setAutoDelete(true);
                swapname=KURL(kgpgFolderExtract->name());
                if (KMessageBox::warningContinueCancel(0,i18n("<qt>The file to decrypt is an archive. KGpg will create a temporary unencrypted archive file:<br><b>%1</b> before processing the archive extraction. This temporary file will be deleted after the decryption is finished.</qt>").arg(kgpgFolderExtract->name()),i18n("Temporary File Creation"),KStdGuiItem::cont(),"FolderTmpDecFile")==KMessageBox::Cancel)
                        return;
        } else*/ {
                swapname=KURL(droppedUrls.first().directory(0,0)+oldname);
                QFile fgpg(swapname.path());
                if (fgpg.exists()) {
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
        lib->slotFileDec(droppedUrls.first(),swapname,KGpgSettings::customDecrypt());
	connect(lib,SIGNAL(importOver(QStringList)),this,SIGNAL(importedKeys(QStringList)));
	connect(lib,SIGNAL(systemMessage(QString,bool)),this,SLOT(busyMessage(QString,bool)));
//        if (isFolder)
        connect(lib,SIGNAL(decryptionOver()),this,SLOT(decryptNextFile()));

}

void  MyView::decryptNextFile()
{
if (droppedUrls.count()>1)
{
droppedUrls.pop_front();
decryptDroppedFile();
}
}

void  MyView::unArchive()
{
        KTar compressedFolder(kgpgFolderExtract->name(),"application/x-gzip");
        if (!compressedFolder.open( IO_ReadOnly )) {
                KMessageBox::sorry(0,i18n("Unable to read temporary archive file"));
                return;
        }
        const KArchiveDirectory *archiveDirectory=compressedFolder.directory();
        //KURL savePath=KURL::getURL(droppedUrl,this,i18n(""));
        KURLRequesterDlg *savePath=new KURLRequesterDlg(droppedUrl.directory(false),i18n("Extract to: "),0,"extract");
        savePath->fileDialog()->setMode(KFile::Directory);
        if (!savePath->exec()==QDialog::Accepted) {
                delete kgpgFolderExtract;
                return;
        }
        archiveDirectory->KArchiveDirectory::copyTo(savePath->selectedURL().path());
        compressedFolder.close();
        delete savePath;
        delete kgpgFolderExtract;
}


void  MyView::showDroppedFile()
{
kdDebug(2100)<<"------Show dropped file"<<endl;
        KgpgApp *kgpgtxtedit = new KgpgApp(0, "editor",WDestructiveClose,goDefaultKey);
        kgpgtxtedit->view->editor->slotDroppedFile(droppedUrls.first());
	connect(kgpgtxtedit,SIGNAL(encryptFiles(KURL::List)),this,SLOT(encryptFiles(KURL::List)));
	connect(this,SIGNAL(setFont(QFont)),kgpgtxtedit,SLOT(slotSetFont(QFont)));
	connect(kgpgtxtedit,SIGNAL(refreshImported(QStringList)),this,SIGNAL(importedKeys(QStringList)));
	connect(kgpgtxtedit->view->editor,SIGNAL(refreshImported(QStringList)),this,SIGNAL(importedKeys(QStringList)));
        kgpgtxtedit->show();
}


void  MyView::droppedfile (KURL::List url)
{
        droppedUrls=url;
        droppedUrl=url.first();
        if (KMimeType::findByURL(droppedUrl)->name()=="inode/directory") {
		encryptDroppedFolder();
                //KMessageBox::sorry(0,i18n("Sorry, only file operations are currently supported."));
                return;
        }
        if (!droppedUrl.isLocalFile()) {
                showDroppedFile();
                return;
        }

        if ((droppedUrl.path().endsWith(".asc")) || (droppedUrl.path().endsWith(".pgp")) || (droppedUrl.path().endsWith(".gpg"))) {
                switch (KGpgSettings::encryptedDropEvent()) {
                case KGpgSettings::EnumEncryptedDropEvent::DecryptAndSave:
                        decryptDroppedFile();
                        break;
                case KGpgSettings::EnumEncryptedDropEvent::DecryptAndOpen:
                        showDroppedFile();
                        break;
                case KGpgSettings::EnumEncryptedDropEvent::Ask:
                        droppopup->exec(QCursor::pos ());
			kdDebug(2100)<<"Drop menu--------"<<endl;
                        break;
                }
        } else if (droppedUrl.path().endsWith(".sig")) {
                slotVerifyFile();
        } else
                switch (KGpgSettings::unencryptedDropEvent()) {
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


void  MyView::droppedtext (QString inputText,bool allowEncrypt)
{

        if (inputText.startsWith("-----BEGIN PGP MESSAGE")) {
                KgpgApp *kgpgtxtedit = new KgpgApp(0, "editor",WDestructiveClose,goDefaultKey);
		connect(kgpgtxtedit,SIGNAL(encryptFiles(KURL::List)),this,SLOT(encryptFiles(KURL::List)));
		connect(this,SIGNAL(setFont(QFont)),kgpgtxtedit,SLOT(slotSetFont(QFont)));
                kgpgtxtedit->view->editor->setText(inputText);
                kgpgtxtedit->view->slotdecode();
                kgpgtxtedit->show();
                return;
        }
        if (inputText.startsWith("-----BEGIN PGP PUBLIC KEY")) {
                int result=KMessageBox::warningContinueCancel(0,i18n("<p>The dropped text is a public key.<br>Do you want to import it ?</p>"),i18n("Warning"));
                if (result==KMessageBox::Cancel)
                        return;
                else {
                        KgpgInterface *importKeyProcess=new KgpgInterface();
                        importKeyProcess->importKey(inputText);
			connect(importKeyProcess,SIGNAL(importfinished(QStringList)),this,SIGNAL(importedKeys(QStringList)));
                        return;
                }
        }
	        if (inputText.startsWith("-----BEGIN PGP SIGNED MESSAGE")) {
                clipSign(false);
                return;
        }
        if (allowEncrypt) clipEncrypt();
	else KMessageBox::sorry(this,i18n("No encrypted text found."));
}


void  MyView::dragEnterEvent(QDragEnterEvent *e)
{
        e->accept (KURLDrag::canDecode(e) || QTextDrag::canDecode (e));
}


void  MyView::dropEvent (QDropEvent *o)
{
        KURL::List list;
        QString text;
        if ( KURLDrag::decode( o, list ) )
                droppedfile(list);
        else if ( QTextDrag::decode(o, text) )
		{
	        QApplication::clipboard()->setText(text,clipboardMode);
                droppedtext(text);
		}
}


void  MyView::readOptions()
{
	clipboardMode=QClipboard::Clipboard;
        if ( KGpgSettings::useMouseSelection() && kapp->clipboard()->supportsSelection())
                clipboardMode=QClipboard::Selection;

	if (KGpgSettings::firstRun()) {
                firstRun();
        } else {
                QString path = KGpgSettings::gpgConfigPath();
                if (path.isEmpty()) {
                        if (KMessageBox::questionYesNo(0,i18n("<qt>You have not set a path to your GnuPG config file.<br>This may cause some surprising results in KGpg's execution.<br>Would you like to start KGpg's Wizard to fix this problem?</qt>"),QString::null,i18n("Start Wizard"),i18n("Do Not Start"))==KMessageBox::Yes)
                                startWizard();
                } else {
                        QStringList groups=KgpgInterface::getGpgGroupNames(path);
                        if (!groups.isEmpty())
                                KGpgSettings::setGroups(groups.join(","));
                }
        }
}


void  MyView::firstRun()
{
        KProcIO *p=new KProcIO();
        *p<<"gpg"<<"--no-tty"<<"--list-secret-keys";
        p->start(KProcess::Block);  ////  start gnupg so that it will create a config file
        startWizard();
}


static QString getGpgHome()
{
    char    *env=getenv("GNUPGHOME");
    QString gpgHome(env ? env : QDir::homeDirPath()+"/.gnupg/");

    gpgHome.replace("//", "/");

    if(!gpgHome.endsWith("/"))
        gpgHome.append('/');

    KStandardDirs::makeDir(gpgHome, 0700);
    return gpgHome;
}


void  MyView::startWizard()
{
        kdDebug(2100)<<"Starting Wizard"<<endl;
        wiz=new KgpgWizard(0,"wizard");
        QString gpgHome(getGpgHome());
        QString confPath=gpgHome+"options";
        if (!QFile(confPath).exists()) {
                confPath=gpgHome+"gpg.conf";
                if (!QFile(confPath).exists()) {
                        if (KMessageBox::questionYesNo(this,i18n("<qt><b>The GnuPG configuration file was not found</b>. Please make sure you have GnuPG installed. Should KGpg try to create a config file ?</qt>"),QString::null,i18n("Create Config"),i18n("Do Not Create"))==KMessageBox::Yes) {
                                confPath=gpgHome+"options";
                                QFile file(confPath);
                                if ( file.open( IO_WriteOnly ) ) {
                                        QTextStream stream( &file );
                                        stream <<"# GnuPG config file created by KGpg"<< "\n";
                                        file.close();
                                }
                        } else {
                                wiz->text_optionsfound->setText(i18n("<qt><b>The GnuPG configuration file was not found</b>. Please make sure you have GnuPG installed and give the path to the config file.</qt>"));
                                confPath=QString::null;
                        }
                }
        }

	int gpgVersion=KgpgInterface::getGpgVersion();
	if (gpgVersion<120) wiz->txtGpgVersion->setText(i18n("Your GnuPG version seems to be older than 1.2.0. Photo Id's and Key Groups will not work properly. Please consider upgrading GnuPG (http://gnupg.org)."));
	else wiz->txtGpgVersion->setText(QString::null);

        wiz->kURLRequester1->setURL(confPath);
        /*
	wiz->kURLRequester2->setURL(KGlobalSettings::desktopPath());
        wiz->kURLRequester2->setMode(2);*/

        FILE *fp,*fp2;
        QString tst,tst2,name,trustedvals="idre-";
        QString firstKey=QString::null;
        char line[300];
        bool counter=false;

        fp = popen("gpg --display-charset=utf-8 --no-tty --with-colon --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=QString::fromUtf8(line);
                if (tst.startsWith("sec")) {
                        name=KgpgInterface::checkForUtf8(tst.section(':',9,9));
                        if (!name.isEmpty()) {
                                fp2 = popen("gpg --display-charset=utf-8 --no-tty --with-colon --list-keys "+QFile::encodeName(tst.section(':',4,4)), "r");
                                while ( fgets( line, sizeof(line), fp2)) {
                                        tst2=QString::fromUtf8(line);
                                        if (tst2.startsWith("pub") && (trustedvals.find(tst2.section(':',1,1))==-1)) {
                                                counter=true;
                                                wiz->CBdefault->insertItem(tst.section(':',4,4).right(8)+": "+name);
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
        else {
                wiz->textGenerate->hide();
		wiz->setTitle(wiz->page_4,i18n("Step Three: Select your Default Private Key"));
                connect(wiz->finishButton(),SIGNAL(clicked()),this,SLOT(slotSaveOptionsPath()));
        }
        connect(wiz->nextButton(),SIGNAL(clicked()),this,SLOT(slotWizardChange()));
        connect( wiz , SIGNAL( destroyed() ) , this, SLOT( slotWizardClose()));
	connect(wiz,SIGNAL(helpClicked()),this,SLOT(help()));

        wiz->setFinishEnabled(wiz->page_4,true);
        wiz->show();
}

void  MyView::slotWizardChange()
{
        QString tst,name;
        char line[300];
        FILE *fp;

        if (wiz->indexOf(wiz->currentPage())==2) {
                QString defaultID=KgpgInterface::getGpgSetting("default-key",wiz->kURLRequester1->url());
                if (defaultID.isEmpty())
                        return;
                fp = popen("gpg --display-charset=utf-8 --no-tty --with-colon --list-secret-keys "+QFile::encodeName(defaultID), "r");
                while ( fgets( line, sizeof(line), fp)) {
                        tst=QString::fromUtf8(line);
                        if (tst.startsWith("sec")) {
				name=KgpgInterface::checkForUtf8(tst.section(':',9,9));
                                wiz->CBdefault->setCurrentItem(tst.section(':',4,4).right(8)+": "+name);
                        }
                }
                pclose(fp);
        }
}

void  MyView::installShred()
{
                KURL path;
		path.setPath(KGlobalSettings::desktopPath());
		path.addPath("shredder.desktop");
                KDesktopFile configl2(path.path(), false);
                if (configl2.isImmutable() ==false) {
                        configl2.setGroup("Desktop Entry");
                        configl2.writeEntry("Type", "Application");
                        configl2.writeEntry("Name",i18n("Shredder"));
                        configl2.writeEntry("Icon","editshred");
                        configl2.writeEntry("Exec","kgpg -X %U");
                }
}

void  MyView::slotSaveOptionsPath()
{
qWarning("Save wizard settings...");
        if (wiz->checkBox1->isChecked()) installShred();

        KGpgSettings::setAutoStart( wiz->checkBox2->isChecked() );

        KGpgSettings::setGpgConfigPath( wiz->kURLRequester1->url() );
        KGpgSettings::setFirstRun( false );

        QString defaultID=wiz->CBdefault->currentText().section(':',0,0);
/*        if (!defaultID.isEmpty()) {
        	KGpgSettings::setDefaultKey(defaultID);
        }*/

        KGpgSettings::writeConfig();

        emit updateDefault("0x"+defaultID);
        if (wiz)
                delete wiz;
}

void  MyView::slotWizardClose()
{
        wiz=0L;
}

void  MyView::slotGenKey()
{
        slotSaveOptionsPath();
        emit createNewKey();
}

void  MyView::about()
{
        KAboutApplication dialog(kapp->aboutData());//_aboutData);
        dialog.exec();
}

void  MyView::help()
{
        kapp->invokeHelp(0,"kgpg");
}

kgpgapplet::kgpgapplet(QWidget *parent, const char *name)
                : KSystemTray(parent,name)
{
        w=new MyView(this);
        w->show();
        KPopupMenu *conf_menu=contextMenu();
        KgpgEncryptClipboard = new KAction(i18n("&Encrypt Clipboard"), "kgpg", 0,w, SLOT(clipEncrypt()),actionCollection(),"clip_encrypt");
        KgpgDecryptClipboard = new KAction(i18n("&Decrypt Clipboard"), 0, 0,w, SLOT(clipDecrypt()),actionCollection(),"clip_decrypt");
	KgpgSignClipboard = new KAction(i18n("&Sign/Verify Clipboard"), "signature", 0,w, SLOT(clipSign()),actionCollection(),"clip_sign");
        KAction *KgpgOpenEditor;
	if (KGpgSettings::leftClick()==KGpgSettings::EnumLeftClick::KeyManager)
	KgpgOpenEditor = new KAction(i18n("&Open Editor"), "edit", 0,parent, SLOT(slotOpenEditor()),actionCollection(),"kgpg_editor");
	else
	KgpgOpenEditor = new KAction(i18n("&Open Key Manager"), "kgpg", 0,this, SLOT(slotOpenKeyManager()),actionCollection(),"kgpg_editor");

	KAction *KgpgOpenServer = new KAction(i18n("&Key Server Dialog"), "network", 0,this, SLOT(slotOpenServerDialog()),actionCollection(),"kgpg_server");
        KAction *KgpgPreferences=KStdAction::preferences(this, SLOT(showOptions()), actionCollection());

	connect (conf_menu,SIGNAL(aboutToShow()),this,SLOT(checkMenu()));

        KgpgEncryptClipboard->plug(conf_menu);
        KgpgDecryptClipboard->plug(conf_menu);
	KgpgSignClipboard->plug(conf_menu);
        KgpgOpenEditor->plug(conf_menu);
	KgpgOpenServer->plug(conf_menu);
        conf_menu->insertSeparator();
        KgpgPreferences->plug(conf_menu);
}


void kgpgapplet::checkMenu()
{
	KgpgDecryptClipboard->setEnabled(false);
	if ((kapp->clipboard()->text(w->clipboardMode).isEmpty()))
	{
		KgpgEncryptClipboard->setEnabled(false);
		KgpgSignClipboard->setEnabled(false);
	}
	else
	{
		KgpgEncryptClipboard->setEnabled(true);
		if (kapp->clipboard()->text(w->clipboardMode).stripWhiteSpace().startsWith("-----BEGIN"))
		KgpgDecryptClipboard->setEnabled(true);
		KgpgSignClipboard->setEnabled(true);
	}
}

void kgpgapplet::showOptions()
{
QByteArray data;
if (!kapp->dcopClient()->send("kgpg", "KeyInterface", "showOptions()",data))
kdDebug(2100) <<"there was some error using DCOP."<<endl;
}

void kgpgapplet::slotOpenKeyManager()
{
QByteArray data;
if (!kapp->dcopClient()->send("kgpg", "KeyInterface", "showKeyManager()",data))
kdDebug(2100) <<"there was some error using DCOP."<<endl;
}

void kgpgapplet::slotOpenServerDialog()
{
QByteArray data;
if (!kapp->dcopClient()->send("kgpg", "KeyInterface", "showKeyServer()",data))
kdDebug(2100) <<"there was some error using DCOP."<<endl;
}



kgpgapplet::~kgpgapplet()
{
        delete w;
        w = 0L;
}

KgpgAppletApp::KgpgAppletApp()
                : KUniqueApplication()//, kgpg_applet( 0 )
{

        running=false;
}


KgpgAppletApp::~KgpgAppletApp()
{
        delete s_keyManager;
        s_keyManager=0L;
        delete kgpg_applet;
        kgpg_applet = 0L;
}

void KgpgAppletApp::slotHandleQuit()
{
s_keyManager->keysList2->saveLayout(KGlobal::config(),"KeyView");
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
        if (defaultKeyId.length()==10)
                s_keyManager->slotSetDefaultKey(defaultKeyId);
        s_keyManager->show();
        s_keyManager->raise();
}

int KgpgAppletApp::newInstance()
{
        kdDebug(2100)<<"New instance"<<endl;
        args = KCmdLineArgs::parsedArgs();
        if (running) {
                kdDebug(2100)<<"Already running"<<endl;
                kgpg_applet->show();
        } else {
                kdDebug(2100) << "Starting KGpg"<<endl;
                running=true;

                s_keyManager=new listKeys(0, "key_manager");

		QString gpgPath= KGpgSettings::gpgConfigPath();
		if (!gpgPath.isEmpty() && KURL(gpgPath).directory(false)!=QDir::homeDirPath()+"/.gnupg/")
		setenv("GNUPGHOME", QFile::encodeName(KURL::fromPathOrURL(gpgPath).directory(false)), 1);

        s_keyManager->refreshkey();

		if (KGpgSettings::leftClick()==KGpgSettings::EnumLeftClick::KeyManager)
                kgpg_applet=new kgpgapplet(s_keyManager,"kgpg_systrayapplet");
		else
		{
		kgpg_applet=new kgpgapplet(s_keyManager->s_kgpgEditor,"kgpg_systrayapplet");
		}
		connect(s_keyManager,SIGNAL(encryptFiles(KURL::List)),kgpg_applet->w,SLOT(encryptFiles(KURL::List)));
		connect(s_keyManager,SIGNAL(installShredder()),kgpg_applet->w,SLOT(installShred()));
		connect(s_keyManager->s_kgpgEditor,SIGNAL(encryptFiles(KURL::List)),kgpg_applet->w,SLOT(encryptFiles(KURL::List)));

                connect( kgpg_applet, SIGNAL(quitSelected()), this, SLOT(slotHandleQuit()));
                connect(s_keyManager,SIGNAL(readAgainOptions()),kgpg_applet->w,SLOT(readOptions()));
                connect(kgpg_applet->w,SIGNAL(updateDefault(QString)),this,SLOT(wizardOver(QString)));
                connect(kgpg_applet->w,SIGNAL(createNewKey()),s_keyManager,SLOT(slotgenkey()));
		connect(s_keyManager,SIGNAL(fontChanged(QFont)),kgpg_applet->w,SIGNAL(setFont(QFont)));
		connect(kgpg_applet->w,SIGNAL(importedKeys(QStringList)),s_keyManager->keysList2,SLOT(slotReloadKeys(QStringList)));
                kgpg_applet->show();


                if (!gpgPath.isEmpty()) {
                        if ((KgpgInterface::getGpgBoolSetting("use-agent",gpgPath)) && (!getenv("GPG_AGENT_INFO")))
                                KMessageBox::sorry(0,i18n("<qt>The use of <b>GnuPG Agent</b> is enabled in GnuPG's configuration file (%1).<br>"
                                                          "However, the agent does not seem to be running. This could result in problems with signing/decryption.<br>"
                                                          "Please disable GnuPG Agent from KGpg settings, or fix the agent.</qt>").arg(gpgPath));
                }

        }
	goHome=s_keyManager->actionCollection()->action("go_default_key")->shortcut();
	kgpg_applet->w->goDefaultKey=goHome;

        ////////////////////////   parsing of command line args
        if (args->isSet("k")!=0) {
                s_keyManager->show();
                KWin::setOnDesktop( s_keyManager->winId() , KWin::currentDesktop() );  //set on the current desktop
                KWin::deIconifyWindow( s_keyManager->winId());  //de-iconify window
                s_keyManager->raise();  // set on top
        } else
                if (args->count()>0) {
                        kdDebug(2100) << "KGpg: found files"<<endl;

                        urlList.clear();

                        for (int ct=0;ct<args->count();ct++)
                                urlList.append(args->url(ct));

                        if (urlList.empty())
                                return 0;

                        kgpg_applet->w->droppedUrl=urlList.first();

                        bool directoryInside=false;
                        QStringList lst=urlList.toStringList();
                        for ( QStringList::Iterator it = lst.begin(); it != lst.end(); ++it ) {
                                if (KMimeType::findByURL(KURL( *it ))->name()=="inode/directory")
                                        directoryInside=true;
                        }

                        if ((directoryInside) && (lst.count()>1)) {
                                KMessageBox::sorry(0,i18n("Unable to perform requested operation.\nPlease select only one folder, or several files, but do not mix files and folders."));
                                return 0;
                        }

                        kgpg_applet->w->droppedUrls=urlList;

                        if (args->isSet("e")!=0) {
                                if (!directoryInside)
                                        kgpg_applet->w->encryptDroppedFile();
                                else
                                        kgpg_applet->w->encryptDroppedFolder();
                        } else if (args->isSet("X")!=0) {
                                if (!directoryInside)
                                        kgpg_applet->w->shredDroppedFile();
                                else
                                        KMessageBox::sorry(0,i18n("Cannot shred folder."));
                        } else if (args->isSet("s")!=0) {
                                if (!directoryInside)
                                        kgpg_applet->w->showDroppedFile();
                                else
                                        KMessageBox::sorry(0,i18n("Cannot decrypt and show folder."));
                        } else if (args->isSet("S")!=0) {
                                if (!directoryInside)
                                        kgpg_applet->w->signDroppedFile();
                                else
                                        KMessageBox::sorry(0,i18n("Cannot sign folder."));
                        } else if (args->isSet("V")!=0) {
                                if (!directoryInside)
                                        kgpg_applet->w->slotVerifyFile();
                                else
                                        KMessageBox::sorry(0,i18n("Cannot verify folder."));
                        } else if (kgpg_applet->w->droppedUrl.fileName().endsWith(".sig"))
                                kgpg_applet->w->slotVerifyFile();
                        else
                                kgpg_applet->w->decryptDroppedFile();
                }
        return 0;
}




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void MyView::encryptClipboard(QStringList selec,QStringList encryptOptions,bool,bool symmetric)
{
        if (kapp->clipboard()->text(clipboardMode).isEmpty()) {
	KPassivePopup::message(i18n("Clipboard is empty."),QString::null,KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop),this);
	return;
	}
                if (KGpgSettings::pgpCompatibility())
                        encryptOptions<<"--pgp6";
                encryptOptions<<"--armor";

		if (symmetric) selec.clear();
                KgpgInterface *txtEncrypt=new KgpgInterface();
                connect (txtEncrypt,SIGNAL(txtencryptionfinished(QString)),this,SLOT(slotSetClip(QString)));
		connect (txtEncrypt,SIGNAL(txtencryptionstarted()),this,SLOT(slotPassiveClip()));
                txtEncrypt->KgpgEncryptText(kapp->clipboard()->text(clipboardMode),selec,encryptOptions);
}

void MyView::slotPassiveClip()
{
QString newtxt=kapp->clipboard()->text(clipboardMode);
if (newtxt.length()>300)
                        newtxt=QString(newtxt.left(250).stripWhiteSpace())+"...\n"+QString(newtxt.right(40).stripWhiteSpace());

                newtxt.replace(QRegExp("<"),"&lt;");   /////   disable html tags
                newtxt.replace(QRegExp("\n"),"<br>");

pop = new KPassivePopup( this);
                pop->setView(i18n("Encrypted following text:"),newtxt,KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop));
                pop->setTimeout(3200);
                pop->show();
                QRect qRect(QApplication::desktop()->screenGeometry());
                int iXpos=qRect.width()/2-pop->width()/2;
                int iYpos=qRect.height()/2-pop->height()/2;
                pop->move(iXpos,iYpos);
}

void MyView::slotSetClip(QString newtxt)
{
        if (newtxt.isEmpty()) return;
                QApplication::clipboard()->setText(newtxt,clipboardMode);//,QClipboard::Clipboard);    QT 3.1 only
}



/////////////////////////////////////////////////////////////////////////////

#include "kgpg.moc"


