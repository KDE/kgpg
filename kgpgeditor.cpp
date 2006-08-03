/***************************************************************************
                          kgpgeditor.cpp  -  description
                             -------------------
    begin                : Mon Jul 8 2002
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

#include <kaction.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <dcopclient.h>
#include <qpaintdevicemetrics.h>
#include <qcstring.h>

#include <kencodingfiledialog.h>

#include <qradiobutton.h>
#include <qclipboard.h>
#include <qtextcodec.h>
#include <qpainter.h>
#include <kprinter.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <klineedit.h>
#include <qcheckbox.h>
#include <kurlrequester.h>
#include <ktempfile.h>
#include <kio/netaccess.h>
#include <kio/renamedlg.h>
#include <kedittoolbar.h>


#include "kgpgsettings.h"
#include "kgpgeditor.h"
#include "sourceselect.h"
#include "keyexport.h"
#include "keyserver.h"
#include "keyservers.h"
#include "kgpg.h"
#include "kgpgview.h"
#include "listkeys.h"
#include "kgpglibrary.h"

KgpgApp::KgpgApp(QWidget *parent, const char *name, WFlags f,KShortcut goHome,bool mainWindow):KMainWindow(parent, name,f)
{
	isMainWindow=mainWindow;
	textEncoding=QString::null;
        readOptions();
	goDefaultKey=goHome;
        // call inits to invoke all other construction parts

        initActions();
        initView();

	slotSetFont(KGpgSettings::font());
        setupGUI(( ToolBar | Keys | StatusBar | Save | Create ), "kgpg.rc");
	setAutoSaveSettings("Editor",true);

}

KgpgApp::~KgpgApp()
{
delete view;
}

void KgpgApp::slotOptions()
{
QByteArray data;
if (!kapp->dcopClient()->send("kgpg", "KeyInterface", "showOptions()",data))
kdDebug(2100) <<"there was some error using DCOP."<<endl;
}

void KgpgApp::slotKeyManager()
{
QByteArray data;
if (!kapp->dcopClient()->send("kgpg", "KeyInterface", "showKeyManager()",data))
kdDebug(2100) <<"there was some error using DCOP."<<endl;
}

void KgpgApp::closeEvent ( QCloseEvent * e )
{
        if (!isMainWindow)
	{
	 kapp->ref();
	KMainWindow::closeEvent( e );
	}
  else e->accept();
}

void KgpgApp::closeWindow()
{
if (view->windowAutoClose) close();
kdDebug(2100) << "Close requested"<<endl;
}

void KgpgApp::saveOptions()
{
        KGpgSettings::setEditorGeometry(size());
        KGpgSettings::setFirstRun(false);
        KGpgSettings::writeConfig();
}

void KgpgApp::readOptions(bool doresize)
{
	customDecrypt=QStringList::split(QString(" "), KGpgSettings::customDecrypt().simplifyWhiteSpace());

        if (doresize) {
                QSize size= KGpgSettings::editorGeometry();
                if (!size.isEmpty())
                        resize(size);
        }

}

void KgpgApp::initActions()
{
        KStdAction::openNew(this, SLOT(slotFileNew()), actionCollection());
        KStdAction::open(this, SLOT(slotFileOpen()), actionCollection());
        KStdAction::saveAs(this, SLOT(slotFileSaveAs()), actionCollection());
        KStdAction::quit(this, SLOT(slotFileQuit()), actionCollection());
        KStdAction::cut(this, SLOT(slotEditCut()), actionCollection());
        KStdAction::copy(this, SLOT(slotEditCopy()), actionCollection());
        KStdAction::paste(this, SLOT(slotEditPaste()), actionCollection());
	KStdAction::selectAll(this, SLOT(slotSelectAll()), actionCollection());
	KStdAction::preferences(this, SLOT(slotOptions()), actionCollection(),"kgpg_config");

        //KStdAction::keyBindings(guiFactory(), SLOT(configureShortcuts()), actionCollection());
        //KStdAction::configureToolbars(this, SLOT(slotConfigureToolbars()), actionCollection());

        fileSave = KStdAction::save(this, SLOT(slotFileSave()), actionCollection());
        (void) new KAction(i18n("&Encrypt File..."), "encrypted", 0,this, SLOT(slotFilePreEnc()), actionCollection(),"file_encrypt");
        (void) new KAction(i18n("&Decrypt File..."), "decrypted", 0,this, SLOT(slotFilePreDec()), actionCollection(),"file_decrypt");
	(void) new KAction(i18n("&Open Key Manager"), "kgpg", 0,this, SLOT(slotKeyManager()), actionCollection(),"key_manage");
        editUndo = KStdAction::undo(this, SLOT(slotundo()), actionCollection());
        editRedo = KStdAction::redo(this, SLOT(slotredo()), actionCollection());
        //(void) new KAction(i18n("&Manage Keys"), "kgpg_manage", CTRL+Key_K,this, SLOT(slotManageKey()), actionCollection(),"keys_manage");
        (void) new KAction(i18n("&Generate Signature..."),0, this, SLOT(slotPreSignFile()), actionCollection(), "sign_generate");
        (void) new KAction(i18n("&Verify Signature..."),0, this, SLOT(slotPreVerifyFile()), actionCollection(), "sign_verify");
        (void) new KAction(i18n("&Check MD5 Sum..."), 0,this, SLOT(slotCheckMd5()), actionCollection(), "sign_check");
	KStdAction::print(this, SLOT(slotFilePrint()), actionCollection());

    // comment out for now, only confusing
	//encodingAction=new KToggleAction(i18n("&Unicode (utf-8) Encoding"), 0, 0,this, SLOT(slotSetCharset()),actionCollection(),"charsets");
}

void KgpgApp::slotSetFont(QFont myFont)
{
view->editor->setFont (myFont);
}


void KgpgApp::slotSetCharset()
{
////////  work in progress
//if (encodingAction->isChecked())
//view->editor->setText(QString::fromUtf8(view->editor->text().ascii()));
//else
{
if (checkEncoding(QTextCodec::codecForLocale ())) return;
view->editor->setText(view->editor->text().utf8());
}
}

void KgpgApp::initView()
{
        ////////////////////////////////////////////////////////////////////
        // create the main widget here that is managed by KTMainWindow's view-region and
        // connect the widget to your document to display document contents.

        view = new KgpgView(this,0);
        //  doc->addView(view);
	connect(view,SIGNAL(resetEncoding(bool)),this,SLOT(slotResetEncoding(bool)));
        setCentralWidget(view);
        setCaption(i18n("Untitled"),false); ///   doc->URL().fileName(),false);

}

void KgpgApp::slotFileQuit()
{
        saveOptions();
        exit(1);
}

void KgpgApp::slotResetEncoding(bool enc)
{
//kdDebug(2100)<<"Resetting encoding--------------------"<<endl;
//encodingAction->setChecked(enc);
//if (enc) slotSetCharset();
}

void KgpgApp::slotFileNew()
{
        //////  delete all text from editor

        view->editor->setText(QString::null);
        editRedo->setEnabled(false);
        editUndo->setEnabled(false);
        setCaption(i18n("Untitled"), false);
        fileSave->setEnabled(false);
        Docname=QString::null;
	slotResetEncoding(false);
}

void KgpgApp::slotFilePreEnc()
{
        QStringList opts;
	KURL::List urls=KFileDialog::getOpenURLs(QString::null,
                                         i18n("*|All Files"), this, i18n("Open File to Encode"));
        if (urls.isEmpty())
                return;
	emit encryptFiles(urls);
}

void KgpgApp::slotFilePreDec()
{

        KURL url=KFileDialog::getOpenURL(QString::null,
                                         i18n("*|All Files"), this, i18n("Open File to Decode"));

        if (url.isEmpty())
                return;
        QString oldname=url.fileName();

        QString newname;

        if (oldname.endsWith(".gpg") || oldname.endsWith(".asc") || oldname.endsWith(".pgp"))
                oldname.truncate(oldname.length()-4);
        else
                oldname.append(".clear");
        oldname.prepend(url.directory(0,0));

	KDialogBase *popn=new KDialogBase( KDialogBase::Swallow, i18n("Decrypt File To"), KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok, this, "file_decrypt",true);

	SrcSelect *page=new SrcSelect();
	popn->setMainWidget(page);
	page->newFilename->setURL(oldname);
	page->newFilename->setMode(KFile::File);
	page->newFilename->setCaption(i18n("Save File"));

	page->checkClipboard->setText(i18n("Editor"));
	page->resize(page->minimumSize());
	popn->resize(popn->minimumSize());
        if (popn->exec()==QDialog::Accepted) {
                if (page->checkFile->isChecked())
                        newname=page->newFilename->url();
        } else {
                delete popn;
                return;
        }
        delete popn;


        if (!newname.isEmpty()) {
                QFile fgpg(newname);
                if (fgpg.exists()) {
			KIO::RenameDlg *over=new KIO::RenameDlg(0,i18n("File Already Exists"),QString::null,newname,KIO::M_OVERWRITE);
		    	if (over->exec()==QDialog::Rejected)
	    		{
                	delete over;
                	return;
            		}
	    		newname=over->newDestURL().path();
	    		delete over;
                }
                KgpgLibrary *lib=new KgpgLibrary(this);
                lib->slotFileDec(url,KURL(newname), customDecrypt);
		connect(lib,SIGNAL(importOver(QStringList)),this,SIGNAL(refreshImported(QStringList)));
        } else
                openEncryptedDocumentFile(url);
}

void KgpgApp::slotFileOpen()
{
	KEncodingFileDialog::Result loadResult;
	loadResult=KEncodingFileDialog::getOpenURLAndEncoding(QString::null,QString::null,QString::null,this);
	KURL url=loadResult.URLs.first();
	textEncoding=loadResult.encoding;

        if(!url.isEmpty()) {
                openDocumentFile(url,textEncoding);
                Docname=url;
                fileSave->setEnabled(false);
                //fileSaveAs->setEnabled(true);
                setCaption(url.fileName(), false);
        }

}

bool KgpgApp::checkEncoding(QTextCodec *codec)
{
 /////////////          KGlobal::locale()->encoding()->name()
return codec->canEncode(view->editor->text());
}

void KgpgApp::slotFileSave()
{
    QString filn=Docname.path();
    if (filn.isEmpty()) {
    	slotFileSaveAs();
    	return;
    }
    QTextCodec*cod=QTextCodec::codecForName (textEncoding.ascii());
        // slotStatusMsg(i18n("Saving file..."));
    if (!checkEncoding(cod)) {
	KMessageBox::sorry(this,i18n("The document could not been saved, as the selected encoding cannot encode every unicode character in it."));
	return;
    }

    KTempFile tmpfile;
    if (Docname.isLocalFile()) {
    QFile f(filn);
    if ( !f.open( IO_WriteOnly ) ) {
	KMessageBox::sorry(this,i18n("The document could not be saved, please check your permissions and disk space."));
        return;
    }
    QTextStream t( &f );
    t.setCodec(cod);
    //t.setEncoding( QTextStream::Latin1 );
    t << view->editor->text();//.utf8();
    f.close();
    }
    else {
	/*FIXME  use following code:
	 QFile f( fName );
00983         if ( !f.open( IO_ReadOnly ) )
00984             return;
00985         QFileInfo info ( f );
00986         smModificationTime = new QTime( info.lastModified().time() );
00987         QTextStream t(&f);
00988         t.setEncoding( QTextStream::Latin1 );
00989         QString s = t.readLine();
00990         f.close();

*/
	QTextStream *stream = tmpfile.textStream();
	stream->setCodec(cod);
    	*stream << view->editor->text();//.utf8();
   	tmpfile.close();
	if(!KIO::NetAccess::upload(tmpfile.name(), Docname,this)) {
	    KMessageBox::sorry(this,i18n("The document could not be saved, please check your permissions and disk space."));
	    tmpfile.unlink();
            return;
	}
	tmpfile.unlink();
    }

    fileSave->setEnabled(false);
    setCaption(Docname.fileName(),false);
}


