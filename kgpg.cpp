/***************************************************************************
                          kgpg.cpp  -  description
                             -------------------
    begin                : Mon Nov 18 2002
    copyright            : (C) 2002 by y0k0
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
#include <qcursor.h>
#include <qwidget.h>
#include <qtooltip.h>
#include <kaboutapplication.h>
#include <qpopupmenu.h>
#include <kaction.h>
#include <kurlrequester.h>
#include <ktip.h>
#include <qregexp.h>
#include <kurldrag.h>
#include <kdebug.h>
#include <ktar.h>

#include "kgpg.h"

MyView::MyView( QWidget *parent, const char *name )
                : QLabel( parent, name )
{
        setBackgroundMode(  X11ParentRelative );

        KAction *saveDecrypt = new KAction(i18n("&Decrypt && Save File"),"decrypted",0,this, SLOT(decryptDroppedFile()),this,"decrypt_file");
        KAction *showDecrypt = new KAction(i18n("&Show Decrypted File"),"edit",0,this, SLOT(showDroppedFile()),this,"show_file");
        KAction *encrypt = new KAction(i18n("&Encrypt File"),"encrypted",0,this, SLOT(encryptDroppedFile()),this,"encrypt_file");
        KAction *sign = new KAction(i18n("&Sign File"), "signature",0,this, SLOT(signDroppedFile()),this,"sign_file");
        //QToolTip::add(this,i18n("KGpg drag & drop encryption applet"));

        ksConfig=kapp->config();
        readOptions();

        if (tipofday)
                KTipDialog::showTip(this, "kgpg/tips", true);

        setPixmap(KGlobal::iconLoader()->loadIcon("kgpg",KIcon::User,22));
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


void MyView::showPopupMenu( QPopupMenu *menu )
{
        Q_ASSERT( menu != 0L );

        menu->move(-1000,-1000);
        menu->show();
        menu->hide();

        QPoint g = QCursor::pos();
        if ( menu->height() < g.y() )
                menu->popup(QPoint( g.x(), g.y() - menu->height()));
        else
                menu->popup(QPoint(g.x(), g.y()));
}


void  MyView::openKeyServer()
{
        if(!m_keyServer) {
                //keyServer *ks
                m_keyServer=new keyServer(0,"server_dialog",false,WDestructiveClose);
                connect( m_keyServer , SIGNAL( destroyed() ) , this, SLOT( slotKeyServerClosed()));
        }
        m_keyServer->show();
        KWin::setOnDesktop( m_keyServer->winId() , KWin::currentDesktop() );  //set on the current desktop
        KWin::deIconifyWindow( m_keyServer->winId());  //de-iconify window
        m_keyServer->raise();  // set on top
}


void  MyView::clipEncrypt()
{
        popupPublic *dialoguec=new popupPublic(this, "public_keys", 0,false);
        connect(dialoguec,SIGNAL(selectedKey(QString &,QString,bool,bool)),this,SLOT(encryptClipboard(QString &,QString)));
        dialoguec->show();
}

void  MyView::clipDecrypt()
{
        QString clippie=kapp->clipboard()->text().stripWhiteSpace();
        if (clippie.startsWith("-----BEGIN PGP MESSAGE")) {
                KgpgApp *kgpgtxtedit = new KgpgApp(0, "editor",WDestructiveClose);
                kgpgtxtedit->view->editor->setText(clippie);
                kgpgtxtedit->view->slotdecode();
                kgpgtxtedit->show();
        } else
                KMessageBox::sorry(this,i18n("No encrypted text found."));
}


void  MyView::openEditor()
{
        KgpgApp *kgpgtxtedit = new KgpgApp(0, "editor",WType_Dialog);
        kgpgtxtedit->show();
}

void MyView::encryptDroppedFolder()
{
kgpgfoldertmp=new KTempFile(QString::null,".tar.gz");
if (KMessageBox::warningContinueCancel(0,i18n("<qt>KGpg will now create a temporary archive file:<br><b>%1</b> to process the encryption. The file will be deleted after the encryption is finished.</qt>").arg(kgpgfoldertmp->name()),i18n("Temporary File Creation"),KStdGuiItem::cont(),"FolderTmpFile")==KMessageBox::Cancel) return;

 popupPublic *dialogue=new popupPublic(0,"Public keys","files",true);
                        connect(dialogue,SIGNAL(selectedKey(QString &,QString,bool,bool)),this,SLOT(startFolderEncode(QString &,QString,bool,bool)));
			dialogue->CBshred->setEnabled(false);
                        if (!dialogue->exec()==QDialog::Accepted) return;
                        delete dialogue;
	}

void MyView::startFolderEncode(QString &selec,QString encryptOptions,bool ,bool symetric)
{
pop = new KPassivePopup();
        pop->setView(i18n("Processing archiving & encryption"),i18n("Please wait..."),KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop));
        pop->show();
        QRect qRect(QApplication::desktop()->screenGeometry());
        int iXpos=qRect.width()/2-pop->width()/2;
        int iYpos=qRect.height()/2-pop->height()/2;
        pop->move(iXpos,iYpos);
QString extension;
KTar arch(kgpgfoldertmp->name(), "application/x-gzip");
if (!arch.open( IO_WriteOnly )) KMessageBox::sorry(0,i18n("Unable to create temporary file"));
arch.addLocalDirectory (droppedUrls.first().path(),droppedUrls.first().filename());
arch.close();

if (encryptOptions.find("armor")!=-1) extension=".asc";
else if (pgpExtension) extension=".pgp";
else extension=".gpg";
KgpgInterface *folderprocess=new KgpgInterface();
folderprocess->KgpgEncryptFile(selec,KURL(kgpgfoldertmp->name()),KURL(droppedUrls.first().path()+".tar.gz"+extension),encryptOptions,symetric);
connect(folderprocess,SIGNAL(encryptionfinished(KURL)),this,SLOT(slotFolderFinished(KURL)));
connect(folderprocess,SIGNAL(errormessage(QString)),this,SLOT(slotFolderFinishedError(QString)));
}

void  MyView::slotFolderFinished(KURL)
{
delete pop;
kgpgfoldertmp->unlink();
}

void  MyView::slotFolderFinishedError(QString errmsge)
{
delete pop;
KMessageBox::sorry(0,errmsge);
kgpgfoldertmp->unlink();
}


void  MyView::encryptDroppedFile()
{
        QString opts="";
        KgpgLibrary *lib=new KgpgLibrary(pgpExtension);
        if (encryptfileto) {
                if (untrusted)
                        opts=" --always-trust ";
                if (ascii)
                        opts+=" --armor ";
                if (hideid)
                        opts+=" --throw-keyid ";
                if (pgpcomp)
                        opts+=" --pgp6 ";
                ksConfig->setGroup("Encryption");
                lib->slotFileEnc(droppedUrls,opts,ksConfig->readEntry("file key").left(8));
        } else
                lib->slotFileEnc(droppedUrls);
}


void  MyView::shredDroppedFile()
{
        if (KMessageBox::warningContinueCancelList(0,i18n("Do you really want to shred these files"),droppedUrls.toStringList())!=KMessageBox::Continue)
                return;
        KURL::List::iterator it;
        for ( it = droppedUrls.begin(); it != droppedUrls.end(); ++it ) {
                if (!KURL(*it).isLocalFile())
                        KMessageBox::sorry(0,i18n("Cannot shred remote files!"));
                else {
                        kgpgShredWidget *sh=new kgpgShredWidget(0,"shred");
                        sh->setCaption(i18n("Shredding %1").arg(KURL(*it).filename()));
                        sh->show();
                        sh->kgpgShredFile(KURL(*it));
                }
        }
}


void  MyView::slotVerifyFile()
{
        ///////////////////////////////////   check file signature
        if (droppedUrl.isEmpty())
                return;

        QString sigfile="";
        //////////////////////////////////////       try to find detached signature.
        if (!droppedUrl.filename().endsWith(".sig")) {
                sigfile=droppedUrl.path()+".sig";
                QFile fsig(sigfile);
                if (!fsig.exists()) {
                        sigfile=droppedUrl.path()+".asc";
                        QFile fsig(sigfile);
                        //////////////   if no .asc or .sig signature file included, assume the file is internally signed
                        if (!fsig.exists())
                                sigfile="";
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
        keyServer *kser=new keyServer(0,"server_dialog",false,WDestructiveClose);
        kser->kLEimportid->setText(ID);
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
        QString Options;
        if (ascii)
                Options=" --armor ";
        if (pgpcomp)
                Options+=" --pgp6 ";
        KgpgInterface *signFileProcess=new KgpgInterface();
        signFileProcess->KgpgSignFile(signKeyID,droppedUrl,Options);
}

void  MyView::decryptDroppedFile()
{
        if (!droppedUrl.isLocalFile()) {
                showDroppedFile();
                return;
        }
        QString oldname=droppedUrl.filename();
        if (oldname.endsWith(".gpg") || oldname.endsWith(".asc") || oldname.endsWith(".pgp"))
                oldname.truncate(oldname.length()-4);
        else
                oldname.append(".clear");
        KURL swapname(droppedUrl.directory(0,0)+oldname);
        QFile fgpg(swapname.path());
        if (fgpg.exists()) {
                KgpgOverwrite *over=new KgpgOverwrite(0,"overwrite",swapname);
                if (over->exec()==QDialog::Accepted)
                        swapname=KURL(swapname.directory(0,0)+over->getfname());
                else {
                        delete over;
                        return;
                }
                delete over;
        }

        KgpgLibrary *lib=new KgpgLibrary();
        lib->slotFileDec(droppedUrl,swapname,customDecrypt);
}

void  MyView::showDroppedFile()
{
        KgpgApp *kgpgtxtedit = new KgpgApp(0, "editor",WDestructiveClose);
        kgpgtxtedit->view->editor->droppedfile(droppedUrl);
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
        ksConfig->setGroup("User Interface");

        if ((droppedUrl.path().endsWith(".asc")) || (droppedUrl.path().endsWith(".pgp")) || (droppedUrl.path().endsWith(".gpg"))) {
                switch (ksConfig->readNumEntry("encrypted_drop_event",0)) {
                case 0:
                        decryptDroppedFile();
                        break;
                case 1:
                        showDroppedFile();
                        break;
                case 2:
                        droppopup->exec(QCursor::pos ());
                        break;
                }
        } else if (droppedUrl.path().endsWith(".sig")) {
                slotVerifyFile();
        } else
                switch (ksConfig->readNumEntry("unencrypted_drop_event",0)) {
                case 0:
                        encryptDroppedFile();
                        break;
                case 1:
                        signDroppedFile();
                        break;
                case 2:
                        udroppopup->exec(QCursor::pos ());
                        break;
                }
}


void  MyView::droppedtext (QString inputText)
{

        QClipboard *cb = QApplication::clipboard();
        cb->setText(inputText);
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
                        importKeyProcess->importKey(inputText,false);
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
        //kdDebug()<<"Reading options\n";
        ksConfig->setGroup("Encryption");
        encryptfileto=ksConfig->readBoolEntry("encrypt_files_to",false);
        ascii=ksConfig->readBoolEntry("Ascii_armor",true);
        untrusted=ksConfig->readBoolEntry("Allow_untrusted_keys",false);
        hideid=ksConfig->readBoolEntry("Hide_user_ID",false);
        pgpcomp=ksConfig->readBoolEntry("PGP_compatibility",false);
        pgpExtension=ksConfig->readBoolEntry("Pgp_extension",false);

        ksConfig->setGroup("Decryption");
        customDecrypt=ksConfig->readEntry("custom_decrypt");

        ksConfig->setGroup("User Interface");
        if (ksConfig->readBoolEntry("selection clip",false)) {
                if (kapp->clipboard()->supportsSelection())
                        kapp->clipboard()->setSelectionMode(true);
        } else
                kapp->clipboard()->setSelectionMode(false);
        ksConfig->setGroup("General Options");
        if (ksConfig->readBoolEntry("First run",true))
                firstRun();
        else {
                ksConfig->setGroup("GPG Settings");
                if (ksConfig->readPathEntry("gpg_config_path").isEmpty()) {
                        if (KMessageBox::questionYesNo(0,"<qt>You did not set a path to your GnuPG config file.<br>This may bring some surprising results in KGpg's execution.<br>Would you like to start KGpg's Wizard to fix this problem ?</qt>")==KMessageBox::Yes)
                                startWizard();
                }
        }

        ksConfig->setGroup("TipOfDay");
        tipofday=ksConfig->readBoolEntry("RunOnStart",true);
}


void  MyView::firstRun()
{
        FILE *fp;
        QString tst;
        char line[200];
        bool found=false;

        //kgpgOptions *opts=new kgpgOptions(this,0);
        //opts->checkMimes();
        //delete opts;

        fp = popen("gpg --no-tty --with-colon --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=line;
                if (tst.startsWith("sec")) {
                        found=true;
                        break;
                }
        }
        pclose(fp);
        startWizard();
}


void  MyView::startWizard()
{
        kdDebug()<<"Starting Wizard\n";

        wiz=new KgpgWizard(0,"wizard");
        QString confPath=QDir::homeDirPath()+"/.gnupg/options";
        if (!QFile(confPath).exists()) {
                confPath=QDir::homeDirPath()+"/.gnupg/gpg.conf";
                if (!QFile(confPath).exists()) {
                        wiz->text_optionsfound->setText(i18n("<qt><b>The GnuPG configuration file was not found</b>. Please make sure you have GnuPG installed and give the path to the file.</qt>"));
                        confPath="";
                }
        }
        wiz->kURLRequester1->setURL(confPath);
        wiz->kURLRequester2->setURL(QString(QDir::homeDirPath()+"/Desktop"));
        wiz->kURLRequester2->setMode(2);
        connect(wiz->pushButton4,SIGNAL(clicked()),this,SLOT(slotGenKey()));
        connect(wiz->finishButton(),SIGNAL(clicked()),this,SLOT(slotSaveOptionsPath()));
        connect( wiz , SIGNAL( destroyed() ) , this, SLOT( slotWizardClose()));

        wiz->setFinishEnabled(wiz->page_4,true);
        wiz->show();
}

void  MyView::slotSaveOptionsPath()
{

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

        ksConfig->setGroup("User Interface");
        ksConfig->writeEntry("AutoStart", wiz->checkBox2->isChecked());
        ksConfig->setGroup("GPG Settings");
#if KDE_IS_VERSION(3,1,3)

        ksConfig->writePathEntry("gpg_config_path",wiz->kURLRequester1->url());
#else

        ksConfig->writeEntry("gpg_config_path",wiz->kURLRequester1->url());
#endif

        ksConfig->setGroup("General Options");
        ksConfig->writeEntry("First run",false);
        ksConfig->sync();
        if (wiz)
                delete wiz;
}

void  MyView::slotWizardClose()
{
        wiz=0L;
}

void  MyView::slotGenKey()
{
        listKeys *creat=new listKeys(0);
        creat->slotgenkey();
        delete creat;
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


void  MyView::preferences()
{
        if (KAutoConfigDialog::showDialog("settings"))
                return;
        kgpgOptions *optsDialog=new kgpgOptions(this,"settings");
        connect(optsDialog,SIGNAL(updateSettings()),this,SLOT(readAgain1()));
        optsDialog->show();
}

void MyView::readAgain1()
{
        readOptions();
        emit readAgain2();
}


void MyView::slotKeyServerClosed()
{
        delete m_keyServer;
        m_keyServer=0L;
}


kgpgapplet::kgpgapplet(QWidget *parent, const char *name)
                : KSystemTray(parent,name)
{
        w=new MyView(this);
        connect(w,SIGNAL(readAgain2()),this,SLOT(readAgain3()));
        w->show();
        KPopupMenu *conf_menu=contextMenu();
        KAction *KgpgEncryptClipboard = new KAction(i18n("&Encrypt Clipboard"), 0, 0,this, SLOT(slotencryptclip()),actionCollection(),"clip_encrypt");
        KAction *KgpgDecryptClipboard = new KAction(i18n("&Decrypt Clipboard"), 0, 0,this, SLOT(slotdecryptclip()),actionCollection(),"clip_decrypt");
        KAction *KgpgOpenEditor = new KAction(i18n("&Open Editor"), "edit", 0,this, SLOT(sloteditor()),actionCollection(),"kgpg_editor");
        KAction *KgpgPreferences=KStdAction::preferences(this, SLOT(slotOptions()), actionCollection());
        KgpgEncryptClipboard->plug(conf_menu);
        KgpgDecryptClipboard->plug(conf_menu);
        KgpgOpenEditor->plug(conf_menu);
        conf_menu->insertSeparator();
        KgpgPreferences->plug(conf_menu);
}

void kgpgapplet::readAgain3()
{
        emit readAgain4();
}



kgpgapplet::~kgpgapplet()
{
        delete w;
        w = 0L;
}

void kgpgapplet::sloteditor()
{
        w->openEditor();
}

void kgpgapplet::slotencryptclip()
{
        w->clipEncrypt();
}

void kgpgapplet::slotdecryptclip()
{
        w->clipDecrypt();
}

void kgpgapplet::slotOptions()
{
        w->preferences();
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
        /*
                int autoStart = KMessageBox::questionYesNoCancel( 0, i18n("Should KGpg start automatically\nwhen you login?"), i18n("Automatically Start KGpg?"),KStdGuiItem::yes(),KStdGuiItem::no(),"Autostartup");
                kgpg_applet->w->ksConfig->setGroup("User Interface");
                if ( autoStart == KMessageBox::Yes )
                        kgpg_applet->w->ksConfig->writeEntry("AutoStart", true);
                else if ( autoStart == KMessageBox::No) {
                        kgpg_applet->w->ksConfig->writeEntry("AutoStart", false);
                } else  // cancel chosen don't quit
                        return;
                kgpg_applet->w->ksConfig->sync();
        */
        quit();
}


