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


#include <qlabel.h>
#include <qpixmap.h>
#include <qclipboard.h>
#include <qfile.h>
#include <qpopupmenu.h>
#include <qwidget.h>

#include <kglobal.h>
#include <kdeversion.h>
#include <klocale.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kcombobox.h>
#include <kwin.h>
#include <kprocess.h>
#include <kprocio.h>
#include <kaboutapplication.h>
#include <kaction.h>
#include <kurlrequester.h>
#include <ktip.h>
#include <kurldrag.h>
#include <kdebug.h>
#include <ktar.h>
#include <kzip.h>

#include "kgpg.h"
#include "kgpgsettings.h"


MyView::MyView( QWidget *parent, const char *name )
                : QLabel( parent, name )
{
        setBackgroundMode(  X11ParentRelative );

	KAction *saveDecrypt=new KAction(i18n("&Decrypt && Save File"),"decrypted",0,this, SLOT(decryptDroppedFile()),this,"decrypt_file");
        KAction *showDecrypt=new KAction(i18n("&Show Decrypted File"),"edit",0,this, SLOT(showDroppedFile()),this,"show_file");
        KAction *encrypt=new KAction(i18n("&Encrypt File"),"encrypted",0,this, SLOT(encryptDroppedFile()),this,"encrypt_file");
        KAction *sign=new KAction(i18n("&Sign File"), "signature",0,this, SLOT(signDroppedFile()),this,"sign_file");
        //QToolTip::add(this,i18n("KGpg drag & drop encryption applet"));

        readOptions();

        setPixmap( KSystemTray::loadIcon("kgpg"));
        resize(24,24);
        setAcceptDrops(true);

        droppopup=new QPopupMenu();
        showDecrypt->plug(droppopup);
        saveDecrypt->plug(droppopup);

        udroppopup=new QPopupMenu();
        encrypt->plug(udroppopup);
        sign->plug(udroppopup);
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
        popupPublic *dialoguec=new popupPublic(0, "public_keys", 0,false);
        connect(dialoguec,SIGNAL(selectedKey(QStringList,QStringList,bool,bool)),this,SLOT(encryptClipboard(QStringList,QStringList,bool,bool)));
        dialoguec->exec();
        delete dialoguec;
}

void  MyView::clipDecrypt()
{
        QString clippie=kapp->clipboard()->text(clipboardMode).stripWhiteSpace();
        if (clippie.startsWith("-----BEGIN PGP MESSAGE")) {
                KgpgApp *kgpgtxtedit = new KgpgApp(0, "editor",WDestructiveClose);
                kgpgtxtedit->view->editor->setText(clippie);
                kgpgtxtedit->view->slotdecode();
                kgpgtxtedit->show();
        } else
                KMessageBox::sorry(this,i18n("No encrypted text found."));
}

void MyView::encryptDroppedFolder()
{
	compressionScheme=0;
        kgpgfoldertmp=new KTempFile(QString::null);
        kgpgfoldertmp->setAutoDelete(true);
        if (KMessageBox::warningContinueCancel(0,i18n("<qt>KGpg will now create a temporary archive file:<br><b>%1</b> to process the encryption. The file will be deleted after the encryption is finished.</qt>").arg(kgpgfoldertmp->name()),i18n("Temporary File Creation"),KStdGuiItem::cont(),"FolderTmpFile")==KMessageBox::Cancel)
                return;

	dialogue=new popupPublic(0,"Public keys",droppedUrls.first().filename(),true);

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
                return;
        	}
        arch->addLocalDirectory (droppedUrls.first().path(),droppedUrls.first().filename());
        arch->close();

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


void  MyView::encryptDroppedFile()
{
        QStringList opts;
        KgpgLibrary *lib=new KgpgLibrary(KGpgSettings::pgpExtension());
        if (KGpgSettings::encryptFilesTo()) {
                if (KGpgSettings::allowUntrustedKeys())
                        opts<<"--always-trust";
                if (KGpgSettings::asciiArmor())
                        opts<<"--armor";
                if (KGpgSettings::hideUserID())
                        opts<<"--throw-keyid";
                if (KGpgSettings::pgpCompatibility())
                        opts<<"--pgp6";
                lib->slotFileEnc(droppedUrls,opts,KGpgSettings::fileEncryptionKey().left(8));
        } else
                lib->slotFileEnc(droppedUrls);
}


void  MyView::shredDroppedFile()
{
        if (KMessageBox::warningContinueCancelList(0,i18n("Do you really want to shred these files?"),droppedUrls.toStringList())!=KMessageBox::Continue)
                return;
        KURL::List::iterator it;
        for ( it = droppedUrls.begin(); it != droppedUrls.end(); ++it ) {

	KgpgLibrary *lib=new KgpgLibrary();
        lib->shredprocessenc(KURL(*it));
        }
}