void KgpgApp::slotFileSaveAs()
{

        //KURL url=KFileDialog::getSaveURL(QDir::currentDirPath(),i18n("*|All Files"), this, i18n("Save As"));
	KEncodingFileDialog::Result saveResult;
	saveResult=KEncodingFileDialog::getSaveURLAndEncoding (QString::null,QString::null,QString::null,this);
	KURL url=saveResult.URLs.first();
	QString selectedEncoding=saveResult.encoding;

	if(!url.isEmpty()) {
		if (url.isLocalFile())
		{
                QString filn=url.path();
                QFile f(filn);
                if (f.exists()) {
                        QString message=i18n("Overwrite existing file %1?").arg(url.fileName());
                        int result=KMessageBox::warningContinueCancel(this,QString(message),i18n("Warning"),i18n("Overwrite"));
                        if (result==KMessageBox::Cancel)
                                return;
                }
                f.close();
		}
		else if (KIO::NetAccess::exists(url,false,this))
		{
		QString message=i18n("Overwrite existing file %1?").arg(url.fileName());
                        int result=KMessageBox::warningContinueCancel(this,QString(message),i18n("Warning"),i18n("Overwrite"));
                        if (result==KMessageBox::Cancel)
                                return;
		}
	Docname=url;
	textEncoding=selectedEncoding;
	slotFileSave();
	}
}

