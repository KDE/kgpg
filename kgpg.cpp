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
#include <qfile.h>
#include <stdlib.h>

// include files for KDE
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kmenubar.h>

#include <klocale.h>
#include <kconfig.h>
#include <kstdaction.h>
#include <kpopupmenu.h>
#include <ktip.h>


#include <qapplication.h>
#include <qclipboard.h>

// application specific includes

#include "kgpgfast.h"

#include "kgpg.h"

#define ID_STATUS_MSG 1

KgpgApp::KgpgApp(QWidget* parent, const char* name,KURL fileToOpen,bool encmode,bool decmode,bool clipmode):KMainWindow(0, name)
{
  config=kapp->config();
  readOptions();
  checkVersion();
  if (tipofday==true) slotTip();
  if ((encmode==false) && (decmode==false))
    {
      KIconLoader *loader = KGlobal::iconLoader();
      fileEnc=loader->loadIcon("kgpg",KIcon::Small);
      fileDec=loader->loadIcon("kgpg2",KIcon::Small);
      ///////////////////////////////////////////////////////////////////
      // call inits to invoke all other construction parts

      //initStatusBar();
      initActions();
      //  initDocument();
      KMenuBar *menubar=KgpgApp::menuBar();

      /// Add "keys" menu;

      QPopupMenu *popupfile=new QPopupMenu();


      /// Add custom "signature" menu;
      //QPopupMenu *popupsig=new QPopupMenu();
      //popupsig->insertItem(i18n("&Test"),this,SLOT(slotTest()));
//      popupsig->insertItem(i18n("&Generate signature"),this,SLOT(slotSignFile()));
//      popupsig->insertItem(i18n("&Verify signature"),this,SLOT(slotVerifyFile()));
//      popupsig->insertItem(i18n("&Check MD5 sum"),this,SLOT(slotCheckMd5()));
//      menubar->insertItem(i18n("&Signature"),popupsig);

      KPopupMenu *help = helpMenu();
      helptips->plug(help);
      manpage->plug(help);
      menubar->insertItem( i18n("&Help"), help);



      initView();
      if (clipmode==true)   slotClip();
      view->gpgversion=version;
      //statusBar()->hide();
    }

  if (fileToOpen.filename()!="")
    {
      urlselected=fileToOpen;
      if (encmode==true) ///// user asked for encryption in command line arguments
        {
          if (encryptfileto==false)
            {
              popupPublic *dialogue=new popupPublic(this,i18n("Public Keys"),fileToOpen.filename(),true,ascii,untrusted,encrypttodefault,defaultkey);
              connect(dialogue,SIGNAL(selectedKey(QString &,bool,bool,bool,bool)),this,SLOT(fastencode(QString &,bool,bool,bool,bool)));
              dialogue->exec();
              delete dialogue;
            }
          else fastencode(filekey,untrusted,ascii,false,false);
          exit(1);
        }
      else if (decmode==true)
        {
          fastdecode(true);  ///// user asked for decryption in command line arguments
          //exit(1);
        }

      else view->editor->droppedfile(urlselected);

    }
  if (decmode==false)
    {
      ///////////////////////////////////////////////////////////////////
      // disable actions at startup
      fileSave->setEnabled(false);
      //fileSaveAs->setEnabled(false);
      editRedo->setEnabled(false);
      editUndo->setEnabled(false);
    }

  createGUI("kgpg.rc");
}

KgpgApp::~KgpgApp()
{}

void KgpgApp::slotman()
{

KProcess *conprocess=new KProcess();
*conprocess<< "konsole"<<"-e"<<"man"<<"gpg";
conprocess->start(KProcess::NotifyOnExit,KProcess::AllOutput);
}

