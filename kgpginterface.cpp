/***************************************************************************
                          kgpginterface.cpp  -  description
                             -------------------
    begin                : Mon Jul 8 2002
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


#include <qdialog.h>
#include <qclipboard.h>
#include <qlayout.h>
#include <qregexp.h>

#include <kmessagebox.h>
#include <kapplication.h>
#include <klocale.h>
#include <kpassdlg.h>
#include <kmdcodec.h>
#include <klineedit.h>

#include "kgpginterface.h"

KgpgInterface::KgpgInterface()
{}


void KgpgInterface::KgpgEncryptFile(QString userIDs,KURL srcUrl,KURL destUrl, QString Options, bool symetrical)
{
    file=destUrl;
    QString cut;
    encError=false;
    message="";
    KProcIO *proc=new KProcIO();
    userIDs=userIDs.stripWhiteSpace();
    userIDs=userIDs.simplifyWhiteSpace();
    Options=Options.stripWhiteSpace();
    Options=Options.simplifyWhiteSpace();
    if (symetrical==false)
    {
        *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--command-fd=0"<<"--status-fd=2";
        while (!Options.isEmpty())
        {
            QString fOption=Options.section(' ',0,0);
            Options.remove(0,fOption.length());
            Options=Options.stripWhiteSpace();
            *proc<<QFile::encodeName(fOption);
        }
        *proc<<"--output"<<QFile::encodeName(destUrl.path())<<"-e";
        int ct=userIDs.find(" ");
        while (ct!=-1)  // if multiple keys...
        {
            *proc<<"--recipient"<<userIDs.section(' ',0,0);
            userIDs.remove(0,ct+1);
            ct=userIDs.find(" ");
        }
        *proc<<"--recipient"<<userIDs<<QFile::encodeName(srcUrl.path());
    }
    else  ////////////   symetrical encryption, prompt for password
    {
        int ppass[2];
        FILE *pass;
        QCString password;
        int code=KPasswordDialog::getNewPassword(password,QString(i18n("Enter passphrase for file %1(symmetrical encryption):").arg(srcUrl.filename())));
        if (code!=QDialog::Accepted)
        {
            emit encryptionfinished(true);
            return;
        }
        pipe(ppass);
        pass = fdopen(ppass[1], "w");
        fwrite(password, sizeof(char), strlen(password), pass);
        fwrite("\n", sizeof(char), 1, pass);
        fclose(pass);

        *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--status-fd=2"<<"--command-fd=0";
        while (!Options.isEmpty())
        {
            QString fOption=Options.section(' ',0,0);
            Options.remove(0,fOption.length());
            Options=Options.stripWhiteSpace();
            *proc<<QFile::encodeName(fOption);
        }
        *proc<<"--output"<<QFile::encodeName(destUrl.path())<<"--passphrase-fd"<<QString::number(ppass[0])<<"-c"<<QFile::encodeName(srcUrl.path());
	}

    /////////  when process ends, update dialog infos
    QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(encryptfin(KProcess *)));
    QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(readprocess(KProcIO *)));
    proc->start(KProcess::NotifyOnExit,true);
    //encryptfin(proc);
}


KgpgInterface::~KgpgInterface()
{}


void KgpgInterface::encryptfin(KProcess *)
{
  if (message.find("END_ENCRYPTION")!=-1)  emit encryptionfinished(true);
  else
  {
  //KMessageBox::sorry(0,message);
  emit encryptionfinished(false);
  }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////   File decryption


int KgpgInterface::KgpgDecryptFile(QString userIDs,KURL srcUrl,KURL destUrl,int chances)
{
    QCString password;
    QString passdlg;
    FILE *pass;
    int ppass[2];

    filedec=destUrl;
    QString cut;
    decError=false;
    message="";

    userIDs=userIDs.stripWhiteSpace();
    userIDs=userIDs.simplifyWhiteSpace();

    /// pipe for passphrase


      if (userIDs=="")
        passdlg=i18n("Enter passphrase for file %1:").arg(srcUrl.filename());
      else
        passdlg=i18n("Enter passphrase for %1:").arg(userIDs);
      if (chances!=0)
        passdlg.prepend(i18n("<b>Bad passphrase</b><br> You have %1 trial(s) left.<br>").arg(QString::number(chances)));

      /// pipe for passphrase
      int code=KPasswordDialog::getPassword(password,passdlg);
      if (code!=QDialog::Accepted) return 0;

      //   pass=password;

      pipe(ppass);
      pass = fdopen(ppass[1], "w");
      fwrite(password, sizeof(char), strlen(password), pass);
      fwrite("\n", sizeof(char), 1, pass);
      fclose(pass);

      /// create gpg command

        KProcIO *proc=new KProcIO();
      if (destUrl.filename()!="") // a filename was entered
	        *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--status-fd=2"<<"--command-fd=0"<<"--passphrase-fd"<<
                QString::number(ppass[0])<<"-o"<<QFile::encodeName(destUrl.path())<<"-d"<<QFile::encodeName(srcUrl.path());

      else //// no filename -> decrypt to editor
              *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--status-fd=2"<<"--passphrase-fd"<<QString::number(ppass[0])
                   <<"-d"<<QFile::encodeName(srcUrl.path());

  QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(decryptfin(KProcess *)));
  QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(readprocess(KProcIO *)));
proc->start(KProcess::NotifyOnExit,true);
return 1;
}

void KgpgInterface::decryptfin(KProcess *)
{
 if (message.find("BAD_PASSPHRASE")!=-1) emit badpassphrase(false);
else if ((message.find("DECRYPTION_OKAY")!=-1) && (message.find("END_DECRYPTION")!=-1)) //&& (message.find("GOODMDC")!=-1)
emit decryptionfinished(true);
else
{
KMessageBox::sorry(0,message);
emit decryptionfinished(false);
}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////    Text encryption

QString KgpgInterface::KgpgEncryptText(QString text,QString userIDs, QString Options)
{
    FILE *fp;
    QString dests,gpgcmd,encResult;
    char buffer[200];

    userIDs=userIDs.stripWhiteSpace();
    userIDs=userIDs.simplifyWhiteSpace();
    Options=Options.stripWhiteSpace();


    int ct=userIDs.find(" ");
    while (ct!=-1)  // if multiple keys...
    {
        dests+=" --recipient "+userIDs.section(' ',0,0);
        userIDs.remove(0,ct+1);
        ct=userIDs.find(" ");
    }
    dests+=" --recipient "+userIDs;

    text=text.replace(QRegExp("\\\\") , "\\\\").replace(QRegExp("\\\"") , "\\\"").replace(QRegExp("\\$") , "\\$");

	gpgcmd="echo \""+text+"\" | gpg --no-secmem-warning --no-tty "+Options+" -e "+dests;
    //////////   encode with untrusted keys or armor if checked by user
    fp = popen(QFile::encodeName(gpgcmd), "r");
    while ( fgets( buffer, sizeof(buffer), fp))
        encResult+=buffer;
    pclose(fp);
    if (encResult!="") return encResult;
    else return QString::null;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////     Text decryption

QString KgpgInterface::KgpgDecryptText(QString text,QString userID)
{
    FILE *fp,*pass;
    QString encResult,gpgcmd;
    char buffer[200];
    int counter=0,ppass[2];
    QCString password;

    while ((counter<3) && (encResult==""))
    {
        /// pipe for passphrase
        counter++;
        QString passdlg=i18n("Enter passphrase for %1:").arg(userID);
        if (counter>1) passdlg.prepend(i18n("<b>Bad passphrase</b><br> You have %1 trial(s) left.<br>").arg(QString::number(4-counter)));

        /// pipe for passphrase
        int code=KPasswordDialog::getPassword(password,passdlg);
        if (code!=QDialog::Accepted) return "";

        pipe(ppass);
        pass = fdopen(ppass[1], "w");
        fwrite(password, sizeof(char), strlen(password), pass);
        fwrite("\n", sizeof(char), 1, pass);
        fclose(pass);

        gpgcmd="echo \""+text+"\" | gpg --no-tty --passphrase-fd "+QString::number(ppass[0])+" -d ";
        //////////   encode with untrusted keys or armor if checked by user
        fp = popen(QFile::encodeName(gpgcmd), "r");
        while ( fgets( buffer, sizeof(buffer), fp))
            encResult+=buffer;
        pclose(fp);
    }
    if (encResult!="") return encResult;
    else return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////   MD5

Md5Widget::Md5Widget(QWidget *parent, const char *name,KURL url):KDialogBase( parent, name, true,i18n("MD5 Checksum"),Apply | Close)
  {
setButtonApplyText(i18n("Compare MD5 with clipboard"));
mdSum="";
QFile f(url.path());
f.open( IO_ReadOnly);
KMD5 checkfile;
checkfile.reset();
checkfile.update(f);
mdSum=checkfile.hexDigest().data();
f.close();
QWidget *page = new QWidget(this);

resize( 360, 150 );
QGridLayout *MyDialogLayout = new QGridLayout( page, 1, 1, 5, 6, "MyDialogLayout");

QLabel *TextLabel1 = new QLabel( page, "TextLabel1" );
    //TextLabel1->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)3, (QSizePolicy::SizeType)0, 0, 0, TextLabel1->sizePolicy().hasHeightForWidth() ) );
    TextLabel1->setText(i18n("MD5 sum for <b>%1</b> is:").arg(url.filename()));
    MyDialogLayout->addWidget( TextLabel1, 0, 0 );

    KLineEdit *KRestrictedLine1 = new KLineEdit(mdSum,page);
    KRestrictedLine1->setReadOnly(true);
    KRestrictedLine1->setPaletteBackgroundColor(QColor(255,255,255));
    MyDialogLayout->addWidget( KRestrictedLine1, 1, 0 );


    QHBoxLayout *Layout4 = new QHBoxLayout( 0, 0, 6, "Layout4");

    KLed1=new KLed(QColor(80,80,80),KLed::Off,KLed::Sunken,KLed::Circular,page,"KLed1");
  KLed1->off();
    KLed1->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)0, (QSizePolicy::SizeType)0, 0, 0, KLed1->sizePolicy().hasHeightForWidth() ) );
    Layout4->addWidget( KLed1 );

    TextLabel1_2 = new QLabel( page, "TextLabel1_2" );
    TextLabel1_2->setText(i18n( "<b>Unknown status</b>" ) );
    Layout4->addWidget( TextLabel1_2 );

    MyDialogLayout->addLayout( Layout4, 2, 0 );
    QSpacerItem* spacer = new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding );
    MyDialogLayout->addItem( spacer, 3, 0 );

page->show();
page->resize(page->minimumSize());
setMainWidget(page);


}

Md5Widget::~Md5Widget()
{}

void Md5Widget::slotApply()
{
    QClipboard *cb = QApplication::clipboard();
    QString text;
    // Copy text from the clipboard (paste)
    text = cb->text();
    if ( !text.isEmpty() )
    {
        text=text.stripWhiteSpace();
        while (text.find(' ')!=-1) text.remove(text.find(' '),1);
        if (text==mdSum) {
            TextLabel1_2->setText(i18n("<b>Correct checksum</b>, file is ok"));
            KLed1->setColor(QColor(0,255,0));
            KLed1->on();
        }//KMessageBox::sorry(0,"OK");
        else if (text.length()!=mdSum.length())
            KMessageBox::sorry(0,i18n("Clipboard content is not a MD5 sum..."));
        else
        {
            TextLabel1_2->setText(i18n("<b>Wrong checksum, FILE CORRUPTED</b>"));
            KLed1->setColor(QColor(255,0,0));KLed1->on();
        }
    }
}

 /////////////////////////////////////////////////////////////////////////////////////////////   signatures


void KgpgInterface::KgpgSignFile(QString keyName,QString keyID,KURL srcUrl,QString Options)
{
  //////////////////////////////////////   create a detached signature for a chosen file
  FILE *pass;
  int ppass[2];
  QCString password;
  QString cut;
  message="";

       /////////////////////  get passphrase
      //int code=KPasswordDialog::getPassword(password,QString("Enter passphrase for "+signKey+":"));
      int code=KPasswordDialog::getPassword(password,i18n("Enter passphrase for %1:").arg(keyName));
      if (code!=QDialog::Accepted) {emit signfinished();return;}

      pipe(ppass);
      pass = fdopen(ppass[1], "w");
      fwrite(password, sizeof(char), strlen(password), pass);
      fwrite("\n", sizeof(char), 1, pass);
      fclose(pass);

      /////////////       create gpg command
      KProcIO *proc=new KProcIO();
  keyID=keyID.stripWhiteSpace();
  Options=Options.stripWhiteSpace();
  Options=Options.simplifyWhiteSpace();

    *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--status-fd=2"<<"--command-fd=0"<<"--passphrase-fd"<<QString::number(ppass[0])<<"-u"<<keyID.local8Bit();
	while (!Options.isEmpty())
  {
  QString fOption=Options.section(' ',0,0);
  Options.remove(0,fOption.length());
  Options=Options.stripWhiteSpace();
   *proc<<QFile::encodeName(fOption);
   }
	 *proc<<"--detach-sig"<<QFile::encodeName(srcUrl.path());

      /////////         open gpg pipe
      //file=KURL(srcUrl.path()+".sig");
      //QFile fsig(file.path());
      //if (fsig.exists()) fsig.remove();

  QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(signfin(KProcess *)));
  QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(readprocess(KProcIO *)));
  //QObject::connect(proc,SIGNAL(receivedStderr(KProcess *, char *, int)),this,SLOT(encrypterror(KProcess *, char *, int)));
  proc->start(KProcess::NotifyOnExit,true);
}



void KgpgInterface::signfin(KProcess *)
{
    if (message.find("BAD_PASSPHRASE")!=-1) KMessageBox::sorry(0,i18n("Bad passphrase, signature was not created"));
    else if (message.find("SIG_CREATED")!=-1) KMessageBox::information(0,i18n("The signature file %1 was successfully created").arg(file.filename()));
    else KMessageBox::sorry(0,message);
    emit signfinished();
}


void KgpgInterface::KgpgVerifyFile(KURL sigUrl,KURL srcUrl)
{
  //////////////////////////////////////   verify signature for a chosen file
  message="";
  /////////////       create gpg command
  KProcIO *proc=new KProcIO();
  file=srcUrl;
  *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--status-fd=2"<<"--verify";
  if (srcUrl.filename()!="") *proc<<QFile::encodeName(srcUrl.path());
  *proc<<QFile::encodeName(sigUrl.path());

  QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(verifyfin(KProcess *)));
  QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(readprocess(KProcIO *)));
  //QObject::connect(proc,SIGNAL(receivedStdout(KProcess *, char *, int)),this,SLOT(verifyprocess(KProcess *, char *, int)));
  //QObject::connect(proc,SIGNAL(receivedStderr(KProcess *, char *, int)),this,SLOT(verifyprocess(KProcess *, char *, int)));
  proc->start(KProcess::NotifyOnExit,true);
}


void KgpgInterface::readprocess(KProcIO *p)
{
QString required="";
while (p->readln(required,true)!=-1)
{
//KMessageBox::sorry(0,required);
if (required.find("GET_")!=-1)
{
if (required.find("openfile.overwrite.okay")!=-1) p->writeStdin("Yes");
else p->writeStdin("quit");
}
message+=required+"\n";
}
}

void KgpgInterface::verifyfin(KProcess *)
{
    QString keyID,keyMail;
    if ((message.find("VALIDSIG")!=-1) && (message.find("GOODSIG")!=-1) && (message.find("BADSIG")==-1))
    {
        message.remove(0,message.find("GOODSIG")+7);
        message=message.section('\n',0,0);
        message=message.stripWhiteSpace();
        keyID=message.section(' ',0,0);
        message.remove(0,keyID.length());
        keyMail=message;
        KMessageBox::information(0,i18n("Good signature from %1\nKey ID: %2").arg(keyMail).arg(keyID),file.filename());
    }
    else if (message.find("UNEXPECTED")!=-1) KMessageBox::sorry(0,i18n("No signature found..."),file.filename());
    else if (message.find("BADSIG")!=-1)
    {
        message.remove(0,message.find("BADSIG")+7);
        message=message.section('\n',0,0);
        message=message.stripWhiteSpace();
        keyID=message.section(' ',0,0);
        message.remove(0,keyID.length());
        keyMail=message;
        KMessageBox::sorry(0,i18n("BAD signature from %1\nKey ID: %2\n\nThe file is corrupted!").arg(keyMail).arg(keyID),file.filename());
    } else
        KMessageBox::sorry(0,message);
		emit verifyfinished();
}



////////////////////////////////////////////////////////////   sign a key

void KgpgInterface::KgpgSignKey(QString keyID,QString signKeyID,QString signKeyMail,bool local)
{
if (checkuid(keyID)>0)
{
  KProcess *conprocess=new KProcess();
  *conprocess<< "konsole"<<"-e"<<"gpg";
  *conprocess<<"--no-secmem-warning"<<"-u"<<signKeyID;
  if (local==false) *conprocess<<"--sign-key"<<keyID;
  else *conprocess<<"--lsign-key"<<keyID;
  conprocess->start(KProcess::Block);
emit signatureFinished(0);
return;
}
konsLocal=local;
konsSignKey=signKeyID;
konsKeyID=keyID;
signSuccess=0;
step=0;
message="sign";
if (local==true) message="lsign";
int code=KPasswordDialog::getPassword(passphrase,i18n("Enter passphrase for %1:").arg(signKeyMail));
      if (code!=QDialog::Accepted)
        return;
KProcIO *conprocess=new KProcIO();
  *conprocess<<"gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--command-fd=0"<<"--status-fd=2"<<"-u"<<signKeyID;
  *conprocess<<"--edit-key"<<keyID;
  QObject::connect(conprocess,SIGNAL(readReady(KProcIO *)),this,SLOT(sigprocess(KProcIO *)));
  QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(signover(KProcess *)));
  conprocess->start(KProcess::NotifyOnExit,KProcess::AllOutput);

}

void KgpgInterface::sigprocess(KProcIO *p)//ess *p,char *buf, int buflen)
{
QString required="";

while (p->readln(required,true)!=-1)
{
//KMessageBox::sorry(0,required);
if ((step==0) && (required.find("keyedit.prompt")!=-1)) {p->writeStdin(message);step=1;required="";}
 if (required.find("sign_uid.expire")!=-1) {p->writeStdin("Never");required="";}
 if (required.find("sign_uid.class")!=-1) {p->writeStdin("");required="";}
 if (required.find("sign_uid.okay")!=-1) {p->writeStdin("Y");required="";}
 if (required.find("passphrase.enter")!=-1) {p->writeStdin(QString(passphrase));passphrase="xxxxxxxxxxxxxx";required="";step=2;}
 if ((step==2) && (required.find("keyedit.prompt")!=-1)) {p->writeStdin("save");required="";}
if (required.find("BAD_PASSPHRASE")!=-1){p->writeStdin("quit");signSuccess=2;}
if ((required.find("GET_")!=-1) && (signSuccess!=2)) /////// gpg asks for something unusal, turn to konsole mode
{
//p->writeStdin("quit");
signSuccess=0;
  KProcess *conprocess=new KProcess();
  *conprocess<< "konsole"<<"-e"<<"gpg";
  *conprocess<<"--no-secmem-warning"<<"-u"<<konsSignKey;
  if (konsLocal==false) *conprocess<<"--sign-key"<<konsKeyID;
  else *conprocess<<"--lsign-key"<<konsKeyID;
  conprocess->start(KProcess::Block);
  emit signatureFinished(0);
}
}
//p->ackRead();
}

void KgpgInterface::signover(KProcess *)
{
emit signatureFinished(signSuccess);
}

////////////////////////////////////////////////////////////////////////////     delete signature


void KgpgInterface::KgpgDelSignature(QString keyID,QString signKeyID)
{
    if (checkuid(keyID)>0) {
        KMessageBox::sorry(0,i18n("This key has more than one user ID...\nEdit the key manually to delete signature."));
        return;
    }

    message=signKeyID.remove(0,2);
    deleteSuccess=false;
    step=0;
/*
  int code=KPasswordDialog::getPassword(passphrase,i18n("Enter passphrase for %1:").arg(signKeyMail));
  if (code!=QDialog::Accepted)
  return;
*/
    FILE *fp;
    QString encResult;
    char buffer[200];
    signb=0;
    sigsearch=0;

    QString gpgcmd="gpg --no-tty --no-secmem-warning --with-colon --list-sigs "+keyID;
    //////////   encode with untrusted keys or armor if checked by user
    fp = popen(QFile::encodeName(gpgcmd), "r");
    while ( fgets( buffer, sizeof(buffer), fp))
    {
        encResult=buffer;
        if (encResult.startsWith("sig"))
        {
            if (encResult.find(message)!=-1) break;
            signb++;
        }
        else if (encResult.startsWith("rev")) signb++;
    }
    pclose(fp);