void  MyView::slotVerifyFile()
{
        ///////////////////////////////////   check file signature
        if (droppedUrl.isEmpty())
                return;

        QString sigfile=QString::null;
        //////////////////////////////////////       try to find detached signature.
        if (!droppedUrl.filename().endsWith(".sig")) {
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
        //bool isFolder=false;
        KURL swapname;

        if (!droppedUrl.isLocalFile()) {
                showDroppedFile();
                return;
        }
        QString oldname=droppedUrl.filename();
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
                swapname=KURL(droppedUrl.directory(0,0)+oldname);
                QFile fgpg(swapname.path());
                if (fgpg.exists()) {
	    KIO::RenameDlg *over=new KIO::RenameDlg(0,i18n("File Already Exists"),QString::null,swapname.path(),KIO::M_OVERWRITE);
	    if (over->exec()==QDialog::Rejected)
	    {
                delete over;
                return;
            }
	    swapname=over->newDestURL();
	    delete over;
                }
        }
        KgpgLibrary *lib=new KgpgLibrary();
        lib->slotFileDec(droppedUrl,swapname,KGpgSettings::customDecrypt());
//        if (isFolder)
  //              connect(lib,SIGNAL(decryptionOver()),this,SLOT(unArchive()));
	connect(lib,SIGNAL(importOver(QStringList)),this,SIGNAL(importedKeys(QStringList)));
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
kdDebug()<<"------Show dropped file"<<endl;
        KgpgApp *kgpgtxtedit = new KgpgApp(0, "editor",WDestructiveClose);
        kgpgtxtedit->view->editor->slotDroppedFile(droppedUrl);
	connect(kgpgtxtedit,SIGNAL(refreshImported(QStringList)),this,SIGNAL(importedKeys(QStringList)));
	connect(kgpgtxtedit->view->editor,SIGNAL(refreshImported(QStringList)),this,SIGNAL(importedKeys(QStringList)));
        kgpgtxtedit->show();
}