void KgpgApp::slotTip()
{
  KTipDatabase *tip=new KTipDatabase("tips");
  KTipDialog::setShowOnStart(true);
  KTipDialog::showTip();
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
  
  keysManage = new KAction(i18n("&Manage keys"), "kgpg_manage", 0,this, SLOT(slotManageKey()), actionCollection(),"keys_manage");

  signGenerate = new KAction(i18n("&Generate Signature..."),0, this, SLOT(slotSignFile()), actionCollection(), "sign_generate");
  signVerify = new KAction(i18n("&Verify Signature..."),0, this, SLOT(slotVerifyFile()), actionCollection(), "sign_verify");
  signCheck = new KAction(i18n("&Check MD5 sum..."), 0,this, SLOT(slotCheckMd5()), actionCollection(), "sign_check");

  KStdAction::preferences(this, SLOT(slotOptions()), actionCollection());
  
  helptips = new KAction(i18n("Tip of the &Day..."), "idea", 0,this, SLOT(slotTip()), actionCollection(),"help_tipofday");
  manpage = new KAction(i18n("View GnuPG manual"), "contents", 0,this, SLOT(slotman()),0);
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







void KgpgApp::openDocumentFile(const KURL& url)
{

  /////////////////////////////////////////////////

  QFile qfile(url.path());

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
  config->writeEntry("Ascii armor",ascii);
  config->writeEntry("Allow untrusted keys",untrusted);
  config->writeEntry("PGP compatibility",pgpcomp);
  config->writeEntry("encrypt to default key",encrypttodefault);
  config->writeEntry("default key",defaultkey);
  config->writeEntry("encrypt files to",encryptfileto);
  config->writeEntry("file key",filekey);
  //  config->writeEntry("Decrypt files to editor",edecrypt);
}







void KgpgApp::readOptions()
{
  config->setGroup("General Options");
  bool frun=config->readBoolEntry("First run",true);
  ascii=config->readBoolEntry("Ascii armor",true);
  untrusted=config->readBoolEntry("Allow untrusted keys",false);
  pgpcomp=config->readBoolEntry("PGP compatibility",false);
  encrypttodefault=config->readBoolEntry("encrypt to default key",false);
  defaultkey=config->readEntry("default key");
  encryptfileto=config->readBoolEntry("encrypt files to",false);
  filekey=config->readEntry("file key");
  QSize size=config->readSizeEntry("Geometry");
  if(!size.isEmpty())
    {
      resize(size);
    }
  if (frun==true) firstrun();
  config->setGroup("TipOfDay");
  tipofday=config->readBoolEntry("RunOnStart",true);

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
  fp = popen("gpg --no-tty --with-colon --list-secret-keys", "r");
  while ( fgets( line, sizeof(line), fp))
    {
      tst=line;
      if (tst.find("sec",0,FALSE)!=-1) found=true;
    }
  pclose(fp);
  if (found==false)
    {
      int result=KMessageBox::questionYesNo(0,i18n("Welcome to KGPG.\nNo secret key was found on your computer.\nWould you like to create one now ?"));
      if (result==3)
        {
          listKeys *creat=new listKeys(this,i18n("Key Management"),encrypttodefault,defaultkey);
          creat->slotgenkey();
          delete creat;
        }
    }
}




void KgpgApp::slotOptions()
{
  /////////////////////// open option dialog box --> kgpgoptions.cpp
  kgpgOptions *opts=new kgpgOptions(this,0,ascii,untrusted,pgpcomp,encryptfileto,encrypttodefault,filekey,defaultkey);

  if (opts->exec()==QDialog::Accepted)
    {
      ascii=opts->getascii();
      untrusted=opts->getuntrusted();
      pgpcomp=opts->getpgp();
      encrypttodefault=opts->defaultenc();
      defaultkey=opts->getdefkey();
      encryptfileto=opts->fileenc();
      filekey=opts->getfilekey();

      view->pubascii=ascii;
      view->pubuntrusted=untrusted;
      view->pubpgp=pgpcomp;
      view->pubencrypttodefault=encrypttodefault;
      view->pubdefaultkey=defaultkey;
    }
  //delete opts;
  opts->delayedDestruct();

  saveOptions();
}

////////////////////////////////////////////////////    tree view slots

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

  QString tst="",md;
  char gpgcmd[1024] = "\0",line[130]="";

  FILE *fp;

  KURL url=KFileDialog::getOpenURL(QString::null,
                                   i18n("*|all files"), this, i18n("Open file to verify"));
  if (!url.isEmpty())
    {
      strcat(gpgcmd,QString("gpg --print-md MD5 "+url.path()));
      fp=popen(gpgcmd,"r");
      while ( fgets( line, sizeof(line), fp))
        {
          md=line;
          if (md.startsWith("gpg")==false) tst+=md;
        }
      pclose(fp);

      KMessageBox::information(this,QString("MD5 sum for "+url.filename()+" is:\n"+tst.section(':',1,1).stripWhiteSpace()));
    }
}


