/***************************************************************************
                        kgpg.cpp  -  description
                           -------------------
  begin                : Tue Jul  2 12:31:38 GMT 2002
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

// include files for QT
#include <qstring.h>
#include <qtimer.h>
#include <qfile.h>
#include <stdlib.h>
#include <qwidgetlist.h>
#include <unistd.h>

// include files for KDE
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kmenubar.h>
#include <kshred.h>

#include <klocale.h>
#include <kconfig.h>
#include <kstdaction.h>
#include <kpopupmenu.h>
#include <ktip.h>
#include <kapplication.h>
#include <qclipboard.h>
#include <qdialog.h>



// application specific includes

#include "kgpgfast.h"

//#include <qfont.h>
#include "kgpg.h"
#include "kgpginterface.h"

#define ID_STATUS_MSG 1

KgpgApp::KgpgApp(const char* name,KURL fileToOpen,QString opmode):KMainWindow(0, name)
{ 
  config=kapp->config();
  readOptions();
commandLineMode=false;
  if ((opmode=="") || (opmode=="show") || (opmode=="clipboard"))
    {
	  if (tipofday==true)
        slotTip();
		 KIconLoader *loader = KGlobal::iconLoader();
      fileEnc=loader->loadIcon("kgpg",KIcon::Small);
      fileDec=loader->loadIcon("kgpg2",KIcon::Small);
      ///////////////////////////////////////////////////////////////////
      // call inits to invoke all other construction parts
      initActions();
      initView();
      if (opmode=="clipboard") {commandLineMode=true;slotClip();} /// decrypt & open clipboard content
      view->gpgversion=version;
         fileSave->setEnabled(false);
      editRedo->setEnabled(false);
      editUndo->setEnabled(false);
	  if (opmode=="show") checkEncryptedDocumentFile(fileToOpen);
    createGUI("kgpg.rc");
	}
  else
  	if (opmode=="clipboardEnc")
	{
popupPublic *dialoguec=new popupPublic(this, "public_keys", 0,false);
connect(dialoguec,SIGNAL(selectedKey(QString &,bool,bool,bool,bool)),this,SLOT(encryptClipboard(QString &,bool,bool)));
if (dialoguec->exec()==QDialog::Rejected ) exit(0);
	}
else if (fileToOpen.filename()!="")
    {
      urlselected=fileToOpen;
	  commandLineMode=true;
      if (opmode=="encrypt") ///// user asked for encryption in command line arguments
        {
          if (!encryptfileto)
            {
              popupPublic *dialogue=new popupPublic(this,"public_keys",fileToOpen.filename(),true);
              connect(dialogue,SIGNAL(selectedKey(QString &,bool,bool,bool,bool)),this,SLOT(fastencode(QString &,bool,bool,bool,bool)));
              if (dialogue->exec()==QDialog::Rejected ) exit(0);
            }
          else
            fastencode(filekey,untrusted,ascii,false,false);
          return;
        }
      else if (opmode=="decrypt") fastdecode(true);  ///// user asked for decryption in command line arguments
     else if (opmode=="verify") slotVerifyFile(urlselected);
	  else if (opmode=="sign") slotSignFile(urlselected);
	  else view->editor->droppedfile(urlselected);
    }
}

KgpgApp::~KgpgApp()
{
}


void KgpgApp::encryptClipboard(QString &selec,bool utrust,bool)
{
QString encryptOptions;
QString clipContent=kapp->clipboard()->text();//=cb->text(QClipboard::Clipboard);   ///   QT 3.1 only

  if (clipContent)
  {
  //////////////////              encode from editor
  if (utrust==true) encryptOptions+=" --always-trust ";
encryptOptions+=" --armor ";

  if (pgpcomp==true)
    {
      if (version<120) encryptOptions+=" --compress-algo 1 --cipher-algo cast5 ";
      else encryptOptions+=" --pgp6 ";
    }
 if (selec==NULL) {KMessageBox::sorry(0,i18n("You have not choosen an encryption key..."));exit(0);}
 QString decresultat=KgpgInterface::KgpgEncryptText(clipContent,selec,encryptOptions);
 //cryptedClipboard=decresultat;
     QWidgetList  *list = QApplication::topLevelWidgets();
    QWidgetListIt it( *list );          // iterate over the widgets
    while ( it.current() ) {            // for each top level widget...
        if ( it.current()->isVisible() )
            it.current()->hide();
        ++it;
    }
    delete list;
QString cryptedClipboard=QString(clipContent.left(60).stripWhiteSpace());
 
 QClipboard *clip=QApplication::clipboard();
 clip->setText(decresultat);//,QClipboard::Clipboard);    QT 3.1 only
 
    clippop = new QDialog( this,0,false,WStyle_Customize | WStyle_NormalBorder);
              QVBoxLayout *vbox=new QVBoxLayout(clippop,3);
              QLabel *tex=new QLabel(clippop);
              tex->setText(i18n("<b>Encrypted following text:</b>"));
			  QLabel *tex2=new QLabel(clippop);
			  tex2->setText(cryptedClipboard+"...");
              vbox->addWidget(tex);
			  vbox->addWidget(tex2);
              clippop->setMinimumWidth(250);
              clippop->adjustSize();
			  clippop->show();
 QTimer::singleShot( 3000, this, SLOT(killDisplayClip())); 
 
 //KMessageBox::information(this,i18n("Encrypted following text:\n")+QString(clipContent.left(60).stripWhiteSpace())+"...");
 connect(kapp->clipboard(),SIGNAL(dataChanged ()),this,SLOT(expressQuit()));
 }
 else 
{
KMessageBox::sorry(0,i18n("Clipboard is empty"));
exit(0);
}
}	

void KgpgApp::killDisplayClip()
{
delete clippop;
}


void KgpgApp::expressQuit()
{
exit(0);
//KMessageBox::sorry(0,QString("Following text was encrypted:\n"));
//if (kapp->clipboard()->text()==cryptedClipboard) exit(0);//{KMessageBox::sorry(0,kapp->clipboard()->text());exit(0);}
}


void KgpgApp::slotman()
{
   kapp->startServiceByDesktopName("khelpcenter", QString("man:/gpg"), 0, 0, 0, "", true);
}

void KgpgApp::slotTip()
{
    KTipDialog::showTip(this, "kgpg/tips", true);
}

void KgpgApp::checkVersion()
{
  FILE *fp;
  QString tst;
  char line[200];
  fp = popen("gpg --version", "r");
  while ( fgets( line, sizeof(line), fp))
    {
      tst=line;
      if (tst.startsWith("gpg (GnuPG)") )
        {
          QString n=tst.section('.',0,0);
          n=n.section(' ',-1,-1);
          version=100*n.toInt();
          n=tst.section('.',1,1);
          version+=10*n.toInt();
          n=tst.section('.',2,2);
          version+=n.toInt();
        }
    }
  pclose(fp);


}


void KgpgApp::initActions()
{
  KStdAction::openNew(this, SLOT(slotFileNew()), actionCollection());
  KStdAction::open(this, SLOT(slotFileOpen()), actionCollection());
  fileSave = KStdAction::save(this, SLOT(slotFileSave()), actionCollection());
  KStdAction::saveAs(this, SLOT(slotFileSaveAs()), actionCollection());
  fileEncrypt = new KAction(i18n("&Encrypt file..."), fileEnc, 0,this, SLOT(slotFileEnc()), actionCollection(),"file_encrypt");
  fileDecrypt = new KAction(i18n("&Decrypt file..."), fileDec, 0,this, SLOT(slotFileDec()), actionCollection(),"file_decrypt");
  KStdAction::quit(this, SLOT(slotFileQuit()), actionCollection());

  editUndo = KStdAction::undo(this, SLOT(slotundo()), actionCollection());
  editRedo = KStdAction::redo(this, SLOT(slotredo()), actionCollection());
  editCut = KStdAction::cut(this, SLOT(slotEditCut()), actionCollection());
  editCut = KStdAction::copy(this, SLOT(slotEditCopy()), actionCollection());
  editCut = KStdAction::paste(this, SLOT(slotEditPaste()), actionCollection());

  keysManage = new KAction(i18n("&Manage keys"), "kgpg_manage", 0,this, SLOT(slotManageKey()), actionCollection(),"keys_manage");

  signGenerate = new KAction(i18n("&Generate Signature..."),0, this, SLOT(slotPreSignFile()), actionCollection(), "sign_generate");
  signVerify = new KAction(i18n("&Verify Signature..."),0, this, SLOT(slotPreVerifyFile()), actionCollection(), "sign_verify");
  signCheck = new KAction(i18n("&Check MD5 sum..."), 0,this, SLOT(slotCheckMd5()), actionCollection(), "sign_check");

  KStdAction::preferences(this, SLOT(slotOptions()), actionCollection());

  helptips = new KAction(i18n("Tip of the &Day..."), "idea", 0,this, SLOT(slotTip()), actionCollection(),"help_tipofday");
  manpage = new KAction(i18n("View GnuPG manual"), "contents", 0,this, SLOT(slotman()),actionCollection(),"gpg_man");
}

void KgpgApp::initStatusBar()
{
  ///////////////////////////////////////////////////////////////////
  // STATUSBAR
  // TODO: add your own items you need for displaying current application status.
  //  statusBar()->insertItem(i18n("Ready."), ID_STATUS_MSG);
}








void KgpgApp::initView()
{
  ////////////////////////////////////////////////////////////////////
  // create the main widget here that is managed by KTMainWindow's view-region and
  // connect the widget to your document to display document contents.

  view = new KgpgView(this,0);
  //  doc->addView(view);
  setCentralWidget(view);
  setCaption(i18n("untitled"),false); ///   doc->URL().fileName(),false);

}



void KgpgApp::slotTest()
{
}


void KgpgApp::checkEncryptedDocumentFile(const KURL& url)
{
  /////////////////////////////////////////////////
  urlselected=url;
  messages="";
  KProcIO *encid=new KProcIO();
  *encid << "gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--batch"<<"--status-fd=1"<<"-d"<<url.path().local8Bit();
  /////////  when process ends, update dialog infos
  QObject::connect(encid, SIGNAL(processExited(KProcess *)),this, SLOT(slotprocresulted(KProcess *)));
  QObject::connect(encid, SIGNAL(readReady(KProcIO *)),this, SLOT(slotprocread(KProcIO *)));
  encid->start(KProcess::NotifyOnExit,false);
}

void KgpgApp::slotprocresulted(KProcess *)
{
  openEncryptedDocumentFile(urlselected,messages);
}

void KgpgApp::openEncryptedDocumentFile(const KURL& url,QString userIDs)
{
  QFile qfile(url.path().local8Bit());
  QString encryptedText;
  if (qfile.open(IO_ReadOnly))
    {
      QTextStream t( &qfile );
      encryptedText=t.read();
      qfile.close();
      view->editor->setText(KgpgInterface::KgpgDecryptText(encryptedText,userIDs));
      fileSave->setEnabled(false);
      editRedo->setEnabled(false);
      editUndo->setEnabled(false);
    }
    else KMessageBox::sorry(0,i18n("Unable to read file..."));
}



void KgpgApp::openDocumentFile(const KURL& url)
{

  /////////////////////////////////////////////////

  QFile qfile(url.path().local8Bit());

  if (qfile.open(IO_ReadOnly))
    {
      QTextStream t( &qfile );
      view->editor->setText(t.read());
      qfile.close();
      fileSave->setEnabled(false);
      editRedo->setEnabled(false);
      editUndo->setEnabled(false);
    }

}


void KgpgApp::saveOptions()
{
  config->setGroup("General Options");
  config->writeEntry("Geometry", size());
  config->writeEntry("First run",false);
  config->writeEntry("gpg version",version);
}

void KgpgApp::readOptions(bool doresize)
{
  config->setGroup("General Options");

  ascii=config->readBoolEntry("Ascii armor",true);
  untrusted=config->readBoolEntry("Allow untrusted keys",false);
  pgpcomp=config->readBoolEntry("PGP compatibility",false);
  encrypttodefault=config->readBoolEntry("encrypt to default key",false);
  defaultkey=config->readEntry("default key");
  encryptfileto=config->readBoolEntry("encrypt files to",false);
  filekey=config->readEntry("file key");

version=config->readNumEntry("gpg version",0);
if (version==0) checkVersion();

  if (doresize==true)
    {
      QSize size=config->readSizeEntry("Geometry");
      bool frun=config->readBoolEntry("First run",true);
      config->setGroup("TipOfDay");
      tipofday=config->readBoolEntry("RunOnStart",true);
      if (frun) firstrun();
	else 
      if (!size.isEmpty())
        resize(size);
    }
	
}

/////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATION
/////////////////////////////////////////////////////////////////////

void KgpgApp::firstrun()
{
  FILE *fp;
  QString tst;
  char line[200];
  bool found=false;
  
kgpgOptions *opts=new kgpgOptions(this,0);  
opts->checkMimes();
delete opts;	  

  
  fp = popen("gpg --no-tty --with-colon --list-secret-keys", "r");
  while ( fgets( line, sizeof(line), fp))
    {
      tst=line;
      if (tst.startsWith("sec"))
        found=true;
    }
  pclose(fp);
  if (found==false)
    {
      int result=KMessageBox::questionYesNo(0,i18n("Welcome to KGPG.\nNo secret key was found on your computer.\nWould you like to create one now ?"));
      if (result==3)
        {
          listKeys *creat=new listKeys(this);
          creat->slotgenkey();
          delete creat;
        }
    }
}




void KgpgApp::slotOptions()
{
  /////////////////////// open option dialog box --> kgpgoptions.cpp
  kgpgOptions *opts=new kgpgOptions(this,0);
  opts->exec();

  delete opts;
  readOptions(false);
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
                                   i18n("*|all files"), this, i18n("Open file to verify"));
  if (!url.isEmpty())
    {

      Md5Widget *mdwidget=new Md5Widget(this,0,url);
      mdwidget->exec();
      delete mdwidget;
      //      KMessageBox::information(this,QString("MD5 sum for "+url.filename()+" is:\n"+checkfile.hexDigest().data()));
    }
}


void KgpgApp::slotPreSignFile()
{
  //////////////////////////////////////   create a detached signature for a chosen file
  KURL url=KFileDialog::getOpenURL(QString::null,i18n("*|all files"), this, i18n("Open file to sign"));
 if (!url.isEmpty()) slotSignFile(url);
}

void KgpgApp::slotSignFile(KURL url)
{
  //////////////////////////////////////   create a detached signature for a chosen file
  QString signKeyID,signKeyMail;
 if (!url.isEmpty())
    {
      //////////////////   select a private key to sign file --> listkeys.cpp
	  KgpgSelKey *opts=new KgpgSelKey(this,"select_secret",false);
      opts->exec();
      if (opts->result())
        {
          signKeyID=opts->getkeyID();
          signKeyMail=opts->getkeyMail();
        }
      else
        {
          delete opts;
		  if (commandLineMode) slotExpressQuit();
          return;
        }
      delete opts;
  QString Options="";
  if ((pgpcomp) && (version>=120)) Options=" --pgp6 ";
 KgpgInterface *signFileProcess=new KgpgInterface();
 if (commandLineMode) connect (signFileProcess,SIGNAL(signfinished()),this,SLOT(slotExpressQuit()));
 signFileProcess->KgpgSignFile(signKeyMail,signKeyID,url,Options);
  }
  else if (commandLineMode) slotExpressQuit();
}

void KgpgApp::slotPreVerifyFile()
{
 KURL url=KFileDialog::getOpenURL(QString::null,
                                   i18n("*|all files"), this, i18n("Open File to verify"));
 slotVerifyFile(url);
}

void KgpgApp::slotVerifyFile(KURL url)
{
  ///////////////////////////////////   check file signature
  QString sigfile="";
 if (!url.isEmpty())
    {
      //////////////////////////////////////       try to find detached signature.
if (!url.filename().endsWith(".sig"))
{
      sigfile=url.path()+".sig";
      QFile fsig(sigfile.local8Bit());
      if (!fsig.exists())
        {
          sigfile=url.path()+".asc";
          QFile fsig(sigfile.local8Bit());
          //////////////   if no .asc or .sig signature file included, assume the file is internally signed
          if (!fsig.exists())
            sigfile="";
        }
}
      ///////////////////////// pipe gpg command
      KgpgInterface *verifyFileProcess=new KgpgInterface();
	  if (commandLineMode) connect (verifyFileProcess,SIGNAL(verifyfinished()),this,SLOT(slotExpressQuit()));
 verifyFileProcess->KgpgVerifyFile(url,KURL(sigfile));
 }
 else if (commandLineMode) slotExpressQuit();
}

///////////////////////////////////   key slots


void KgpgApp::slotClip()
{
  
  QString text;

  // Copy text from the clipboard (paste)
  text = kapp->clipboard()->text();
  if ( !text.isEmpty())
  {
  view->editor->setText(text);
  view->slotdecode();
  commandLineMode=false;
  }
  else {KMessageBox::sorry(this,i18n("Clipboard is empty"));exit(0);}
}




void KgpgApp::slotManageKey()
{
    /////////// open key management window --> listkeys.cpp	
   listKeys * keydialogue = new listKeys(this, "key_manager",WShowModal | WType_Dialog);
   keydialogue->show();
}


void KgpgApp::slotFileNew()
{
  //////  delete all text from editor

  view->editor->setText("");
  editRedo->setEnabled(false);
  editUndo->setEnabled(false);
  setCaption(i18n("untitled"), false);
  fileSave->setEnabled(false);
  Docname="";
}


void KgpgApp::slotFileOpen()
{

  KURL url=KFileDialog::getOpenURL(QString::null,
                                   i18n("*|All files"), this, i18n("Open File..."));
  if(!url.isEmpty())
    {
      openDocumentFile(url);
      Docname=url;
      fileSave->setEnabled(false);
      //fileSaveAs->setEnabled(true);
      setCaption(url.fileName(), false);
    }

}



void KgpgApp::slotFileSave()
{
  // slotStatusMsg(i18n("Saving file..."));

  QString filn=Docname.path();
  if (filn=="")
    {
      slotFileSaveAs();
      return;
    }
  QFile f(filn.local8Bit());
  if ( !f.open( IO_WriteOnly ) )
    {
      return;
    }

  QTextStream t( &f );
  t << view->editor->text();
  f.close();
  fileSave->setEnabled(false);
  setCaption(Docname.fileName(),false);
}


void KgpgApp::slotFileSaveAs()
{

  KURL url=KFileDialog::getSaveURL(QDir::currentDirPath(),
                                   i18n("*|All files"), this, i18n("Save as..."));
  if(!url.isEmpty())
    {

      QString filn=url.path();
      QFile f(filn.local8Bit());
      if (f.exists())
        {
          QString message=i18n("Overwrite existing file %1 ?").arg(url.filename());
          int result=KMessageBox::warningContinueCancel(this,QString(message),i18n("Warning"),i18n("Overwrite"));
          if (result==KMessageBox::Cancel)
            return;
        }

      if ( !f.open( IO_WriteOnly ) )
        {
          return;
        }

      QTextStream t( &f );
      t << view->editor->text();
      f.close();
      Docname=url;
      fileSave->setEnabled(false);
      setCaption(url.fileName(),false);
    }
}

void KgpgApp::slotFilePrint()
{}

void KgpgApp::slotExpressQuit()
{
  //kapp->
  exit(0);
}

void KgpgApp::slotFileQuit()
{
  saveOptions();
  // close the first window, the list makes the next one the first again.
  // This ensures that queryClose() is called on each window to ask for closing
  /*
    KMainWindow* w;
    if(memberList)
    {
      for(w=memberList->first(); w!=0; w=memberList->first())
      {
        // only close the window if the closeEvent is accepted. If the user presses Cancel on the saveModified() dialog,
        // the window and the application stay open.
        if(!w->close())
  	break;
      }
    }
    */
  kapp->exit(0);
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