int KgpgAppletApp::newInstance()
{
        kdDebug()<<"New instance\n";
        args = KCmdLineArgs::parsedArgs();
        if (running) {
                kdDebug()<<"Already running\n";
                kgpg_applet->show();
        } else {
                kdDebug() << "Starting KGpg\n";
                running=true;
		s_keyManager=new listKeys(0, "key_manager");
                s_keyManager->refreshkey();
                kgpg_applet=new kgpgapplet(s_keyManager,"kgpg_systrayapplet");
                connect( kgpg_applet, SIGNAL(quitSelected()), this, SLOT(slotHandleQuit()));
                connect(kgpg_applet,SIGNAL(readAgain4()),s_keyManager,SLOT(readOptions()));
                connect(s_keyManager,SIGNAL(readAgainOptions()),kgpg_applet->w,SLOT(readOptions()));
                kgpg_applet->show();
                kgpg_applet->w->ksConfig->setGroup("GPG Settings");
                QString gpgPath=kgpg_applet->w->ksConfig->readPathEntry("gpg_config_path");

                if (!gpgPath.isEmpty()) {
                        if ((KgpgInterface::getGpgBoolSetting("use-agent",gpgPath)) && (!getenv("GPG_AGENT_INFO")))
                                KMessageBox::sorry(0,i18n("<qt>The use of <b>GnuPG Agent</b> is enabled in GnuPG's configuration file (%1).<br>"
                                                          "However, the agent doesn't seem to run. This could result in problems with signing/decryption.<br>"
							  "Please disable GnuPG Agent from KGpg settings, or fix the agent.</qt>").arg(gpgPath));
                }

        }

        ////////////////////////   parsing of command line args
        if (args->isSet("k")!=0) {
                s_keyManager->show();
                KWin::setOnDesktop( s_keyManager->winId() , KWin::currentDesktop() );  //set on the current desktop
                KWin::deIconifyWindow( s_keyManager->winId());  //de-iconify window
                s_keyManager->raise();  // set on top
                s_keyManager->refreshkey();
        } else
                if (args->count()>0) {
                        kdDebug() << "KGpg: found files";

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

			if ((directoryInside) && (lst.count()>1)){
                                KMessageBox::sorry(0,i18n("Sorry, cannot perform requested operation.\nPlease select only one directory, or several files, but don't mix files and directories."));
                                return 0;
                        }

                        kgpg_applet->w->droppedUrls=urlList;

                        if (args->isSet("e")!=0)
			{
				if (!directoryInside)
                                kgpg_applet->w->encryptDroppedFile();
				else kgpg_applet->w->encryptDroppedFolder();
				}
                        else if (args->isSet("X")!=0)
			{
				if (!directoryInside)
                                kgpg_applet->w->shredDroppedFile();
				else KMessageBox::sorry(0,i18n("Cannot shred directory"));
				}
                        else if (args->isSet("s")!=0)
			{
                                if (!directoryInside)
				kgpg_applet->w->showDroppedFile();
				else KMessageBox::sorry(0,i18n("Cannot decrypt & show directory"));
				}
                        else if (args->isSet("S")!=0)
			{
                                if (!directoryInside)
				kgpg_applet->w->signDroppedFile();
				else KMessageBox::sorry(0,i18n("Cannot sign directory"));
				}
                        else if (args->isSet("V")!=0)
			{
				if (!directoryInside)
                                kgpg_applet->w->slotVerifyFile();
				else KMessageBox::sorry(0,i18n("Cannot verify directory"));
				}
                        else if (kgpg_applet->w->droppedUrl.filename().endsWith(".sig"))
                                kgpg_applet->w->slotVerifyFile();
                        else
                                kgpg_applet->w->decryptDroppedFile();
                }
        return 0;
}




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void MyView::encryptClipboard(QString &selec,QString encryptOptions)
{
        QString clipContent=kapp->clipboard()->text();//=cb->text(QClipboard::Clipboard);   ///   QT 3.1 only

        if (!clipContent.isEmpty()) {
                if (pgpcomp)
                        encryptOptions+=" --pgp6 ";
                encryptOptions+=" --armor ";

                if (selec==NULL) {
                        KMessageBox::sorry(0,i18n("You have not chosen an encryption key."));
                }//exit(0);}

                KgpgInterface *txtEncrypt=new KgpgInterface();
                connect (txtEncrypt,SIGNAL(txtencryptionfinished(QString)),this,SLOT(slotSetClip(QString)));
                txtEncrypt->KgpgEncryptText(clipContent,selec,encryptOptions);
                QString cryptedClipboard;
                if (clipContent.length()>300)
                        cryptedClipboard=QString(clipContent.left(250).stripWhiteSpace())+"...\n"+QString(clipContent.right(40).stripWhiteSpace());
                else
                        cryptedClipboard=clipContent;


                cryptedClipboard.replace(QRegExp("<"),"&lt;");   /////   disable html tags
                cryptedClipboard.replace(QRegExp("\n"),"<br>");



                pop = new KPassivePopup( this);
                pop->setView(i18n("Encrypted following text:"),cryptedClipboard,KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop));
                pop->setTimeout(3200);
                pop->show();
                QRect qRect(QApplication::desktop()->screenGeometry());
                int iXpos=qRect.width()/2-pop->width()/2;
                int iYpos=qRect.height()/2-pop->height()/2;
                pop->move(iXpos,iYpos);

        } else {
                KMessageBox::sorry(0,i18n("Clipboard is empty."));
                //exit(0);
        }
}

void MyView::slotSetClip(QString newtxt)
{
        if (!newtxt.isEmpty()) {
                QClipboard *clip=QApplication::clipboard();
                clip->setText(newtxt);//,QClipboard::Clipboard);    QT 3.1 only
                //connect(kapp->clipboard(),SIGNAL(dataChanged ()),this,SLOT(expressQuit()));
        }
        //else expressQuit();
}



/////////////////////////////////////////////////////////////////////////////

#include "kgpg.moc"