void KgpgApp::slotSignFile()
{
  //////////////////////////////////////   create a detached signature for a chosen file

  QString signKey;
  char gpgcmd[1024] = "\0";

  FILE *fp,*pass;
  int ppass[2];
  QCString password;

  KURL url=KFileDialog::getOpenURL(QString::null,i18n("*|all files"), this, i18n("Open file to sign"));
  if (!url.isEmpty())
    {
      //////////////////   select a private key to sign file --> listkeys.cpp
      KgpgSelKey *opts=new KgpgSelKey(this,0,false);
      opts->exec();
      if (opts->result()==true) signKey=opts->getkey();
      else
        {
          delete opts;
          return;
        }
      delete opts;
      /////////////////////  get passphrase
      //int code=KPasswordDialog::getPassword(password,QString("Enter passphrase for "+signKey+":"));
      int code=KPasswordDialog::getPassword(password,i18n("Enter passphrase for %1:").arg(signKey));
      if (code!=QDialog::Accepted) return;

      pipe(ppass);
      pass = fdopen(ppass[1], "w");
      fwrite(password, sizeof(char), strlen(password), pass);
      fwrite("\n", sizeof(char), 1, pass);
      fclose(pass);

      /////////////       create gpg command
      snprintf(gpgcmd, 200, "gpg --no-tty --no-armor --passphrase-fd %d -u ",ppass[0]);
      strcat(gpgcmd,signKey);
      if ((pgpcomp==true) && (version>=120)) strcat(gpgcmd," --pgp6 ");
      strcat(gpgcmd," --detach-sig ");
      strcat(gpgcmd,url.path());

      //KMessageBox::sorry(this,gpgcmd);

      /////////         open gpg pipe
      QString signame=url.path()+".sig";
      QFile fsig(signame);
      if (fsig.exists()) fsig.remove();
      fp=popen(gpgcmd,"r");
      pclose(fp);
      if (fsig.exists())
        KMessageBox::information(this,i18n("The signature file %1 was successfully created").arg(signame));
      else
        KMessageBox::sorry(this,i18n("The signature could not be created.\nCheck passphrase & permissions"));
    }
}

void KgpgApp::slotVerifyFile()
{
  ///////////////////////////////////   check file signature
  QString sigfile="",tst="";
  char gpgcmd[1024] = "\0",line[130]="";

  FILE *fp,*pass,*stat;
  int ppass[2];
  QCString password;

  KURL url=KFileDialog::getOpenURL(QString::null,
                                   i18n("*|all files"), this, i18n("Open File to verify"));
  if (!url.isEmpty())
    {
      //////////////////////////////////////       try to find detached signature.

      sigfile=url.path()+".sig";
      QFile fsig(sigfile);
      if (!fsig.exists())
        {
          sigfile=url.path()+".asc";
          QFile fsig(sigfile);
          //////////////   if no .asc or .sig signature file included, assume the file is internally signed
          if (!fsig.exists()) sigfile="";
        }

      ///////////////////////// pipe gpg command
      pipe(ppass);
      stat = fdopen(ppass[1], "w");

      snprintf(gpgcmd,130,"gpg --no-tty --logger-fd %d --no-secmem-warning --verify ",ppass[1]);
      if (sigfile!="") strcat(gpgcmd,QString(sigfile+" "));
      else strcat(gpgcmd,QString(url.path()+" "));
      //strcat(gpgcmd,url.path());
      fp=popen(gpgcmd,"r");
      pclose(fp);
      fclose(stat);

      int Len;
      char Buff[500]="\0";

      //////////////////////////   read gpg output
      while (read(ppass[0], &Len, sizeof(Len)) > 0)
        {
          read(ppass[0],Buff, Len);
          tst+=Buff;
        }
      if (tst.find("Good signature",0,FALSE)!=-1) KMessageBox::information(this,tst);
      else if (tst.find("no valid",0,FALSE)!=-1)
        KMessageBox::sorry(this,i18n("No signature found for this file.\nIf you have a detached signature, save it to %1.sig").arg(url.path()));
      else KMessageBox::sorry(this,tst);
    }
}