void KgpgApp::fastencode(QString &selec,bool utrust,bool arm,bool shred,bool symetric)
{
  //////////////////              encode from file
  if ((selec==NULL) && (symetric==false))
    {
      KMessageBox::sorry(0,i18n("You have not choosen an encryption key..."));
      return;
    }

  KURL dest;

  if (arm==true) dest.setPath(urlselected.path()+".asc");
  else dest.setPath(urlselected.path()+".gpg");

  QFile fgpg(dest.path().local8Bit());

  if (fgpg.exists())
    {
      KgpgOverwrite *over=new KgpgOverwrite(0,"overwrite",dest);
      over->exec();
      if (over->result())
        dest.setFileName(over->getfname());
      else
        return;
    }
  QString encryptOptions="";

  if (utrust==true)
    encryptOptions+=" --always-trust ";
  if (arm==true)
    encryptOptions+=" --armor ";
  if (pgpcomp==true)
    {
      if (version<120)
        encryptOptions+=" --compress-algo 1 --cipher-algo cast5 ";
      else
        encryptOptions+=" --pgp6 ";
    }
  KgpgInterface *cryptFileProcess=new KgpgInterface();
  cryptFileProcess->KgpgEncryptFile(selec,urlselected,dest,encryptOptions,symetric);
  //KgpgEncryptFile *cryptFileProcess=new KgpgEncryptFile(selec,urlselected,dest,symetric,encryptOptions);
  if (shred==true)
    connect(cryptFileProcess,SIGNAL(encryptionfinished(bool)),this,SLOT(shredprocessenc(bool)));
  else
    connect(cryptFileProcess,SIGNAL(encryptionfinished(bool)),this,SLOT(processenc(bool)));
}