//KMessageBox::sorry(0,"Signature numero: "+QString::number(signb));


//KMessageBox::sorry(0,keyID+"::"+signKeyID);
    KProcIO *conprocess=new KProcIO();
    *conprocess<<"gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--command-fd=0"<<"--status-fd=2";
    *conprocess<<"--edit-key"<<keyID;
    QObject::connect(conprocess,SIGNAL(readReady(KProcIO *)),this,SLOT(delsigprocess(KProcIO *)));
    QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(delsignover(KProcess *)));
    conprocess->start(KProcess::NotifyOnExit,true);
}


void KgpgInterface::delsigprocess(KProcIO *p)//ess *p,char *buf, int buflen)
{

QString required="";
while (p->readln(required,true)!=-1)
{
 if ((step==0) && (required.find("keyedit.prompt")!=-1)) {p->writeStdin("uid 1");step=1;required="";}
  if ((step==1) && (required.find("keyedit.prompt")!=-1)) {p->writeStdin("delsig");step=2;required="";}
  if ((step==2) && (required.find("keyedit.delsig")!=-1))
  {

  if (sigsearch==signb) {p->writeStdin("Y");step=3;}
  else p->writeStdin("n");
  sigsearch++;
  required="";
  }
 if ((step==3) && (required.find("keyedit.prompt")!=-1)) {p->writeStdin("save");required="";deleteSuccess=true;}
 if (required.find("GET_LINE")!=-1){p->writeStdin("quit");deleteSuccess=false;}
 }
//p->ackRead();
}