///////////////////////////////////   key slots


void KgpgApp::slotClip()
{
  QClipboard *cb = QApplication::clipboard();
  QString text;

  // Copy text from the clipboard (paste)
  text = cb->text();
  if ( text )
    view->editor->setText(text);
  view->popuppass();
  //KMessageBox::sorry(0,text);

}




void KgpgApp::slotManageKey()
{
  /////////// open key management window --> listkeys.cpp

  listKeys *dialogue=new listKeys(this,i18n("Key Management"),encrypttodefault,defaultkey);
  dialogue->show();
  //connect(dialogue,SIGNAL(selectedKey(QString &)),this,SLOT(encode(QString &)));
  //  dialogue->exec();
  //  delete dialogue;
}


void KgpgApp::slotFileNew()
{
  //////  delete all text from editor

  //  slotStatusMsg(i18n("Creating new document..."));
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
  QFile f(filn);
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


      //KMessageBox::sorry(this,filn);
      //     return;

      QFile f(filn);

      if (f.exists())
        {
          QString message=i18n("Overwrite existing file %1 ?").arg(url.filename());
          int result=KMessageBox::warningContinueCancel(this,QString(message),i18n("Warning"),i18n("Overwrite"));
          if (result==KMessageBox::Cancel) return;
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
{
  /*
   
    QPrinter printer;
    if (printer.setup(this))
    {
      view->print(&printer);
    }
   
  */
}

void KgpgApp::slotFileQuit()
{
  // slotStatusMsg(i18n("Exiting..."));
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
{}





void KgpgApp::slotEditCopy()
{}





void KgpgApp::slotEditPaste()
{}


void KgpgApp::fastencode(QString &selec,bool utrust,bool arm,bool shred,bool symetric)
{
  //////////////////              encode from file
  if ((selec==NULL) && (symetric==false))
    {
      KMessageBox::sorry(0,i18n("You have not choosen an encryption key..."));
      return;
    }

  FILE *fp;

  QString tst="",gpgfile="",gpgfilename="";
  char line[200]="";

  if (arm==true)
    {
      gpgfile=urlselected.path()+".asc";
      gpgfilename=urlselected.filename()+".asc";
    }
  else
    {
      gpgfile=urlselected.path()+".gpg";
      gpgfilename=urlselected.filename()+".gpg";
    }
  //dest=gpgfile;
  KURL dest(urlselected.directory(0,0)+gpgfilename);
  QFile fgpg(gpgfile);

  if (fgpg.exists())
    {
      KgpgOverwrite *over=new KgpgOverwrite(0,i18n("overwrite"),dest);
      over->exec();
      if (over->result()==true)
        {
          dest.setFileName(over->getfname());
        }
      else return;
    }

  if (symetric==true)
    {
      QCString password;
      int code=KPasswordDialog::getNewPassword(password,QString(i18n("Enter passphrase for file %1(symmetrical encryption):").arg(urlselected.filename())));
      if (code==QDialog::Accepted)
        strcat(line,"echo \"");
      strcat(line, password);
      strcat(line,"\" | ");
    }
  strcat(line,"gpg --no-tty ");
  //////////   encode with untrusted keys or armor if checked by user
  if (utrust==true) strcat(line,"--always-trust ");
  if (arm==true) strcat(line,"--armor ");
  if (pgpcomp==true)
    {
      if (version<120) strcat(line,"--compress-algo 1 --cipher-algo cast5 ");
      else strcat(line,"--pgp6 ");
    }
  strcat(line,"--output ");
  strcat(line,dest.path());
  if (symetric==false)
    {
      strcat(line," -er ");
      strcat(line,selec);
      if (encrypttodefault==true)
        {
          strcat(line," --recipient ");
          strcat(line,defaultkey);
        }
    }
  else  strcat(line," --passphrase-fd 0 -c ");
  strcat(line," ");
  strcat(line,urlselected.path());

  fp = popen(line, "r");
  /*
  while ( fgets( line, sizeof line, fp))
  tst+=line;
  */
  pclose(fp);
  if (!fgpg.exists())
    KMessageBox::sorry(this,i18n("There was an error encrypting the file\nChek the key and your permissions"));
  else if (shred==true)
    {
      QString mssg=i18n("File %1 was created and encrypted with %2.\n").arg(gpgfilename).arg(selec);
      mssg+=i18n("File %1 is about to be shredded.\n").arg(urlselected.filename());
      mssg+=i18n("[beware: shred may not be 100% secure on some filesystems]");
      int result=KMessageBox::warningContinueCancel(this,mssg,i18n("Warning"));
      if (result==KMessageBox::Cancel) return;
      char shredcmd[200]="";
      strcat(shredcmd,"shred -zu ");
      strcat(shredcmd,urlselected.path());
      fp = popen(shredcmd, "r");
      pclose(fp);
      QFile fsource(urlselected.path());
      if (fsource.exists())
        KMessageBox::sorry(this,i18n("The source file could not be shredded\nChek your permissions"));
    }

}


void KgpgApp::fastdecode(bool quit)
{
  //////////////////////////////////////////////////////////////////    decode file from konqueror or menu

  fastact=quit;
  QString oldname=urlselected.filename();
  messages="";
  KProcIO *encid=new KProcIO();
  *encid << "gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--batch"<<"-d"<<urlselected.path();
  /////////  when process ends, update dialog infos
  QObject::connect(encid, SIGNAL(processExited(KProcess *)),this, SLOT(slotprocresult(KProcess *)));
  QObject::connect(encid, SIGNAL(readReady(KProcIO *)),this, SLOT(slotprocread(KProcIO *)));
  encid->start(KProcess::NotifyOnExit,true);
}

/////////////////////////////////////////////////

void KgpgApp::slotprocresult(KProcess *p)
{
  FILE *output,*pass,*stat;
  QFile f;
  QString tst="",newname="",enckey="";
  char gpgcmd[1024] = "\0",line[130]="";
  int ppass[2];
  QCString password;


  QString oldname=urlselected.filename();

  //KMessageBox::information(0,messages);
  enckey=messages;
  /*
  if (enckey=="")
    {
      KMessageBox::sorry(0,i18n("No encrypted data found...\nMaybe this is a signature or key file"));
      if (fastact==true) exit(1);
      else return;
    }
*/
  if (oldname.endsWith(".gpg")) oldname.truncate(oldname.length()-4);
  if (oldname.endsWith(".asc")) oldname.truncate(oldname.length()-4);
  if (oldname.endsWith(".pgp")) oldname.truncate(oldname.length()-4);

  KURL swapname(urlselected.directory(0,0)+oldname);

  if (fastact==false)
    {
      popupName *popn=new popupName(this,i18n("Decryption to"),swapname);
      //connect(dialogue,SIGNAL(selectedKey(QString &,bool,bool)),this,SLOT(encode(QString &,bool,bool)));
      popn->exec();
      if (popn->result()==true)
        {
          if (popn->getfmode()==true)  newname=popn->getfname();
          else newname="";
          //ascii=popn->getascii();
        }

      else return;
      delete popn;
    }
  else
    {
      ////////////////// fast mode
            newname=swapname.path();
}
    if (newname!="")
        {
      QFile fgpg(newname);
      if (fgpg.exists())
        {
          KgpgOverwrite *over=new KgpgOverwrite(0,i18n("overwrite"),KURL(newname));
          over->exec();
          if (over->result()==true)
            {
             if (fastact==true) newname=swapname.directory(0,0)+over->getfname();
	     else newname=KURL(newname).directory(0,0)+over->getfname();
            }
          else  
	  {
	  if (fastact==true) exit(1);
	  else return;
	  }
        }
    }
  QFile fgpg(newname);
  if (fgpg.exists()) fgpg.remove();
  bool success=false;


  int counter=0;
  while ((counter<3) && (success==false))
    {
      /// pipe for passphrase
      counter++;
      QString passdlg;
      if (enckey=="") passdlg=i18n("Enter passphrase for file %1(symmetrical encryption):").arg(urlselected.filename());
      else passdlg=i18n("Enter passphrase for %1:").arg(messages);
      if (counter>1) passdlg.prepend(i18n("<b>Bad passphrase</b><br> You have %1 trial(s) left.<br>").arg(QString::number(4-counter)));

      /// pipe for passphrase
      int code=KPasswordDialog::getPassword(password,passdlg);
      if (code!=QDialog::Accepted)
        {
          if (fastact==true) exit(1);
          else return;
        }

      //   pass=password;

      pipe(ppass);
      pass = fdopen(ppass[1], "w");
      fwrite(password, sizeof(char), strlen(password), pass);
      fwrite("\n", sizeof(char), 1, pass);
      fclose(pass);

      /// create gpg command
      if (newname!="") // a filename was entered
        {
          snprintf(gpgcmd, 130, "gpg --no-secmem-warning --no-tty --passphrase-fd %d -o ",ppass[0]);
          //    strcat(gpgcmd,urlselected.directory(0,0));
          strcat(gpgcmd,newname);
          strcat(gpgcmd," -d ");
          strcat(gpgcmd,urlselected.path());
        }
      else //// no filename -> decrypt to editor
        {
          snprintf(gpgcmd, 130, "gpg --no-secmem-warning --no-tty --passphrase-fd %d -d ",ppass[0]);
          strcat(gpgcmd,urlselected.path());
        }


      //KMessageBox::sorry(this,QString(gpgcmd));
      /// open gpg pipe
      tst="";
      output=popen(gpgcmd,"r");
      while ( fgets( line, sizeof(line), output))    /// read output
        tst+=line;
      pclose(output);

      if  (newname=="")
        {
          if (tst!="")
            {
              success=true;
              view->editor->setText(tst);
              editRedo->setEnabled(false);
              editUndo->setEnabled(false);
              //view->modified();
            }

        }
      else
        {
          QFile fcheck(newname);
          //KMessageBox::sorry(this,clearfile);
          if (fcheck.exists())
            {
              if (fastact==true) exit(1);
              else success=true;
            }
        }
    }
  if (success==false) KMessageBox::sorry(0,i18n("Decryption not possible: bad passphrase, missing key or corrupted file"));
  if (fastact==true) exit(1);

}

void KgpgApp::slotprocread(KProcIO *p)
{
  ///////////////////////////////////////////////////////////////// extract  encryption keys
  QString outp;
  while (p->readln(outp)!=-1)
    {
      if (outp.find("<")!=-1)
        {
          outp=outp.section('<',1,1);
          outp=outp.section('>',0,0);
          if (messages!="") messages+=" or ";
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
          popupPublic *dialogue=new popupPublic(this,i18n("Public keys"),url.filename(),true,ascii,untrusted,encrypttodefault,defaultkey);
          connect(dialogue,SIGNAL(selectedKey(QString &,bool,bool,bool,bool)),this,SLOT(fastencode(QString &,bool,bool,bool,bool)));
          dialogue->exec();
          delete dialogue;
        }
      else fastencode(filekey,untrusted,ascii,false,false);
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