void KgpgApp::openDocumentFile(const KURL& url,QString encoding)
{
QString tempOpenFile;
        /////////////////////////////////////////////////
if( KIO::NetAccess::download( url, tempOpenFile,this ) ) {
        QFile qfile(tempOpenFile);
        if (qfile.open(IO_ReadOnly)) {
                QTextStream t( &qfile );
		t.setCodec(QTextCodec::codecForName (encoding.ascii()));
                view->editor->setText(t.read());
                qfile.close();
                fileSave->setEnabled(false);
                editRedo->setEnabled(false);
                editUndo->setEnabled(false);
        }
}
}

void KgpgApp::slotFilePrint()
{
        KPrinter prt;
        //kdDebug(2100)<<"Printing..."<<endl;
        if (prt.setup(this)) {
                QPainter painter(&prt);
                QPaintDeviceMetrics metrics(painter.device());
                painter.drawText( 0, 0, metrics.width(), metrics.height(), AlignLeft|AlignTop|DontClip,view->editor->text() );
        }
}

void KgpgApp::slotEditCut()
{
        view->editor->cut();
}

void KgpgApp::slotEditCopy()
{
        view->editor->copy();
}

void KgpgApp::slotEditPaste()
{
        view->editor->paste();
}

void KgpgApp::slotSelectAll()
{
        view->editor->selectAll();
}

