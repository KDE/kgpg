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
#include <qstring.h>

#include <kmessagebox.h>
#include <kapplication.h>
#include <klocale.h>
#include <kpassdlg.h>
#include <kmdcodec.h>
#include <klineedit.h>


#include "kgpginterface.h"

KgpgInterface::KgpgInterface()
{}

void KgpgInterface::KgpgEncryptFile(QString encuserIDs,KURL srcUrl,KURL destUrl, QString Options, bool symetrical)
{
  file=destUrl;
  encError=false;
  message="";
  KProcIO *proc=new KProcIO();
  encuserIDs=encuserIDs.stripWhiteSpace();
  encuserIDs=encuserIDs.simplifyWhiteSpace();
  Options=Options.stripWhiteSpace();
  Options=Options.simplifyWhiteSpace();
  if (!symetrical)
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
      int ct=encuserIDs.find(" ");
      while (ct!=-1)  // if multiple keys...
        {
          *proc<<"--recipient"<<encuserIDs.section(' ',0,0);
          encuserIDs.remove(0,ct+1);
          ct=encuserIDs.find(" ");
        }
      *proc<<"--recipient"<<encuserIDs<<QFile::encodeName(srcUrl.path());
    }
  else  ////////////   symetrical encryption, prompt for password
    {
      *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--status-fd=2"<<"--command-fd=0";
      while (!Options.isEmpty())
        {
          QString fOption=Options.section(' ',0,0);
          Options.remove(0,fOption.length());
          Options=Options.stripWhiteSpace();
          *proc<<QFile::encodeName(fOption);
        }
      *proc<<"--output"<<QFile::encodeName(destUrl.path())<<"-c"<<QFile::encodeName(srcUrl.path());
    }

  /////////  when process ends, update dialog infos
  QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(encryptfin(KProcess *)));
  QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(readencprocess(KProcIO *)));
  proc->start(KProcess::NotifyOnExit,true);
  //encryptfin(proc);
}


KgpgInterface::~KgpgInterface()
{}


void KgpgInterface::encryptfin(KProcess *)
{
  if (message.find("END_ENCRYPTION")!=-1)
    emit encryptionfinished();
  else
    {
      	  emit errormessage(message);
	  //KMessageBox::sorry(0,message);
      //emit encryptionfinished(false);
    }
}

void KgpgInterface::readencprocess(KProcIO *p)
{
  QString required;
  while (p->readln(required,true)!=-1)
    {
      if (required.find("BEGIN_ENCRYPTION",0,false)!=-1)
        emit processstarted();
      if (required.find("GET_")!=-1)
        {
          if (required.find("openfile.overwrite.okay")!=-1)
            p->writeStdin("Yes");
          else if ((required.find("passphrase.enter")!=-1))
            {
              QCString passphrase;
              int code=KPasswordDialog::getNewPassword(passphrase,i18n("Enter passphrase for your file (symmetrical encryption):"));
              if (code!=QDialog::Accepted)
                {
                  delete p;
                  emit processaborted(true);
                  return;
                }
              p->writeStdin(passphrase,true);
              userIDs="";
              //p->writeStdin("\n");
              step--;
            }
          else
            {
			p->writeStdin("quit");
			p->closeWhenDone();
			}
        }
      message+=required+"\n";
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////   File decryption

void KgpgInterface::KgpgDecryptFile(KURL srcUrl,KURL destUrl,QString Options)
{
  message="";
  step=3;
  userIDs="";
  anonymous=false;
  Options=Options.stripWhiteSpace();
  Options=Options.simplifyWhiteSpace();
  KProcIO *proc=new KProcIO();
  if (!destUrl.filename().isEmpty()) // a filename was entered
    {
      *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--status-fd=2"<<"--command-fd=0";
      while (!Options.isEmpty())
        {
          QString fOption=Options.section(' ',0,0);
          Options.remove(0,fOption.length());
          Options=Options.stripWhiteSpace();
          *proc<<QFile::encodeName(fOption);
        }
      *proc<<"-o"<<QFile::encodeName(destUrl.path())<<"-d"<<QFile::encodeName(srcUrl.path());
    }
  else //// no filename given
    {
     *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--status-fd=2"<<"--command-fd=0";
      while (!Options.isEmpty())
        {
          QString fOption=Options.section(' ',0,0);
          Options.remove(0,fOption.length());
          Options=Options.stripWhiteSpace();
          *proc<<QFile::encodeName(fOption);
        }

      *proc<<"-d"<<QFile::encodeName(srcUrl.path());
    }
  QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(decryptfin(KProcess *)));
  QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(readdecprocess(KProcIO *)));
  proc->start(KProcess::NotifyOnExit,true);
}