void KgpgInterface::delsignover(KProcess *)
{
emit encryptionfinished(deleteSuccess);
}

int KgpgInterface::checkuid(QString KeyID)
{
    FILE *fp;
    QString encResult;
    char buffer[200];
    int  uidcnt=0;

    QString gpgcmd="gpg --no-tty --no-secmem-warning --with-colon --list-sigs "+KeyID;
    //////////   encode with untrusted keys or armor if checked by user
    fp = popen(QFile::encodeName(gpgcmd), "r");
    while (fgets(buffer, sizeof(buffer), fp))
    {
        encResult=buffer;
        if (encResult.startsWith("uid")) uidcnt++;
    }
    pclose(fp);
    return uidcnt;
}

//////////////////////////////////////////////////////////////    key import


void KgpgInterface::importKey(KURL url)
{
  /////////////      import a key

  if( KIO::NetAccess::download( url, tempKeyFile ) )
  {
      message=QString::null;
      KProcIO *conprocess=new KProcIO();
      *conprocess<< "gpg";
      *conprocess<<"--no-tty"<<"--no-secmem-warning"<<"--allow-secret-key-import"<<"--import"<<tempKeyFile;
      QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(importover(KProcess *)));
      QObject::connect(conprocess, SIGNAL(readReady(KProcIO *)),this, SLOT(importprocess(KProcIO *)));
      conprocess->start(KProcess::NotifyOnExit,true);
    }
}

