/***************************************************************************
                          kgpgview.cpp  -  description
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

//////////////////////////////////////////////////////////////  code for the main view (editor)

// include files for Qt
//#include <qprinter.h>

// application specific includes

#include <qscrollview.h>

#include <./kio/netaccess.h>
#include <kstdaction.h>

#include "kgpginterface.h"
#include "popupname.h"
#include "kgpgview.h"
#include "kgpg.h"


//////////////// configuration for editor

MyEditor::MyEditor( QWidget *parent, const char *name )
    : QTextEdit( parent, name )
{
setTextFormat(PlainText);
setAcceptDrops(true);
}

void MyEditor::contentsDragEnterEvent( QDragEnterEvent *e )
{
  ////////////////   if a file is dragged into editor ...
  e->accept (QUrlDrag::canDecode(e) || QTextDrag::canDecode (e));
  //e->accept (QTextDrag::canDecode (e));
}




void MyEditor::contentsDropEvent( QDropEvent *e )
{
  /////////////////    decode dropped file
  QStringList list;
  QString text;
  if ( QUrlDrag::decodeToUnicodeUris( e, list ) ) droppedfile(KURL(list.first()));
  else if ( QTextDrag::decode(e, text) ) insert(text);
}

void MyEditor::droppedfile(KURL url)
{
  /////////////////    decide what to do with dropped file
  QString text;

  if (url.isLocalFile())
    tempFile = url.path();
  else if (!KIO::NetAccess::download (url, tempFile))
  {
    KMessageBox::sorry(this,i18n("Could not download file."));
    return;
  }

  QFile qfile(tempFile);

  if (qfile.open(IO_ReadOnly))
  {
    /////////////  if dropped filename ends with gpg, pgp or asc, try to decode it
    if ((tempFile.endsWith(".gpg")) || (tempFile.endsWith(".asc")) || (tempFile.endsWith(".pgp"))) decodef(tempFile);
    //////////   else open file
    else
    {
      QTextStream t( &qfile );
      QString result(t.read());
      //////////////     if  pgp data found, decode it
      if (result.startsWith("-----BEGIN PGP"))
      {
        qfile.close();
        decodef(tempFile);
      }
      else
      {
        setText(result);
        qfile.close();
		KIO::NetAccess::removeTempFile(tempFile);
      }
    }
  }
}

void MyEditor::decodef(QString fname)
{
  ////////////////     decode file from given url into editor
filename=fname;
  //////  trick to find with which key the file was encrypted. decrypt it with no passphrase & read gpg output

    messages="";
  KProcIO *encid=new KProcIO();
  *encid << "gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--batch"<<"--status-fd=1"<<"-d"<<fname.local8Bit();
  /////////  when process ends, update dialog infos
    QObject::connect(encid, SIGNAL(processExited(KProcess *)),this, SLOT(slotprocresultenckey(KProcess *)));
    QObject::connect(encid, SIGNAL(readReady(KProcIO *)),this, SLOT(slotprocreadenckey(KProcIO *)));
    encid->start(KProcess::NotifyOnExit,false);
 }

void MyEditor::slotprocreadenckey(KProcIO *p)
{
///////////////////////////////////////////////////////////////// extract  encryption keys

  QString outp;
  while (p->readln(outp)!=-1)
  {
    if (outp.find("<")!=-1)
{
outp=outp.section('<',-1,-1);
outp=outp.section('>',0,0);
if (messages!="") messages+=i18n(" or ");
messages+=outp;
}
  }
}

void MyEditor::slotprocresultenckey(KProcess *)
{
    QString enckey;
    enckey=messages;

  if (enckey.isEmpty())
  {
    QFile qfile(filename.local8Bit());

    if (qfile.open(IO_ReadOnly))
    {
      QTextStream t( &qfile );
      QString result(t.read());
      //////////////     if  pgp data found, decode it
      if (result.startsWith("-----BEGIN PGP PUBLIC KEY BLOCK"))
      {//////  dropped file is a public key, ask for import
        qfile.close();

        int result=KMessageBox::warningContinueCancel(this,i18n("The file %1 is a public key.\nDo you want to import it ?").arg(filename),i18n("Warning"));
        if (result==KMessageBox::Cancel) return;
        else
        {
	message="";
                  KProcIO *conprocess=new KProcIO();
	  *conprocess<< "gpg";
	  *conprocess<<"--no-tty"<<"--no-secmem-warning"<<"--import"<<QFile::encodeName(filename);
          QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(slotprocresult(KProcess *)));
          QObject::connect(conprocess, SIGNAL(readReady(KProcIO *)),this, SLOT(slotprocread(KProcIO *)));
        conprocess->start(KProcess::NotifyOnExit,true);
        return;
	}
      }
      else
       {
       if (result.startsWith("-----BEGIN PGP PRIVATE KEY BLOCK"))
       KMessageBox::information(0,i18n("This file is a private key !\nPlease use kgpg key management to import it."));
       else
       /// unknown file type
	   if (!result.startsWith("-----BEGIN PGP MESSAGE"))
	   		{
      		KMessageBox::sorry(0,i18n("Sorry, no encrypted data found..."));
      		return;
			}
			else enckey=i18n("[No user ID found]");
      }
    }
  }


 //KMessageBox::sorry(0,text);
   QFile qfile(filename);

if (qfile.open(IO_ReadOnly))
{
      QTextStream t( &qfile );
      QString result(t.read());

QString resultat=KgpgInterface::KgpgDecryptText(result,enckey);
KIO::NetAccess::removeTempFile(tempFile);
if (resultat!="") setText(resultat);
else KMessageBox::sorry(this,i18n("Decryption not possible: bad passphrase, missing key or corrupted file"));
}
 else KMessageBox::sorry(this,i18n("Unable to read file..."));

}


void MyEditor::slotprocresult(KProcess *)
{
KIO::NetAccess::removeTempFile(tempFile);
  KMessageBox::information(0,message);
}

void MyEditor::slotprocread(KProcIO *p)
{
QString outp;
while (p->readln(outp)!=-1)
{
if (outp.find("http-proxy")==-1)
message+=outp+"\n";
}
}

////////////////////////// main view configuration

KgpgView::KgpgView(QWidget *parent, const char *name) : QWidget(parent, name)
{
  viewreadopts();
  editor=new MyEditor(this);

  /////    layout

  QVBoxLayout *vbox=new QVBoxLayout(this,3);

  editor->setReadOnly( false );
  editor->setUndoRedoEnabled(true);
  editor->setUndoDepth(5);

  setAcceptDrops(true);

  KButtonBox *boutonbox=new KButtonBox(this,KButtonBox::Horizontal,15,12);
  boutonbox->addStretch(1);

  bouton0=boutonbox->addButton(i18n("S&ign/Verify"),this,SLOT(clearSign()),TRUE);
  bouton1=boutonbox->addButton(i18n("En&crypt"),this,SLOT(popuppublic()),TRUE);
  bouton2=boutonbox->addButton(i18n("&Decrypt"),this,SLOT(slotdecode()),TRUE);

  QObject::connect(editor,SIGNAL(textChanged()),this,SLOT(modified()));

  boutonbox->layout();
  editor->resize(editor->maximumSize());
  vbox->addWidget(editor);
  vbox->addWidget(boutonbox);
}


void KgpgView::modified()
{
  /////////////// notify for changes in editor window
  KgpgApp *win=(KgpgApp *) parent();
  if (win->fileSave->isEnabled()==false)
  {
  QString capt=win->Docname.filename();
  if (capt=="") capt=i18n("untitled");
  win->setCaption(capt,true);
  win->fileSave->setEnabled(true);
  win->editUndo->setEnabled(true);
  }
}

void KgpgView::viewreadopts()
{
  ////////  read default options for encryption

  KgpgApp *win=(KgpgApp *) parent();
  pubascii=win->ascii;
  pubpgp=win->pgpcomp;
  pubuntrusted=win->untrusted;
  pubencrypttodefault=win->encrypttodefault;
  pubdefaultkey=win->defaultkey;
}

void KgpgView::clearSign()
{
    QString mess=editor->text();
    if (mess.startsWith("-----BEGIN PGP SIGNED"))
    {
        //////////////////////   this is a signed message, verify it

        ///////////////////  generate gpg command
        char line[5000]="echo \"";
        int i=0;
        while(i!=-1)
        {
            i=mess.find("$",i,FALSE);
            if (i!=-1) {mess.insert(i,"\\");i+=2;}
        }
        strcat(line,mess.local8Bit());
        strcat(line,"\" | gpg");
        strcat(line," --no-tty ");
        QString tst="";

        ///////////////// run command
        FILE *fp,*cmdstatus;
        int process[2];

        pipe(process);
        cmdstatus = fdopen(process[1], "w");
        strcat(line,"--logger-fd ");
        QString fd;
        fd.setNum(process[1]);
        strcat(line, fd.latin1());
        strcat(line," --no-secmem-warning --verify");
        fp=popen(line,"r");
        pclose(fp);
        fclose(cmdstatus);

        int Len;
        char Buff[500]="\0";

        //////////////////////////   read gpg output
        while (read(process[0], &Len, sizeof(Len)) > 0)
        {
            read(process[0],Buff, Len);
            tst+=Buff;
        }


        if (tst.find("Good signature",0,FALSE)!=-1) KMessageBox::information(this,tst);
        else KMessageBox::sorry(this,tst);

    }
    else
    {
        /////    Sign the text in Editor


        QString signKeyID,signKeyMail;
        FILE *fp,*pass;
        int ppass[2];
        QCString password;

        ///// open key selection dialog
        KgpgSelKey *opts=new KgpgSelKey(this,0,false);


        if (opts->exec()==QDialog::Accepted) {signKeyID=opts->getkeyID();signKeyMail=opts->getkeyMail();}
        else
        {
            delete opts;
            return;
        }
        delete opts;
        /////////////////////  get passphrase
        int code=KPasswordDialog::getPassword(password,i18n("Enter passphrase for %1:").arg(signKeyMail));
        ///////////////////   ask for password
        if (code!=QDialog::Accepted) return;

        ///////////////////   pipe passphrase
        pipe(ppass);
        pass = fdopen(ppass[1], "w");
        fwrite(password, sizeof(char), strlen(password), pass);
        fwrite("\n", sizeof(char), 1, pass);
        fclose(pass);

        ///////////////////  generate gpg command
        char line[5000]="echo \"";

        int i=0;
        while(i!=-1)
        {
            i=mess.find("$",i,FALSE);
            if (i!=-1) {mess.insert(i,"\\");i+=2;}
        }
        strcat(line,mess.local8Bit());
        strcat(line,"\" | gpg ");
        if (pubpgp) strcat(line,"--pgp6 ");
        strcat(line,"--passphrase-fd ");
        QString fd;
        fd.setNum(ppass[0]);
        strcat(line,fd.latin1());
        strcat(line," --no-tty --clearsign -u ");
        strcat(line,signKeyID.local8Bit());
        //KMessageBox::sorry(0,QString(line));
        QString tst="";

        ///////////////// run command
        fp = popen(line, "r");
        while ( fgets( line, sizeof(line), fp))
            tst+=line;
        pclose(fp);

        /////////////////  paste result into editor
        if (tst!="")
        {
            editor->setText(tst);
            //modified();
            KgpgApp *win=(KgpgApp *) parent();
            win->editRedo->setEnabled(false);
            win->editUndo->setEnabled(false);
        }
        else
            KMessageBox::sorry(this,i18n("Signing not possible: bad passphrase, missing key or corrupted file"));
    }
}

void KgpgView::popuppublic()
{
  /////    popup dialog to select public key for encryption

 //   if (editor->length()>4500)
  //    KMessageBox::sorry(this,i18n("Sorry, but your text is too long for direct encryption.\nPlease save it and use file mode"));
  //  else
   // {
      ////////  open dialog --> popuppublic.cpp
      popupPublic *dialogue=new popupPublic(this, "public_keys", 0,false);
      connect(dialogue,SIGNAL(selectedKey(QString &,bool,bool,bool,bool,bool)),this,SLOT(encode(QString &,bool,bool,bool)));
      dialogue->exec();
      delete dialogue;
   // }
}


//////////////////////////////////////////////////////////////////////////////////////     decode

void KgpgView::slotdecode()
{
  ///////////////    decode data from the editor. triggered by the decode button

  QString dests,encUsers;
  messages="";

  encUsers=KgpgInterface::extractKeyName(editor->text());

  if (encUsers.isEmpty()) encUsers=i18n("[No user ID found]");
  QString resultat=KgpgInterface::KgpgDecryptText(editor->text(),encUsers);
  KgpgApp *win=(KgpgApp *) parent();
if (resultat!="")
{
editor->setText(resultat);
win->editRedo->setEnabled(false);
win->editUndo->setEnabled(false);
 }
 else if (win->commandLineMode) {KMessageBox::sorry(0,i18n("Decryption failed"));exit(0);}
 }


void KgpgView::encode(QString &selec,bool utrust,bool arm,bool hideID)
{
  //////////////////              encode from editor
  QString encryptOptions="";

  if (utrust) encryptOptions+=" --always-trust ";
  if (arm) encryptOptions+=" --armor ";
  if (hideID) encryptOptions+=" --throw-keyid ";
  if (pubpgp) encryptOptions+=" --pgp6 ";
    

 if (selec==NULL) {KMessageBox::sorry(0,i18n("You have not chosen an encryption key..."));return;}

 QString resultat=KgpgInterface::KgpgEncryptText(editor->text(),selec,encryptOptions);
 if (resultat!="") editor->setText(resultat);
}

KgpgView::~KgpgView()
{}

/*
void KgpgView::print(QPrinter *pPrinter)
{
  QPainter printpainter;
  printpainter.begin(pPrinter);

  // TODO: add your printing code here

  printpainter.end();
}
*/
#include "kgpgview.moc"