void KgpgInterface::decryptfin(KProcess *)
{
  if ((message.find("DECRYPTION_OKAY")!=-1) && (message.find("END_DECRYPTION")!=-1)) //&& (message.find("GOODMDC")!=-1)
    emit decryptionfinished();
  else  emit errormessage(message);
}

void KgpgInterface::readdecprocess(KProcIO *p)
{
  QString required;
  while (p->readln(required,true)!=-1)
    {
	//required=QString::fromUtf8(required);
      if (required.find("BEGIN_DECRYPTION",0,false)!=-1)
        emit processstarted();
      if (required.find("USERID_HINT",0,false)!=-1)
        {
          required=required.section("HINT",1,1);
          required=required.stripWhiteSpace();
          int cut=required.find(' ',0,false);
          required.remove(0,cut);
          if (required.find("(",0,false)!=-1)
            required=required.section('(',0,0)+required.section(')',-1,-1);
          if (userIDs.find(required)==-1)
            {
              if (!userIDs.isEmpty())
                userIDs+=i18n(" or ");
              userIDs+=required;
            }
        }
      if (required.find("ENC_TO")!=-1)
        {
          if (required.find("0000000000000000")!=-1)
            anonymous=true;
        }
      if (required.find("GET_")!=-1)
        {
          if (required.find("openfile.overwrite.okay")!=-1)
            p->writeStdin("Yes");
          else if ((required.find("passphrase.enter")!=-1))
            {
              if (userIDs.isEmpty())
                userIDs=i18n("[No user id found]");
				userIDs.replace(QRegExp("<"),"&lt;");
              QCString passphrase;
              QString passdlgmessage;
              if (anonymous)
                passdlgmessage=i18n("<b>No user id found</b>. Trying all secret keys.<br>");
              if ((step<3) && (!anonymous))
                passdlgmessage=i18n("<b>Bad passphrase</b>. You have %1 tries left.<br>").arg(step);

              passdlgmessage+=i18n("Enter passphrase for <b>%1</b>").arg(userIDs);
              int code=KPasswordDialog::getPassword(passphrase,passdlgmessage);
              if (code!=QDialog::Accepted)
                {
                  delete p;
                  emit processaborted(true);
                  return;
                }
              p->writeStdin(passphrase,true);
              userIDs="";
              //p->writeStdin("\n");
              step--;
            }
          else
            {
			p->writeStdin("quit");
			p->closeWhenDone();
			}
        }
      message+=required+"\n";
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////    Text encryption


void KgpgInterface::KgpgEncryptText(QString text,QString userIDs, QString Options)
{
message="";
txtprocess=text;
txtsent=false;
 KProcIO *proc=new KProcIO();
  userIDs=userIDs.stripWhiteSpace();
  userIDs=userIDs.simplifyWhiteSpace();
  Options=Options.stripWhiteSpace();
  Options=Options.simplifyWhiteSpace();
	  *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--command-fd=0"<<"--status-fd=2";
      while (!Options.isEmpty())
        {
          QString fOption=Options.section(' ',0,0);
          Options.remove(0,fOption.length());
          Options=Options.stripWhiteSpace();
          *proc<<QFile::encodeName(fOption);
        }
      *proc<<"-e";
      int ct=userIDs.find(" ");
      while (ct!=-1)  // if multiple keys...
        {
          *proc<<"--recipient"<<userIDs.section(' ',0,0);
          userIDs.remove(0,ct+1);
          ct=userIDs.find(" ");
        }
      *proc<<"--recipient"<<userIDs;

  /////////  when process ends, update dialog infos

  QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(txtencryptfin(KProcess *)));
  QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(txtreadencprocess(KProcIO *)));
  proc->start(KProcess::NotifyOnExit,true);
}