void KgpgApp::slotundo()
{
        view->editor->undo();
        editRedo->setEnabled(true);
}

void KgpgApp::slotredo()
{
        view->editor->redo();
}

/////////////    file signature slots


void KgpgApp::slotCheckMd5()
{
        /////////////////////////////////////////////////////////////////////////  display md5 sum for a chosen file

        KURL url=KFileDialog::getOpenURL(QString::null,
                                         i18n("*|All Files"), this, i18n("Open File to Verify"));
        if (!url.isEmpty()) {

                Md5Widget *mdwidget=new Md5Widget(this,0,url);
                mdwidget->exec();
                delete mdwidget;
                //      KMessageBox::information(this,QString("MD5 sum for "+url.fileName()+" is:\n"+checkfile.hexDigest().data()));
        }
}


void KgpgApp::slotPreSignFile()
{
        //////////////////////////////////////   create a detached signature for a chosen file
        KURL url=KFileDialog::getOpenURL(QString::null,i18n("*|All Files"), this, i18n("Open File to Sign"));
        if (!url.isEmpty())
                slotSignFile(url);
}

void KgpgApp::slotSignFile(KURL url)
{
        //////////////////////////////////////   create a detached signature for a chosen file
        QString signKeyID;
        if (!url.isEmpty()) {
                //////////////////   select a private key to sign file --> listkeys.cpp
                KgpgSelKey *opts=new KgpgSelKey(this,"select_secret");
                if (opts->exec()==QDialog::Accepted)
                        signKeyID=opts->getkeyID();
                else {
                        delete opts;
                        return;
                }
                delete opts;
                QString Options;
                if (KGpgSettings::asciiArmor())
                        Options="--armor";
                if (KGpgSettings::pgpCompatibility())
                        Options+="--pgp6";
                KgpgInterface *signFileProcess=new KgpgInterface();
                signFileProcess->KgpgSignFile(signKeyID,url,Options);
        }
}

void KgpgApp::slotPreVerifyFile()
{
        KURL url=KFileDialog::getOpenURL(QString::null,
                                         i18n("*|All Files"), this, i18n("Open File to Verify"));
        slotVerifyFile(url);
}

void KgpgApp::slotVerifyFile(KURL url)
{
        ///////////////////////////////////   check file signature
        QString sigfile=QString::null;
        if (!url.isEmpty()) {
                //////////////////////////////////////       try to find detached signature.
                if (!url.fileName().endsWith(".sig")) {
                        sigfile=url.path()+".sig";
                        QFile fsig(sigfile);
                        if (!fsig.exists()) {
                                sigfile=url.path()+".asc";
                                QFile fsig(sigfile);
                                //////////////   if no .asc or .sig signature file included, assume the file is internally signed
                                if (!fsig.exists())
                                        sigfile=QString::null;
                        }
                }
                ///////////////////////// pipe gpg command
                KgpgInterface *verifyFileProcess=new KgpgInterface();
                verifyFileProcess->KgpgVerifyFile(url,KURL(sigfile));
                connect (verifyFileProcess,SIGNAL(verifyquerykey(QString)),this,SLOT(importSignatureKey(QString)));
        }
}

void KgpgApp::importSignatureKey(QString ID)
{
        keyServer *kser=new keyServer(0,"server_dialog",false);
        kser->page->kLEimportid->setText(ID);
        kser->slotImport();
}

void KgpgApp::openEncryptedDocumentFile(const KURL& url)
{
        view->editor->slotDroppedFile(url);
}


#include "kgpgeditor.moc"