void KgpgApp::shredprocessenc(bool res)
{

  if (res==false)
    {
      KMessageBox::sorry(0,i18n("There was an error encrypting the file\nChek the key and your permissions"));
if (commandLineMode==true) kapp->exit(0);
    }
  else
    {
      //KMessageBox::sorry(0,"shred");
      KShred *shredres=new KShred(urlselected.path().local8Bit());
      if (shredres->shred()==false) KMessageBox::sorry(0,i18n("The source file could not be shredded\nChek your permissions"));
if (commandLineMode==true) kapp->exit(0);
    }
}


void KgpgApp::processenc(bool res)
{
  if (res==false)
    KMessageBox::sorry(0,i18n("There was an error encrypting the file\nChek the key and your permissions"));
  if (commandLineMode==true)
    kapp->exit(0);
}


void KgpgApp::fastdecode(bool quit)
{
  //////////////////////////////////////////////////////////////////    decode file from konqueror or menu

  fastact=quit;
  QString oldname=urlselected.filename().local8Bit();
  messages="";
  KProcIO *encid=new KProcIO();
  *encid << "gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--batch"<<"--status-fd=1"<<"-d"<<urlselected.path().local8Bit();
  /////////  when process ends, update dialog infos
  QObject::connect(encid, SIGNAL(processExited(KProcess *)),this, SLOT(slotprocresult(KProcess *)));
  QObject::connect(encid, SIGNAL(readReady(KProcIO *)),this, SLOT(slotprocread(KProcIO *)));
  encid->start(KProcess::NotifyOnExit,false);
}