void KgpgInterface::importover(KProcess *)
{
    KIO::NetAccess::removeTempFile(tempKeyFile);
    KMessageBox::information(0,message);
    emit importfinished();
}

void KgpgInterface::importprocess(KProcIO *p)
{
    QString outp;
    while (p->readln(outp)!=-1)
    {
        if (outp.find("http-proxy")==-1)
            message+=outp+"\n";
    }
}

QString KgpgInterface::extractKeyName(KURL url)
{
    ///////////////////////////////////////////////////////////////// extract  encryption keys

    FILE *fp;
    QString encResult,IDs;
    char buffer[200];


    QString gpgcmd="gpg --no-tty --no-secmem-warning --batch --status-fd 1 -d "+url.path();
    //////////   encode with untrusted keys or armor if checked by user
    fp = popen(QFile::encodeName(gpgcmd), "r");
    while ( fgets( buffer, sizeof(buffer), fp))
    {
        encResult=buffer;
        if (encResult.find("USERID_HINT",0,false)!=-1)
        {
            if (!IDs.isEmpty()) IDs+=i18n(" or ");
            encResult=encResult.section("HINT",1,1);
            encResult=encResult.stripWhiteSpace();
            int cut=encResult.find(' ',0,false);
            encResult.remove(0,cut);
            if (encResult.find("(",0,false)!=-1)
                encResult=encResult.section('(',0,0)+encResult.section(')',-1,-1);
            IDs+=encResult;
        }
    }
    pclose(fp);
    return IDs;
}

QString KgpgInterface::extractKeyName(QString txt)
{
    ///////////////////////////////////////////////////////////////// extract  encryption keys

    FILE *fp;
    QString encResult,IDs;
    char buffer[200];


    QString gpgcmd="echo \""+txt+"\" | gpg --no-tty --no-secmem-warning --batch --status-fd 1 -d ";
    //////////   encode with untrusted keys or armor if checked by user
    fp = popen(QFile::encodeName(gpgcmd), "r");
    while ( fgets( buffer, sizeof(buffer), fp))
    {
        encResult=buffer;
        if (encResult.find("USERID_HINT",0,false)!=-1)
        {
            if (!IDs.isEmpty()) IDs+=i18n(" or ");
            encResult=encResult.section("HINT",1,1);
            encResult=encResult.stripWhiteSpace();
            int cut=encResult.find(' ',0,false);
            encResult.remove(0,cut);
            if (encResult.find("(",0,false)!=-1)
                encResult=encResult.section('(',0,0)+encResult.section(')',-1,-1);
            IDs+=encResult;
        }
    }
    pclose(fp);
    return IDs;
}


//#include "kgpginterface.moc"
#include "kgpginterface.moc"