void KgpgInterface::txtencryptfin(KProcess *)
{
if (txtsent) emit txtencryptionfinished(message);
else emit txtencryptionfinished("");
}

void KgpgInterface::txtreadencprocess(KProcIO *p)
{
  QString required;
  while (p->readln(required,true)!=-1)
    {
      if (required.find("BEGIN_ENCRYPTION")!=-1)
      {
	  p->writeStdin(txtprocess,true);
	  p->closeWhenDone();
	  }
	  else
	  if (required.find("END_ENCRYPTION")!=-1) txtsent=true;
     else message+=required+"\n";
	}
}



/*
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

  gpgcmd="echo ";
  gpgcmd+=KShellProcess::quote(text);
  gpgcmd+=" | gpg --no-secmem-warning --no-tty ";
  gpgcmd+=Options;
  gpgcmd+=" -e ";
  gpgcmd+=dests;
  //////////   encode with untrusted keys or armor if checked by user
  fp = popen(QFile::encodeName(gpgcmd), "r");
  while ( fgets( buffer, sizeof(buffer), fp))
    encResult+=buffer;
  pclose(fp);
  if (!encResult.isEmpty())
    return encResult;
  else
    return QString::null;
}

*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////     Text decryption
/*
void KgpgInterface::KgpgDecryptText(QString text,QString Options)   /// doesn't work, gives a mysterious bad mdc error...
{
  message="";
  step=3;
  userIDs="";
  anonymous=false;
txtprocess=text;
decfinished=false;
decok=false;
badmdc=false;
 KProcIO *proc=new KProcIO();
  userIDs=userIDs.stripWhiteSpace();
  userIDs=userIDs.simplifyWhiteSpace();
  Options=Options.stripWhiteSpace();
  Options=Options.simplifyWhiteSpace();
	  *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--command-fd=0"<<"--status-fd=1"<<"--no-batch";
      while (!Options.isEmpty())
        {
          QString fOption=Options.section(' ',0,0);
          Options.remove(0,fOption.length());
          Options=Options.stripWhiteSpace();
          *proc<<QFile::encodeName(fOption);
        }
      *proc<<"-d";

  /////////  when process ends, update dialog infos
  
  QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(txtdecryptfin(KProcess *)));
  QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(txtreaddecprocess(KProcIO *)));
  proc->start(KProcess::NotifyOnExit,false);
  proc->writeStdin(txtprocess,false);

}

void KgpgInterface::txtdecryptfin(KProcess *)
{
//KMessageBox::sorry(0,message);
if ((decok) && (!badmdc)) emit txtencryptionfinished(message);
else if (badmdc) {KMessageBox::sorry(0,i18n("Bad MDC detected. The encrypted message has been manipulated."));emit txtencryptionfinished(message);}
else {KMessageBox::sorry(0,i18n("The encryption was not successful."));emit txtencryptionfinished(message);}
}

void KgpgInterface::txtreaddecprocess(KProcIO *p)
{
  QString required;
  while (p->readln(required,true)!=-1)
    {
	{
	if (required.find("USERID_HINT",0,false)!=-1)
        {
          required=required.section("HINT",1,1);
          required=required.stripWhiteSpace();
          int cut=required.find(' ',0,false);
          required.remove(0,cut);
          if (required.find("(",0,false)!=-1)
            required=required.section('(',0,0)+required.section(')',-1,-1);
          if (userIDs.find(required)==-1)
            {
              if (!userIDs.isEmpty())
                userIDs+=i18n(" or ");
              userIDs+=required;
            }
        }
      if (required.find("ENC_TO")!=-1)
        {
          if (required.find("0000000000000000")!=-1)
            anonymous=true;
        }
      if (required.find("GET_")!=-1)
        {
         if ((required.find("passphrase.enter")!=-1))
            {
              if (userIDs.isEmpty())
                userIDs=i18n("[No user id found]");
              QCString passphrase;
              QString passdlgmessage;
              if (anonymous)
                passdlgmessage=i18n("<b>No user id found</b>. Trying all secret keys.<br>");
              if ((step<3) && (!anonymous))
                passdlgmessage=i18n("<b>Bad passphrase</b>. You have %1 tries left.<br>").arg(step);
              QString prettyuIDs=userIDs;
              prettyuIDs.replace(QRegExp("<"),"&lt;");
              passdlgmessage+=i18n("Enter passphrase for <b>%1</b>").arg(prettyuIDs);
              int code=KPasswordDialog::getPassword(passphrase,passdlgmessage);
              if (code!=QDialog::Accepted)
                {
                  delete p;
                  emit processaborted(true);
                  return;
                }
              p->writeStdin(passphrase,true);
              userIDs="";
              //p->writeStdin("\n");
              step--;
            }
		  else
           p->writeStdin("quit");
        }
		if (required.find("BEGIN_DECRYPTION")!=-1)
      		{
	  		p->writeStdin(txtprocess,false);
	  		//p->closeWhenDone();
			p->closeStdin();
			}
      if (decfinished) message+=required+"\n";
	  if (required.find("END_DECRYPTION")!=-1) decfinished=true;
	  if (required.find("DECRYPTION_OKAY")!=-1) decok=true;
	  if (required.find("BADMDC")!=-1) badmdc=true;
    }
	}
}
*/