/////////////////////////////////////////////////

void KgpgApp::slotprocresult(KProcess *)
{

  QString newname="",enckey="";
  QCString password;


  QString oldname=urlselected.filename().local8Bit();

  enckey=messages;
  if (oldname.endsWith(".gpg"))
    oldname.truncate(oldname.length()-4);
  else if (oldname.endsWith(".asc"))
    oldname.truncate(oldname.length()-4);
  else if (oldname.endsWith(".pgp"))
    oldname.truncate(oldname.length()-4);
  else oldname.append(".clear");
  KURL swapname(urlselected.directory(0,0)+oldname);

  if (fastact==false)
    {
      popupName *popn=new popupName(i18n("Decryption to"), this, "decryption to", swapname);
      popn->exec();
      if (popn->result()==true)
        {
          if (popn->checkFile->isChecked())
            newname=popn->newFilename->text();
          else
            newname="";
          //ascii=popn->getascii();
        }

      else
        return;
      delete popn;
    }
  else
    {
      ////////////////// fast mode
      newname=swapname.path();
    }
  if (newname!="")
    {
      QFile fgpg(newname.local8Bit());
      if (fgpg.exists())
        {
          KgpgOverwrite *over=new KgpgOverwrite(0,"overwrite",KURL(newname));
          over->exec();
          if (over->result()==true)
            {
              if (fastact==true)
                newname=swapname.directory(0,0)+over->getfname();
              else
                newname=KURL(newname).directory(0,0)+over->getfname();
            }
          else
            {
              if (fastact==true)
                exit(1);
              else
                return;
            }
        }
    }
  /*
  QFile fgpg(newname);
  if (fgpg.exists())
    fgpg.remove();
*/
  KgpgInterface *decryptFileProcess=new KgpgInterface();
  int decresult=0;
  decpassuid=messages;
  decpasssrc=urlselected;
  if (newname!="") ////////////////////   decrypt to file
  {
  decpassdest=KURL(newname);
  decresult=decryptFileProcess->KgpgDecryptFile(decpassuid,decpasssrc,decpassdest);
  if (decresult==0) {if (fastact==true) kapp->exit(0); else return;}
  connect(decryptFileProcess,SIGNAL(decryptionfinished(bool)),this,SLOT(processdecover(bool)));
  connect(decryptFileProcess,SIGNAL(badpassphrase(bool)),this,SLOT(processdec(bool)));
  }
  else openEncryptedDocumentFile(urlselected,messages);
}