void  MyView::droppedfile (KURL::List url)
{
        droppedUrls=url;
        droppedUrl=url.first();
        if (KMimeType::findByURL(droppedUrl)->name()=="inode/directory") {
                KMessageBox::sorry(0,i18n("Sorry, only file operations are currently supported."));
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
			kdDebug()<<"Drop menu--------"<<endl;
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


void  MyView::droppedtext (QString inputText)
{

        QApplication::clipboard()->setText(inputText,clipboardMode);
        if (inputText.startsWith("-----BEGIN PGP MESSAGE")) {
                clipDecrypt();
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
        clipEncrypt();
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
                droppedtext(text);
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
                        if (KMessageBox::questionYesNo(0,"<qt>You did not set a path to your GnuPG config file.<br>This may bring some surprising results in KGpg's execution.<br>Would you like to start KGpg's Wizard to fix this problem ?</qt>")==KMessageBox::Yes)
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


void  MyView::startWizard()
{
        kdDebug()<<"Starting Wizard"<<endl;
        wiz=new KgpgWizard(0,"wizard");
        QString confPath=QDir::homeDirPath()+"/.gnupg/options";
        if (!QFile(confPath).exists()) {
                confPath=QDir::homeDirPath()+"/.gnupg/gpg.conf";
                if (!QFile(confPath).exists()) {
                        if (KMessageBox::questionYesNo(this,i18n("<qt><b>The GnuPG configuration file was not found</b>. Please make sure you have GnuPG installed. Should KGpg try to create a config file ?</qt>"))==KMessageBox::Yes) {
                                confPath=QDir::homeDirPath()+"/.gnupg/options";
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
        wiz->kURLRequester2->setURL(QString(QDir::homeDirPath()+"/Desktop"));
        wiz->kURLRequester2->setMode(2);

        FILE *fp,*fp2;
        QString tst,tst2,name,trustedvals="idre-";
        QString firstKey=QString::null;
        char line[300];
        bool counter=false;
	
        fp = popen("gpg --no-tty --with-colon --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=line;
                if (tst.startsWith("sec")) {
                        name=KgpgInterface::checkForUtf8(tst.section(':',9,9));
                        if (!name.isEmpty()) {
                                fp2 = popen("gpg --no-tty --with-colon --list-keys "+QFile::encodeName(tst.section(':',4,4)), "r");
                                while ( fgets( line, sizeof(line), fp2)) {
                                        tst2=line;
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
                fp = popen("gpg --no-tty --with-colon --list-secret-keys "+QFile::encodeName(defaultID), "r");
                while ( fgets( line, sizeof(line), fp)) {
                        tst=line;
                        if (tst.startsWith("sec")) {
				name=KgpgInterface::checkForUtf8(tst.section(':',9,9));
                                wiz->CBdefault->setCurrentItem(tst.section(':',4,4).right(8)+": "+name);
                        }
                }
                pclose(fp);
        }
}


void  MyView::slotSaveOptionsPath()
{
qWarning("Save wizard settings...");
        if (wiz->checkBox1->isChecked()) {
                KURL path;
                path.addPath(wiz->kURLRequester2->url());
                path.adjustPath(1);
                path.setFileName("shredder.desktop");
                KDesktopFile configl2(path.path(), false);
                if (configl2.isImmutable() ==false) {
                        configl2.setGroup("Desktop Entry");
                        configl2.writeEntry("Type", "Application");
                        configl2.writeEntry("Name",i18n("Shredder"));
                        configl2.writeEntry("Icon","shredder");
                        configl2.writeEntry("Exec","kgpg -X %U");
                }
        }

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
        KAction *KgpgEncryptClipboard = new KAction(i18n("&Encrypt Clipboard"), 0, 0,w, SLOT(clipEncrypt()),actionCollection(),"clip_encrypt");
        KAction *KgpgDecryptClipboard = new KAction(i18n("&Decrypt Clipboard"), 0, 0,w, SLOT(clipDecrypt()),actionCollection(),"clip_decrypt");
        KAction *KgpgOpenEditor = new KAction(i18n("&Open Editor"), "edit", 0,parent, SLOT(slotOpenEditor()),actionCollection(),"kgpg_editor");
	KAction *KgpgOpenServer = new KAction(i18n("&Key Server Dialog"), "network", 0,parent, SLOT(keyserver()),actionCollection(),"kgpg_server");
        KAction *KgpgPreferences=KStdAction::preferences(parent, SLOT(slotOptions()), actionCollection());
        KgpgEncryptClipboard->plug(conf_menu);
        KgpgDecryptClipboard->plug(conf_menu);
        KgpgOpenEditor->plug(conf_menu);
	KgpgOpenServer->plug(conf_menu);
        conf_menu->insertSeparator();
        KgpgPreferences->plug(conf_menu);
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
        KGpgSettings::setShowToolbar(!s_keyManager->toolBar()->isHidden());
        KGpgSettings::setPhotoProperties(s_keyManager->photoProps->currentItem());
	KGpgSettings::setShowTrust(s_keyManager->sTrust->isChecked());
	KGpgSettings::setShowExpi(s_keyManager->sExpi->isChecked());
	KGpgSettings::setShowCreat(s_keyManager->sCreat->isChecked());
	KGpgSettings::setShowSize(s_keyManager->sSize->isChecked());
        KGpgSettings::writeConfig();
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
        kdDebug()<<"New instance"<<endl;
        args = KCmdLineArgs::parsedArgs();
        if (running) {
                kdDebug()<<"Already running"<<endl;
                kgpg_applet->show();
        } else {
                kdDebug() << "Starting KGpg"<<endl;
                running=true;
                s_keyManager=new listKeys(0, "key_manager");
                s_keyManager->refreshkey();
                kgpg_applet=new kgpgapplet(s_keyManager,"kgpg_systrayapplet");
                connect( kgpg_applet, SIGNAL(quitSelected()), this, SLOT(slotHandleQuit()));
                connect(s_keyManager,SIGNAL(readAgainOptions()),kgpg_applet->w,SLOT(readOptions()));
                connect(kgpg_applet->w,SIGNAL(updateDefault(QString)),this,SLOT(wizardOver(QString)));
                connect(kgpg_applet->w,SIGNAL(createNewKey()),s_keyManager,SLOT(slotgenkey()));
		connect(kgpg_applet->w,SIGNAL(importedKeys(QStringList)),s_keyManager->keysList2,SLOT(slotReloadKeys(QStringList)));
                kgpg_applet->show();
                QString gpgPath= KGpgSettings::gpgConfigPath();

                if (!gpgPath.isEmpty()) {
                        if ((KgpgInterface::getGpgBoolSetting("use-agent",gpgPath)) && (!getenv("GPG_AGENT_INFO")))
                                KMessageBox::sorry(0,i18n("<qt>The use of <b>GnuPG Agent</b> is enabled in GnuPG's configuration file (%1).<br>"
                                                          "However, the agent does not seem to run. This could result in problems with signing/decryption.<br>"
                                                          "Please disable GnuPG Agent from KGpg settings, or fix the agent.</qt>").arg(gpgPath));
                }

        }

        ////////////////////////   parsing of command line args
        if (args->isSet("k")!=0) {
                s_keyManager->show();
                KWin::setOnDesktop( s_keyManager->winId() , KWin::currentDesktop() );  //set on the current desktop
                KWin::deIconifyWindow( s_keyManager->winId());  //de-iconify window
                s_keyManager->raise();  // set on top
        } else
                if (args->count()>0) {
                        kdDebug() << "KGpg: found files"<<endl;

                        urlList.clear();

                        for (int ct=0;ct<args->count();ct++)
                                urlList.append(args->url(ct));

                        if (urlList.empty())
                                return 0;

                        kgpg_applet->w->droppedUrl=urlList.first();

                        bool directoryInside=false;
                        QStringList lst=urlList.toStringList();
                        for ( QStringList::Iterator it = lst.begin(); it != lst.end(); ++it ) {
                                if (KMimeType::findByURL(*it)->name()=="inode/directory")
                                        directoryInside=true;
                        }

                        if ((directoryInside) && (lst.count()>1)) {
                                KMessageBox::sorry(0,i18n("Sorry, cannot perform requested operation.\nPlease select only one folder, or several files, but do not mix files and folders."));
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
                                        KMessageBox::sorry(0,i18n("Cannot decrypt & show folder."));
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
                        } else if (kgpg_applet->w->droppedUrl.filename().endsWith(".sig"))
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