QString KgpgInterface::KgpgDecryptText(QString text,QString userID)
{
  FILE *fp,*pass;
  QString encResult,gpgcmd;
  char buffer[200];
  int counter=0,ppass[2];
  QCString password;

  while ((counter<3) && (encResult.isEmpty()))
    {
      /// pipe for passphrase
      counter++;
	  //userID=QString::fromUtf8(userID);
	  userID.replace(QRegExp("<"),"&lt;");
      QString passdlg=i18n("Enter passphrase for <b>%1</b>:").arg(userID);
      if (counter>1)
        passdlg.prepend(i18n("<b>Bad passphrase</b><br> You have %1 tries left.<br>").arg(QString::number(4-counter)));

      /// pipe for passphrase
      int code=KPasswordDialog::getPassword(password,passdlg);
      if (code!=QDialog::Accepted)
        return " ";

      pipe(ppass);
      pass = fdopen(ppass[1], "w");
      fwrite(password, sizeof(char), strlen(password), pass);
      //        fwrite("\n", sizeof(char), 1, pass);
      fclose(pass);

      gpgcmd="echo ";
      gpgcmd+=KShellProcess::quote(text);
      gpgcmd+=" | gpg --no-secmem-warning --no-tty ";
      gpgcmd+="--passphrase-fd "+QString::number(ppass[0])+" -d ";
      //////////   encode with untrusted keys or armor if checked by user
      fp = popen(QFile::encodeName(gpgcmd), "r");
      while ( fgets( buffer, sizeof(buffer), fp))
        encResult+=buffer;
      pclose(fp);
    }
  if (encResult!="")
    return encResult;
  else
    return "";
}