void KgpgApp::processdecover(bool res)
{
  if ((res==true) && (fastact==true)) kapp->exit(0);
  if (res==false)
  {
  KMessageBox::sorry(0,i18n("Decryption failed..."));
  if (fastact==true) kapp->exit(0);
  }
}


void KgpgApp::processdec(bool res)
{
  //if ((res==true) && (fastact==true)) kapp->exit(0);
  if (res==false)
  {
   KgpgInterface *decryptFileProcess=new KgpgInterface();
  int decresult=0;
  decresult=decryptFileProcess->KgpgDecryptFile(decpassuid,decpasssrc,decpassdest,2);
  if (decresult==0) {if (fastact==true) kapp->exit(0); else return;}
  connect(decryptFileProcess,SIGNAL(decryptionfinished(bool)),this,SLOT(processdecover(bool)));
  connect(decryptFileProcess,SIGNAL(badpassphrase(bool)),this,SLOT(processdec2(bool)));
 }
}

void KgpgApp::processdec2(bool res)
{
  //if ((res==true) && (fastact==true)) kapp->exit(0);
  if (res==false)
  {
   KgpgInterface *decryptFileProcess=new KgpgInterface();
  int decresult=0;
  decresult=decryptFileProcess->KgpgDecryptFile(decpassuid,decpasssrc,decpassdest,1);
  if (decresult==0) {if (fastact==true) kapp->exit(0); else return;}
  connect(decryptFileProcess,SIGNAL(decryptionfinished(bool)),this,SLOT(processdecover(bool)));
  connect(decryptFileProcess,SIGNAL(badpassphrase(bool)),this,SLOT(processdecover(bool)));
 }
}