QString KgpgInterface::KgpgDecryptFileToText(KURL srcUrl,QString userID)
{
  FILE *fp,*pass;
  QString encResult,gpgcmd;
  char buffer[200];
  int counter=0,ppass[2];
  QCString password;

  while ((counter<3) && (encResult.isEmpty()))
    {
      /// pipe for passphrase
      counter++;
	  //userID=QString::fromUtf8(userID);
	  userID.replace(QRegExp("<"),"&lt;");
      QString passdlg=i18n("Enter passphrase for <b>%1</b>:").arg(userID);
      if (counter>1)
        passdlg.prepend(i18n("<b>Bad passphrase</b><br> You have %1 tries left.<br>").arg(QString::number(4-counter)));

      /// pipe for passphrase
      int code=KPasswordDialog::getPassword(password,passdlg);
      if (code!=QDialog::Accepted)
        return " ";

      pipe(ppass);
      pass = fdopen(ppass[1], "w");
      fwrite(password, sizeof(char), strlen(password), pass);
      //        fwrite("\n", sizeof(char), 1, pass);
      fclose(pass);

      gpgcmd+="gpg --no-secmem-warning --no-tty ";
      gpgcmd+="--passphrase-fd "+QString::number(ppass[0])+" -o - -d '";
	  gpgcmd+=srcUrl.path()+"'";
      //////////   encode with untrusted keys or armor if checked by user
      fp = popen(QFile::encodeName(gpgcmd), "r");
      while ( fgets( buffer, sizeof(buffer), fp))
        encResult+=buffer;
      pclose(fp);
    }
  if (encResult!="")
    return encResult;
  else
    return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////   MD5

Md5Widget::Md5Widget(QWidget *parent, const char *name,KURL url):KDialogBase( parent, name, true,i18n("MD5 Checksum"),Apply | Close)
{
  setButtonApplyText(i18n("Compare MD5 With Clipboard"));
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
      while (text.find(' ')!=-1)
        text.remove(text.find(' '),1);
      if (text==mdSum)
        {
          TextLabel1_2->setText(i18n("<b>Correct checksum</b>, file is ok."));
          KLed1->setColor(QColor(0,255,0));
          KLed1->on();
        }//KMessageBox::sorry(0,"OK");
      else if (text.length()!=mdSum.length())
        KMessageBox::sorry(0,i18n("Clipboard content is not a MD5 sum."));
      else
        {
          TextLabel1_2->setText(i18n("<b>Wrong checksum, FILE CORRUPTED</b>"));
          KLed1->setColor(QColor(255,0,0));
          KLed1->on();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////   signatures


void KgpgInterface::KgpgSignFile(QString keyID,KURL srcUrl,QString Options)
{
  //////////////////////////////////////   create a detached signature for a chosen file
  message="";
  step=3;
  /////////////       create gpg command
  KProcIO *proc=new KProcIO();
  keyID=keyID.stripWhiteSpace();
  Options=Options.stripWhiteSpace();
  Options=Options.simplifyWhiteSpace();
  //<<"--passphrase-fd"<<QString::number(ppass[0])
  *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--status-fd=2"<<"--command-fd=0"<<"-u"<<keyID.local8Bit();
  while (!Options.isEmpty())
    {
      QString fOption=Options.section(' ',0,0);
      Options.remove(0,fOption.length());
      Options=Options.stripWhiteSpace();
      *proc<<QFile::encodeName(fOption);
    }
*proc<<"--output"<<QFile::encodeName(srcUrl.path())+".sig";
  *proc<<"--detach-sig"<<QFile::encodeName(srcUrl.path());

  /////////         open gpg pipe
  //file=KURL(srcUrl.path()+".sig");
  //QFile fsig(file.path());
  //if (fsig.exists()) fsig.remove();

  QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(signfin(KProcess *)));
  QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(readsignprocess(KProcIO *)));
  //QObject::connect(proc,SIGNAL(receivedStderr(KProcess *, char *, int)),this,SLOT(encrypterror(KProcess *, char *, int)));
  proc->start(KProcess::NotifyOnExit,true);
}



void KgpgInterface::signfin(KProcess *)
{
  if (message.find("SIG_CREATED")!=-1)
    KMessageBox::information(0,i18n("The signature file %1 was successfully created.").arg(file.filename()));
  else if (message.find("BAD_PASSPHRASE")!=-1)
    KMessageBox::sorry(0,i18n("Bad passphrase, signature was not created."));
  else
    KMessageBox::sorry(0,message);
  emit signfinished();
}


void KgpgInterface::readsignprocess(KProcIO *p)
{
  QString required;
  while (p->readln(required,true)!=-1)
    {
      if (required.find("USERID_HINT",0,false)!=-1)
        {
          required=required.section("HINT",1,1);
          required=required.stripWhiteSpace();
          int cut=required.find(' ',0,false);
          required.remove(0,cut);
          if (required.find("(",0,false)!=-1)
            required=required.section('(',0,0)+required.section(')',-1,-1);
          if (userIDs.find(required)==-1)
            {
              if (!userIDs.isEmpty())
                userIDs+=i18n(" or ");
              userIDs+=required;
            }
        }

      if (required.find("GET_")!=-1)
        {
          if (required.find("openfile.overwrite.okay")!=-1)
            p->writeStdin("Yes");
          else if ((required.find("passphrase.enter")!=-1))
            {
              if (userIDs.isEmpty())
                userIDs=i18n("[No user id found]");
              QCString passphrase;
              QString passdlgmessage;
              if (step<3)
                passdlgmessage=i18n("<b>Bad passphrase</b>. you have %1 tries left.<br>").arg(step);
              QString prettyuIDs=QString::fromUtf8(userIDs);
              prettyuIDs.replace(QRegExp("<"),"&lt;");
              passdlgmessage+=i18n("Enter passphrase for <b>%1</b>").arg(prettyuIDs);
              int code=KPasswordDialog::getPassword(passphrase,passdlgmessage);
              if (code!=QDialog::Accepted)
                {
                  delete p;
                  emit signfinished();
                  return;
                }
              p->writeStdin(passphrase,true);
              userIDs="";
              step--;
            }
          else
            {
			p->writeStdin("quit");
			p->closeWhenDone();
			}
        }
      message+=required+"\n";
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void KgpgInterface::KgpgVerifyFile(KURL sigUrl,KURL srcUrl)
{
  //////////////////////////////////////   verify signature for a chosen file
  message="";
  /////////////       create gpg command
  KProcIO *proc=new KProcIO();
  file=sigUrl;
  *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--status-fd=2"<<"--verify";
  if (!srcUrl.isEmpty())
  *proc<<QFile::encodeName(srcUrl.path());
  *proc<<QFile::encodeName(sigUrl.path());

  QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(verifyfin(KProcess *)));
  QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(readprocess(KProcIO *)));
  proc->start(KProcess::NotifyOnExit,true);
}


void KgpgInterface::readprocess(KProcIO *p)
{
  QString required;
  while (p->readln(required,true)!=-1)
    {
      if (required.find("GET_")!=-1)
        {
		p->writeStdin("quit");
		p->closeWhenDone();
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
      KMessageBox::information(0,i18n("<qt>Good signature from :<br><b>%1</b><br>Key ID: %2</qt>").arg(keyMail.replace(QRegExp("<"),"&lt;")).arg(keyID),file.filename());
    }
  else if (message.find("UNEXPECTED")!=-1)
    KMessageBox::sorry(0,i18n("No signature found."),file.filename());
  else if (message.find("BADSIG")!=-1)
    {
      message.remove(0,message.find("BADSIG")+7);
      message=message.section('\n',0,0);
      message=message.stripWhiteSpace();
      keyID=message.section(' ',0,0);
      message.remove(0,keyID.length());
      keyMail=message;
      KMessageBox::sorry(0,i18n("<qt><b>BAD signature</b> from:<br> %1<br>Key id: %2<br><br>"
	  "<b>The file is corrupted!</b></qt>").arg(keyMail.replace(QRegExp("<"),"&lt;")).arg(keyID),file.filename());
    }
  else
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
      if (local==false)
        *conprocess<<"--sign-key"<<keyID;
      else
        *conprocess<<"--lsign-key"<<keyID;
      conprocess->start(KProcess::Block);
      emit signatureFinished(0);
      return;
    }
signKeyMail.replace(QRegExp("<"),"&lt;");
  konsLocal=local;
  konsSignKey=signKeyID;
  konsKeyID=keyID;
  signSuccess=0;
  step=0;
  output="";
  message="sign";
  if (local)
    message="lsign";
  int code=KPasswordDialog::getPassword(passphrase,i18n("Enter passphrase for <b>%1</b>:").arg(signKeyMail));
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
	output+=required+"\n";
	    if ((step==2) && (required.find("GOOD_PASSPHRASE")!=-1))
        {
		signSuccess=3;
        }

      //KMessageBox::sorry(0,required);
      if ((step==0) && (required.find("keyedit.prompt")!=-1))
        {
          p->writeStdin(message);
          step=1;
          required="";
        }
      if (required.find("sign_uid.expire")!=-1)
        {
          p->writeStdin("Never");
          required="";
        }
      if (required.find("sign_uid.class")!=-1)
        {
          p->writeStdin("");
          required="";
        }
      if (required.find("sign_uid.okay")!=-1)
        {
          p->writeStdin("Y");
          required="";
        }
      if (required.find("passphrase.enter")!=-1)
        {
          p->writeStdin(QString(passphrase));
          passphrase="xxxxxxxxxxxxxx";
          required="";
          step=2;
        }
      if ((step==2) && (required.find("keyedit.prompt")!=-1))
        {
          p->writeStdin("save");
          required="";
        }
      if (required.find("BAD_PASSPHRASE")!=-1)
        {
		  p->writeStdin("quit");
		  p->closeWhenDone();
          signSuccess=2;  /////  bad passphrase
        }
      if ((required.find("GET_")!=-1) && (signSuccess!=2)) /////// gpg asks for something unusal, turn to konsole mode
        {
          signSuccess=1;  /////  switching to console mode
		  p->writeStdin("quit");
		  p->closeWhenDone();
/*
#if (KDE_VERSION >= 310)
pop = new KPassivePopup();
pop->setView(i18n("Unexpected gpg query"),i18n("Kgpg cannot sign this key in GUI mode... switching to konsole"),KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop));
		pop->setTimeout(3200);
	  	pop->show();	  
	  	QRect qRect(QApplication::desktop()->screenGeometry());
		int iXpos=qRect.width()/2-pop->width()/2;
		int iYpos=qRect.height()/2-pop->height()/2;
      	pop->move(iXpos,iYpos);
#else
	clippop = new QDialog( 0,0,false,WStyle_Customize | WStyle_NormalBorder);
              QVBoxLayout *vbox=new QVBoxLayout(clippop,3);
              QLabel *tex=new QLabel(clippop);
              tex->setText(i18n("<b>Unexpected gpg query</b>"));
			  QLabel *tex2=new QLabel(clippop);
			  //tex2->setTextFormat(Qt::PlainText);
			  tex2->setText(i18n("Kgpg cannot sign this key in GUI mode... switching to konsole"));
              vbox->addWidget(tex);
			  vbox->addWidget(tex2);
              clippop->setMinimumWidth(250);
              clippop->adjustSize();
			  clippop->show();
 QTimer::singleShot( 3200, this, SLOT(signkillDisplayClip()));
#endif
		  		  
		  
          KProcess *conprocess=new KProcess();
          *conprocess<< "konsole"<<"-e"<<"gpg";
          *conprocess<<"--no-secmem-warning"<<"-u"<<konsSignKey;
          if (!konsLocal)
            *conprocess<<"--sign-key"<<konsKeyID;
          else
            *conprocess<<"--lsign-key"<<konsKeyID;
          conprocess->start(KProcess::Block);
          emit signatureFinished(0);
        */
		}
    }
  //p->ackRead();
}