void KgpgApp::processdec3(bool res)
{
  if (res==false)
    KMessageBox::sorry(0,i18n("There was an error decrypting the file\nChek the key and your permissions"));
 if (fastact==true) kapp->exit(0);
}




void KgpgApp::slotprocread(KProcIO *p)
{
  ///////////////////////////////////////////////////////////////// extract  encryption keys
  QString outp;
  while (p->readln(outp)!=-1)
    {
      if (outp.find("<")!=-1)
        {
          outp=outp.section('<',-1,-1);
          outp=outp.section('>',0,0);
          if (messages!="")
            messages+=" or ";
          messages+=outp;
        }
    }
}

void KgpgApp::slotFileEnc()
{
  /////////////////////////////////////////////////////////////////////////  encode file file

  KURL url=KFileDialog::getOpenURL(QString::null,
                                   i18n("*|all files"), this, i18n("Open File to encode"));
  if (!url.isEmpty())
    {
      urlselected=url;
      if (encryptfileto==false)
        {
          popupPublic *dialogue=new popupPublic(this,"Public keys",url.filename(),true);
          connect(dialogue,SIGNAL(selectedKey(QString &,bool,bool,bool,bool)),this,SLOT(fastencode(QString &,bool,bool,bool,bool)));
          dialogue->exec();
          delete dialogue;
        }
      else
        fastencode(filekey,untrusted,ascii,false,false);
    }
}

void KgpgApp::slotFileDec()
{
  /////////////////////////////////////////////////////////////////////////  decode file file

  KURL url=KFileDialog::getOpenURL(QString::null,
                                   i18n("*|all files"), this, i18n("Open File to decode"));
  if (!url.isEmpty())
    {
      urlselected=url;
      fastdecode(false);
    }
}
#include "kgpg.moc"