void KgpgInterface::signkillDisplayClip()
{
/*
#if (KDE_VERSION < 310)
delete clippop;
#endif
*/
}


void KgpgInterface::signover(KProcess *)
{
  if ((signSuccess==3) || (signSuccess==2)) emit signatureFinished(signSuccess);  ////   signature successfull or bad passphrase 
  else 
  {
  KDetailedConsole *q=new KDetailedConsole(0,"sign_error",i18n("<b>Encryption failed:</b><br>Do you want to try signing in a console ?"),output);
  if (q->exec()==QDialog::Accepted) openSignConsole();
  else emit signatureFinished(0);
  }
}

void KgpgInterface::openSignConsole()
{
KProcess *conprocess=new KProcess();
          *conprocess<< "konsole"<<"-e"<<"gpg";
          *conprocess<<"--no-secmem-warning"<<"-u"<<konsSignKey;
          if (!konsLocal)
            *conprocess<<"--sign-key"<<konsKeyID;
          else
            *conprocess<<"--lsign-key"<<konsKeyID;
          conprocess->start(KProcess::Block);
          emit signatureFinished(3);
}

////////////////////////////////////////////////////////////////////////////     delete signature


void KgpgInterface::KgpgDelSignature(QString keyID,QString signKeyID)
{
  if (checkuid(keyID)>0)
    {
      KMessageBox::sorry(0,i18n("This key has more than one user ID.\nEdit the key manually to delete signature."));
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
          if (encResult.find(message)!=-1)
            break;
          signb++;
        }
      else if (encResult.startsWith("rev"))
        signb++;
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
      if ((step==0) && (required.find("keyedit.prompt")!=-1))
        {
          p->writeStdin("uid 1");
          step=1;
          required="";
        }
      if ((step==1) && (required.find("keyedit.prompt")!=-1))
        {
          p->writeStdin("delsig");
          step=2;
          required="";
        }
      if ((step==2) && (required.find("keyedit.delsig")!=-1))
        {

          if (sigsearch==signb)
            {
              p->writeStdin("Y");
              step=3;
            }
          else
            p->writeStdin("n");
          sigsearch++;
          required="";
        }
      if ((step==3) && (required.find("keyedit.prompt")!=-1))
        {
          p->writeStdin("save");
          required="";
          deleteSuccess=true;
        }
      if (required.find("GET_LINE")!=-1)
        {
          p->writeStdin("quit");
          p->closeWhenDone();
		  deleteSuccess=false;
        }
    }
  //p->ackRead();
}

void KgpgInterface::delsignover(KProcess *)
{
  emit delsigfinished(deleteSuccess);
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
      if (encResult.startsWith("uid"))
        uidcnt++;
    }
  pclose(fp);
  return uidcnt;
}

//////////////////////////////////////////////////////////////    key import

void KgpgInterface::importKeyURL(KURL url, bool importSecret)
{
  /////////////      import a key

  if( KIO::NetAccess::download( url, tempKeyFile ) )
    {
      message=QString::null;
      KProcIO *conprocess=new KProcIO();
      *conprocess<< "gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--import";
      if (importSecret)
        *conprocess<<"--allow-secret-key-import";
      *conprocess<<tempKeyFile;
      QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(importURLover(KProcess *)));
	  QObject::connect(conprocess, SIGNAL(readReady(KProcIO *)),this, SLOT(importprocess(KProcIO *)));
      conprocess->start(KProcess::NotifyOnExit,true);
    }
}

void KgpgInterface::importKey(QString keystr, bool importSecret)
{
  /////////////      import a key

      message=QString::null;
      KProcIO *conprocess=new KProcIO();
      *conprocess<< "gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--import";
      if (importSecret)
        *conprocess<<"--allow-secret-key-import";
      QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(importover(KProcess *)));
      QObject::connect(conprocess, SIGNAL(readReady(KProcIO *)),this, SLOT(importprocess(KProcIO *)));
      conprocess->start(KProcess::NotifyOnExit,true);
      conprocess->writeStdin(keystr, true);
      conprocess->closeWhenDone();
}

void KgpgInterface::importover(KProcess *)
{
    KMessageBox::information(0,message);
    emit importfinished();
}

void KgpgInterface::importURLover(KProcess *)
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

  QString gpgcmd="gpg --no-tty --no-secmem-warning --batch --status-fd 1 -d "+KShellProcess::quote(url.path());
  //////////   encode with untrusted keys or armor if checked by user
  fp = popen(QFile::encodeName(gpgcmd), "r");
  while ( fgets( buffer, sizeof(buffer), fp))
    {
      encResult=QString::fromUtf8(buffer);
      if (encResult.find("USERID_HINT",0,false)!=-1)
        {
          encResult=encResult.section("HINT",1,1);
          encResult=encResult.stripWhiteSpace();
          int cut=encResult.find(' ',0,false);
          encResult.remove(0,cut);
          if (encResult.find("(",0,false)!=-1)
            encResult=encResult.section('(',0,0)+encResult.section(')',-1,-1);
           if (IDs.find(encResult)==-1)
            {
              if (!IDs.isEmpty())
                IDs+=i18n(" or ");
              IDs+=encResult;
            }
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
      encResult=QString::fromUtf8(buffer);
      if (encResult.find("USERID_HINT",0,false)!=-1)
        {
          encResult=encResult.section("HINT",1,1);
          encResult=encResult.stripWhiteSpace();
          int cut=encResult.find(' ',0,false);
          encResult.remove(0,cut);
          if (encResult.find("(",0,false)!=-1)
            encResult=encResult.section('(',0,0)+encResult.section(')',-1,-1);
		  
		  if (IDs.find(encResult)==-1)
            {
              if (!IDs.isEmpty())
                IDs+=i18n(" or ");
              IDs+=encResult;
            }
		  
        }
    }
  pclose(fp);
  return IDs;
}


#include "kgpginterface.moc"
