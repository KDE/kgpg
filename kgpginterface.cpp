/***************************************************************************
                          kgpginterface.cpp  -  description
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

#include <stdio.h>

#include <qdialog.h>
#include <qclipboard.h>
#include <qlayout.h>
#include <qregexp.h>
#include <qstring.h>
#include <qlabel.h>
#include <qapplication.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kpassdlg.h>
#include <kmdcodec.h>
#include <klineedit.h>
#include <kcharsets.h>
#include <kpassivepopup.h>
#include <kiconloader.h>

#include <qtextcodec.h>

#include "kgpginterface.h"

KgpgInterface::KgpgInterface()
{}


int KgpgInterface::getGpgVersion()
{
FILE *fp;
        QString readResult,gpgString;
        char buffer[200];
	bool readLine=true;

        QString gpgcmd="gpg --version";
        
        fp = popen(QFile::encodeName(gpgcmd), "r");
        while ( fgets( buffer, sizeof(buffer), fp)) {
                readResult=buffer;
                if (readLine) {
		gpgString=readResult.stripWhiteSpace().section(' ',-1);
		readLine=false;
		}
        }
        pclose(fp);
	return (100*gpgString.section('.',0,0).toInt()+10*gpgString.section('.',1,1).toInt()+gpgString.section('.',2,2).toInt());
}

void KgpgInterface::updateIDs(QString txtString)
{
 int cut=txtString.find(' ',22,false);
          txtString.remove(0,cut);
          if (txtString.find("(",0,false)!=-1)
            txtString=txtString.section('(',0,0)+txtString.section(')',-1);
	    txtString.replace(QRegExp("<"),"&lt;");
          if (userIDs.find(txtString)==-1)
            {
              if (!userIDs.isEmpty())
                userIDs+=i18n(" or ");
              userIDs+=txtString;
            }
}

void KgpgInterface::KgpgEncryptFile(QStringList encryptKeys,KURL srcUrl,KURL destUrl, QStringList Options, bool symetrical)
{
        sourceFile=srcUrl;
        message=QString::null;

        KProcIO *proc=new KProcIO();
	*proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--status-fd=2"<<"--command-fd=0";
	for ( QStringList::Iterator it = Options.begin(); it != Options.end(); ++it ) 
       		if (!QFile::encodeName(*it).isEmpty()) *proc<< QFile::encodeName(*it);

                *proc<<"--output"<<QFile::encodeName(destUrl.path());
		
		if (!symetrical) {
		*proc<<"-e";
		for ( QStringList::Iterator it = encryptKeys.begin(); it != encryptKeys.end(); ++it )
       		*proc<<"--recipient"<< *it;
        	} else  ////////////   symetrical encryption, prompt for password
                *proc<<"-c";
		
		*proc<<QFile::encodeName(srcUrl.path());

        /////////  when process ends, update dialog infos
        QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(encryptfin(KProcess *)));
        QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(readencprocess(KProcIO *)));
        proc->start(KProcess::NotifyOnExit,true);
}


KgpgInterface::~KgpgInterface()
{}


void KgpgInterface::encryptfin(KProcess *)
{
        if (message.find("END_ENCRYPTION")!=-1)
                emit encryptionfinished(sourceFile);
        else {
                emit errormessage(message);
        }
}

void KgpgInterface::readencprocess(KProcIO *p)
{
        QString required;
        while (p->readln(required,true)!=-1) {
                if (required.find("BEGIN_ENCRYPTION",0,false)!=-1)
                        emit processstarted(sourceFile.path());
                if (required.find("GET_")!=-1) {
                        if (required.find("openfile.overwrite.okay")!=-1)
                                p->writeStdin("Yes");
                        else if ((required.find("passphrase.enter")!=-1)) {
                                QCString passphrase;
                                int code=KPasswordDialog::getNewPassword(passphrase,i18n("Enter passphrase for your file (symmetrical encryption):"));
                                if (code!=QDialog::Accepted) {
                                        delete p;
                                        emit processaborted(true);
                                        return;
                                }
                                p->writeStdin(passphrase,true);
                        } else {
                                p->writeStdin("quit");
                                p->closeWhenDone();
                        }
                }
                message+=required+"\n";
        }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////   File decryption

void KgpgInterface::KgpgDecryptFile(KURL srcUrl,KURL destUrl,QStringList Options)
{
        message=QString::null;
        step=3;
	decryptUrl=srcUrl.path();
        userIDs=QString::null;
        anonymous=false;
	kdDebug(2100)<<"Starting decrypt"<<endl;
        KProcIO *proc=new KProcIO();

                *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--status-fd=2"<<"--command-fd=0";

		for ( QStringList::Iterator it = Options.begin(); it != Options.end(); ++it )
       		if (!QFile::encodeName(*it).isEmpty()) *proc<< QFile::encodeName(*it);

		if (!destUrl.filename().isEmpty()) // a filename was entered
				*proc<<"-o"<<QFile::encodeName(destUrl.path());
        
                *proc<<"-d"<<QFile::encodeName(srcUrl.path());
        
        QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(decryptfin(KProcess *)));
        QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(readdecprocess(KProcIO *)));
        proc->start(KProcess::NotifyOnExit,true);
}

void KgpgInterface::decryptfin(KProcess *)
{
        if ((message.find("DECRYPTION_OKAY")!=-1) && (message.find("END_DECRYPTION")!=-1)) //&& (message.find("GOODMDC")!=-1)
                emit decryptionfinished();
        else
                emit errormessage(message);
}

void KgpgInterface::readdecprocess(KProcIO *p)
{
        QString required;
        while (p->readln(required,true)!=-1) {
                if (required.find("BEGIN_DECRYPTION",0,false)!=-1)
                        emit processstarted(decryptUrl);
                if (required.find("USERID_HINT",0,false)!=-1)
			updateIDs(required);

                if (required.find("ENC_TO")!=-1) {
                        if (required.find("0000000000000000")!=-1)
                                anonymous=true;
                }
                if (required.find("GET_")!=-1) {
                        if (required.find("openfile.overwrite.okay")!=-1)
                                p->writeStdin("Yes");
                        else if ((required.find("passphrase.enter")!=-1)) {
                                if (userIDs.isEmpty())
                                        userIDs=i18n("[No user id found]");
                                userIDs.replace(QRegExp("<"),"&lt;");
                                QCString passphrase;
                                QString passdlgmessage;
                                if (anonymous)
                                        passdlgmessage=i18n("<b>No user id found</b>. Trying all secret keys.<br>");
                                if ((step<3) && (!anonymous))
                                        passdlgmessage=i18n("<b>Bad passphrase</b>. You have %1 tries left.<br>").arg(step);

                                passdlgmessage+=i18n("Enter passphrase for <b>%1</b>").arg(checkForUtf8(userIDs));
                                int code=KPasswordDialog::getPassword(passphrase,passdlgmessage);
                                if (code!=QDialog::Accepted) {
                                        delete p;
                                        emit processaborted(true);
                                        return;
                                }
                                p->writeStdin(passphrase,true);
                                userIDs=QString::null;
                                step--;
                        } else {
                                p->writeStdin("quit");
                                p->closeWhenDone();
                        }
                }
                message+=required+"\n";
        }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////    Text encryption


void KgpgInterface::KgpgEncryptText(QString text,QStringList userIDs, QStringList Options)
{
        message=QString::null;
	//QTextCodec *codec = KGlobal::charsets()->codecForName(KGlobal::locale()->encoding());
	QTextCodec *codec =QTextCodec::codecForLocale ();
	if (codec->canEncode(text)) txtprocess=text;
	else txtprocess=text.utf8();
	
        txtsent=false;
        KProcIO *proc=new KProcIO();
        *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--command-fd=0"<<"--status-fd=2";
        
	for ( QStringList::Iterator it = Options.begin(); it != Options.end(); ++it )
       		if (!QFile::encodeName(*it).isEmpty()) *proc<< QFile::encodeName(*it);
	
	if (!userIDs.isEmpty())
	{
        *proc<<"-e";
	for ( QStringList::Iterator it = userIDs.begin(); it != userIDs.end(); ++it )
       		*proc<<"--recipient"<< *it;
	}
	else *proc<<"-c";

        /////////  when process ends, update dialog infos

        QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(txtencryptfin(KProcess *)));
        QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(txtreadencprocess(KProcIO *)));
        proc->start(KProcess::NotifyOnExit,true);
}


void KgpgInterface::txtencryptfin(KProcess *)
{
        if (txtsent)
                emit txtencryptionfinished(QString::fromUtf8(message.ascii()));
        else
                emit txtencryptionfinished(QString::null);
}

void KgpgInterface::txtreadencprocess(KProcIO *p)
{
        QString required;
        while (p->readln(required,true)!=-1) {
	if ((required.find("passphrase.enter")!=-1))
            {
              QCString passphrase;
              QString passdlgmessage=i18n("Enter passphrase (symmetrical encryption)");
              int code=KPasswordDialog::getNewPassword(passphrase,passdlgmessage);
	      if (code!=QDialog::Accepted)
                {
                  delete p;
                  return;
                }
              p->writeStdin(passphrase,true);
            } 
	    else
		if (required.find("BEGIN_ENCRYPTION")!=-1) {
			emit txtencryptionstarted();
                        p->writeStdin(txtprocess,false);
                        p->closeWhenDone();
                } else
                        if (required.find("END_ENCRYPTION")!=-1)
                                txtsent=true;
                        else
                                if (!required.startsWith("[GNUPG:]")) message+=required+"\n";
        }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////     Text decryption

void KgpgInterface::KgpgDecryptText(QString text,QStringList Options)
{
  message=QString::null;
  userIDs=QString::null;
  step=3;
  anonymous=false;
txtprocess=text;
decfinished=false;
decok=false;
badmdc=false;
 KProcIO *proc=new KProcIO();
	  *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--command-fd=0"<<"--status-fd=1"<<"--no-batch";
      	for ( QStringList::Iterator it = Options.begin(); it != Options.end(); ++it )
       		if (!QFile::encodeName(*it).isEmpty()) *proc<< QFile::encodeName(*it);

      *proc<<"-d";

  /////////  when process ends, update dialog infos

  QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(txtdecryptfin(KProcess *)));
  QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(txtreaddecprocess(KProcIO *)));
  proc->start(KProcess::NotifyOnExit,false);
  proc->writeStdin(txtprocess,false);
}

void KgpgInterface::txtdecryptfin(KProcess *)
{
if ((decok) && (!badmdc)) 
{
message.remove(0,message.find("BEGIN_DECRYPTION")+17);
if (message.find("[GNUPG:] DECRYPTION_OKAY")!=-1) message=message.left(message.findRev("[GNUPG:] DECRYPTION_OKAY",-1,false)); 
emit txtdecryptionfinished(message);
}
else if (badmdc) 
{
KMessageBox::sorry(0,i18n("Bad MDC detected. The encrypted text has been manipulated."));
emit txtdecryptionfailed(message);
}
else
emit txtdecryptionfailed(message);
}

void KgpgInterface::txtreaddecprocess(KProcIO *p)
{
  QString required;
  while (p->readln(required,true)!=-1)
    {
	{
	if (required.find("USERID_HINT",0,false)!=-1)
        updateIDs(required);
	
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
              passdlgmessage+=i18n("Enter passphrase for <b>%1</b>").arg(checkForUtf8(userIDs));
              int code=KPasswordDialog::getPassword(passphrase,passdlgmessage);
              if (code!=QDialog::Accepted)
                {
                  delete p;
                  emit processaborted(true);
                  return;
                }
              p->writeStdin(passphrase,true);
              userIDs=QString::null;
              //p->writeStdin("\n");
              step--;
            }
		  else
           p->writeStdin("quit");
        }
		message+=required+"\n";
		if (required.find("BEGIN_DECRYPTION")!=-1)
      		{
			p->closeStdin();
			required=QString::null;
			}

	  if (required.find("END_DECRYPTION")!=-1) decfinished=true;
	  if (required.find("DECRYPTION_OKAY")!=-1) decok=true;
	  if (required.find("BADMDC")!=-1) badmdc=true;
    }
	}
}


void KgpgInterface::KgpgDecryptFileToText(KURL srcUrl,QStringList Options)
{
        
  message=QString::null;
  userIDs=QString::null;
  step=3;
  anonymous=false;
decfinished=false;
decok=false;
badmdc=false;
 KProcIO *proc=new KProcIO();
	  *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--command-fd=0"<<"--status-fd=1"<<"--no-batch"<<"-o"<<"-";
      	for ( QStringList::Iterator it = Options.begin(); it != Options.end(); ++it ) {
       		if (!QFile::encodeName(*it).isEmpty()) *proc<< QFile::encodeName(*it);
    		}
      *proc<<"-d"<<QFile::encodeName(srcUrl.path());

  /////////  when process ends, update dialog infos

  QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(txtdecryptfin(KProcess *)));
  QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(txtreaddecprocess(KProcIO *)));
  proc->start(KProcess::NotifyOnExit,false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////   MD5

Md5Widget::Md5Widget(QWidget *parent, const char *name,KURL url):KDialogBase( parent, name, true,i18n("MD5 Checksum"),Apply | Close)
{
        setButtonApplyText(i18n("Compare MD5 with Clipboard"));
        mdSum=QString::null;
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
        text = cb->text(QClipboard::Clipboard);
        if ( !text.isEmpty() ) {
                text=text.stripWhiteSpace();
                while (text.find(' ')!=-1)
                        text.remove(text.find(' '),1);
                if (text==mdSum) {
                        TextLabel1_2->setText(i18n("<b>Correct checksum</b>, file is ok."));
                        KLed1->setColor(QColor(0,255,0));
                        KLed1->on();
                }//KMessageBox::sorry(0,"OK");
                else if (text.length()!=mdSum.length())
                        KMessageBox::sorry(0,i18n("Clipboard content is not a MD5 sum."));
                else {
                        TextLabel1_2->setText(i18n("<b>Wrong checksum, FILE CORRUPTED</b>"));
                        KLed1->setColor(QColor(255,0,0));
                        KLed1->on();
                }
        }
}

/////////////////////////////////////////////////////////////////////////////////////////////   signatures


void KgpgInterface::KgpgSignFile(QString keyID,KURL srcUrl,QStringList Options)
{
        //////////////////////////////////////   create a detached signature for a chosen file
        message=QString::null;
        step=3;
        /////////////       create gpg command
        KProcIO *proc=new KProcIO();
        keyID=keyID.stripWhiteSpace();
        *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--status-fd=2"<<"--command-fd=0"<<"-u"<<keyID.local8Bit();
		for ( QStringList::Iterator it = Options.begin(); it != Options.end(); ++it )
       		if (!QFile::encodeName(*it).isEmpty()) *proc<< QFile::encodeName(*it);

	*proc<<"--output"<<QFile::encodeName(srcUrl.path()+".sig");
        *proc<<"--detach-sig"<<QFile::encodeName(srcUrl.path());

        QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(signfin(KProcess *)));
        QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(readsignprocess(KProcIO *)));
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
        while (p->readln(required,true)!=-1) {
                if (required.find("USERID_HINT",0,false)!=-1) 
		updateIDs(required);

                if (required.find("GET_")!=-1) {
                        if (required.find("openfile.overwrite.okay")!=-1)
                                p->writeStdin("Yes");
                        else if ((required.find("passphrase.enter")!=-1)) {
                                if (userIDs.isEmpty())
                                        userIDs=i18n("[No user id found]");
                                QCString passphrase;
                                QString passdlgmessage;
                                if (step<3)
                                        passdlgmessage=i18n("<b>Bad passphrase</b>. you have %1 tries left.<br>").arg(step);
                                passdlgmessage+=i18n("Enter passphrase for <b>%1</b>").arg(checkForUtf8(userIDs));
                                int code=KPasswordDialog::getPassword(passphrase,passdlgmessage);
                                if (code!=QDialog::Accepted) {
                                        delete p;
                                        emit signfinished();
                                        return;
                                }
                                p->writeStdin(passphrase,true);
                                userIDs=QString::null;
                                step--;
                        } else {
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
        message=QString::null;
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
        while (p->readln(required,true)!=-1) {
                if (required.find("GET_")!=-1) {
                        p->writeStdin("quit");
                        p->closeWhenDone();
                }
                message+=required+"\n";
        }
}

void KgpgInterface::verifyfin(KProcess *)
{
        QString keyID,keyMail;
        if ((message.find("VALIDSIG")!=-1) && (message.find("GOODSIG")!=-1) && (message.find("BADSIG")==-1)) {
                message.remove(0,message.find("GOODSIG")+7);
                message=message.section('\n',0,0);
                message=message.stripWhiteSpace();
                keyID=message.section(' ',0,0);
                message.remove(0,keyID.length());
                keyMail=message;
                KMessageBox::information(0,i18n("<qt>Good signature from:<br><b>%1</b><br>Key ID: %2</qt>").arg(keyMail.replace(QRegExp("<"),"&lt;")).arg(keyID),file.filename());
        } else if (message.find("UNEXPECTED")!=-1)
                KMessageBox::sorry(0,i18n("No signature found."),file.filename());
        else if (message.find("BADSIG")!=-1) {
                message.remove(0,message.find("BADSIG")+6);
                message=message.section('\n',0,0);
                message=message.stripWhiteSpace();
                keyID=message.section(' ',0,0);
//                 message.remove(0,keyID.length());
                keyMail=message;
                KMessageBox::sorry(0,i18n("<qt><b>BAD signature</b> from:<br> %1<br>Key id: %2<br><br>"
                                          "<b>The file is corrupted!</b></qt>").arg(keyMail.replace(QRegExp("<"),"&lt;")).arg(keyID),file.filename());
        } else if (message.find("NO_PUBKEY")!=-1) {
                message.remove(0,message.find("NO_PUBKEY")+9);
                message=message.section('\n',0,0);
                keyID=message.stripWhiteSpace();
                if (KMessageBox::questionYesNo(0,i18n("<qt><b>Missing signature:</b><br>Key id: %1<br><br>"
                                                      "Do you want to import this key from a keyserver?</qt>").arg(keyID),file.filename())==KMessageBox::Yes)
                        emit verifyquerykey(keyID);
                emit verifyfinished();
                return;
        } else
                KMessageBox::sorry(0,message);
        emit verifyfinished();
}



////////////////////////////////////////////////////////////   sign a key

void KgpgInterface::KgpgSignKey(QString keyID,QString signKeyID,QString signKeyMail,bool local,int checking)
{
        signKeyMail.replace(QRegExp("<"),"&lt;");
        konsChecked=checking;
        konsLocal=local;
        konsSignKey=signKeyID;
        konsKeyID=keyID;
        errMessage=QString::null;
        if (checkuid(keyID)>0) 
	{
                openSignConsole();
		return;
	}
	
        signSuccess=0;
        step=1;
        output=QString::null;
        KProcIO *conprocess=new KProcIO();
        *conprocess<<"gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--command-fd=0"<<"--status-fd=2"<<"-u"<<signKeyID;
        *conprocess<<"--edit-key"<<keyID;
	if (local) *conprocess<<"lsign";
	else *conprocess<<"sign";
        QObject::connect(conprocess,SIGNAL(readReady(KProcIO *)),this,SLOT(sigprocess(KProcIO *)));
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(signover(KProcess *)));
        conprocess->start(KProcess::NotifyOnExit,true);
}

void KgpgInterface::sigprocess(KProcIO *p)
{
        QString required=QString::null;

        while (p->readln(required,true)!=-1)
        {

                output+=required+"\n";
                if (required.find("USERID_HINT",0,false)!=-1) 
		updateIDs(required);
		
                if (signSuccess==4) {
                        if (required.find("GET_")!=-1)
                                p->writeStdin("quit");
                        p->closeWhenDone();
                        return;
                }

                if ((required.find("GOOD_PASSPHRASE")!=-1)) {
                        signSuccess=3;
                        step=2;
                }

                if (required.find("sign_uid.expire")!=-1) {
                        p->writeStdin("Never");
                        required=QString::null;
                }
                if (required.find("sign_uid.class")!=-1) {
                        p->writeStdin(QString::number(konsChecked));
                        required=QString::null;
                }
                if (required.find("sign_uid.okay")!=-1) {
                        p->writeStdin("Y");
                        required=QString::null;
                }
		
		if (required.find("sign_all.okay")!=-1) {
                        p->writeStdin("Y");
                        required=QString::null;
                }
		
                if (required.find("passphrase.enter")!=-1) {
                        QCString signpass;
                        int code=KPasswordDialog::getPassword(signpass,i18n("<qt>%1 Enter passphrase for <b>%2</b>:</qt>").arg(errMessage).arg(checkForUtf8(userIDs)));
                        if (code!=QDialog::Accepted) {
                                signSuccess=4;  /////  aborted by user mode
                                required=QString::null;
                                p->writeStdin("quit");
                                p->closeWhenDone();
                                return;
                        }
                        p->writeStdin(signpass,true);
                        required=QString::null;
                        //               step=2;
                }
                if ((step==2) && (required.find("keyedit.prompt")!=-1)) {
                        p->writeStdin("save");
                        required=QString::null;
                }
                if (required.find("BAD_PASSPHRASE")!=-1) {
                        errMessage=i18n("<b>Bad passphrase</b>. Try again.</br>");
                        required=QString::null;
                        signSuccess=2;  /////  bad passphrase
                }
                if (required.find("GET_")!=-1) /////// gpg asks for something unusal, turn to konsole mode
                {
                        if (signSuccess!=2)
                                signSuccess=1;  /////  switching to console mode
                        p->writeStdin("quit");
                        p->closeWhenDone();

                }
        }
}


void KgpgInterface::signover(KProcess *)
{
        if (signSuccess>1)
                emit signatureFinished(signSuccess);  ////   signature successful or bad passphrase
        else {
                KDetailedConsole *q=new KDetailedConsole(0,"sign_error",i18n("<qt>Signing key <b>%1</b> with key <b>%2</b> failed.<br>"
                                    "Do you want to try signing the key in console mode?</qt>").arg(konsKeyID).arg(konsSignKey),output);
                if (q->exec()==QDialog::Accepted)
                        openSignConsole();
                else
                        emit signatureFinished(0);
        }
}

void KgpgInterface::openSignConsole()
{
        KProcess conprocess;
	KConfig *config = KGlobal::config();
	config->setGroup("General");
	conprocess<< config->readEntry("TerminalApplication","konsole");
        conprocess<<"-e"<<"gpg";
        conprocess<<"--no-secmem-warning"<<"--expert"<<"-u"<<konsSignKey;
        if (!konsLocal)
                conprocess<<"--sign-key"<<konsKeyID;
        else
                conprocess<<"--lsign-key"<<konsKeyID;
        conprocess.start(KProcess::Block);
	emit signatureFinished(3);

}

////////////////////////////////////////////////////////////////////////////     delete signature


void KgpgInterface::KgpgDelSignature(QString keyID,QString signKeyID)
{
        if (checkuid(keyID)>0) {
                KMessageBox::sorry(0,i18n("This key has more than one user ID.\nEdit the key manually to delete signature."));
                return;
        }

        message=signKeyID.remove(0,2);
        deleteSuccess=false;
        step=0;
        
        FILE *fp;
        QString encResult;
        char buffer[200];
        signb=0;
        sigsearch=0;

        QString gpgcmd="gpg --no-tty --no-secmem-warning --with-colon --list-sigs "+keyID;
        fp = popen(QFile::encodeName(gpgcmd), "r");
        while ( fgets( buffer, sizeof(buffer), fp)) {
                encResult=buffer;
                if (encResult.startsWith("sig")) {
                        if (encResult.find(message)!=-1)
                                break;
                        signb++;
                } else if (encResult.startsWith("rev"))
                        signb++;
        }
        pclose(fp);
        KProcIO *conprocess=new KProcIO();
        *conprocess<<"gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--command-fd=0"<<"--status-fd=2";
        *conprocess<<"--edit-key"<<keyID<<"uid 1"<<"delsig";
        QObject::connect(conprocess,SIGNAL(readReady(KProcIO *)),this,SLOT(delsigprocess(KProcIO *)));
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(delsignover(KProcess *)));
        conprocess->start(KProcess::NotifyOnExit,true);
}


void KgpgInterface::delsigprocess(KProcIO *p)//ess *p,char *buf, int buflen)
{

        QString required=QString::null;
        while (p->readln(required,true)!=-1)
        {
                if (required.find("keyedit.delsig")!=-1){

                        if ((sigsearch==signb) && (step==0)) {
                                p->writeStdin("Y");
                                step=1;
                        } else
                                p->writeStdin("n");
                        sigsearch++;
                        required=QString::null;
                }
                if ((step==1) && (required.find("keyedit.prompt")!=-1)) {
                        p->writeStdin("save");
                        required=QString::null;
                        deleteSuccess=true;
                }
                if (required.find("GET_LINE")!=-1) {
                        p->writeStdin("quit");
                        p->closeWhenDone();
                        deleteSuccess=false;
                }
        }
}

void KgpgInterface::delsignover(KProcess *)
{
        emit delsigfinished(deleteSuccess);
}

/////////////////////////////////////////////////// check if a key has more than one id

int KgpgInterface::checkuid(QString KeyID)
{
        FILE *fp;
        QString encResult;
        char buffer[200];
        int  uidcnt=0;

        QString gpgcmd="gpg --no-tty --no-secmem-warning --with-colon --list-sigs "+KeyID;
        //////////   encode with untrusted keys or armor if checked by user
        fp = popen(QFile::encodeName(gpgcmd), "r");
        while (fgets(buffer, sizeof(buffer), fp)) {
                encResult=buffer;
                if (encResult.startsWith("uid"))
                        uidcnt++;
        }
        pclose(fp);
        return uidcnt;
}


///////////////////////////////////////////////////////////////    change key expiration


void KgpgInterface::KgpgKeyExpire(QString keyID,QDate date,bool unlimited)
{
        expSuccess=0;
        step=0;
        if (unlimited)
                expirationDelay=0;
        else
                expirationDelay=QDate::currentDate().daysTo(date);
        output=QString::null;
        KProcIO *conprocess=new KProcIO();
        *conprocess<<"gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--command-fd=0"<<"--status-fd=2";
        *conprocess<<"--edit-key"<<keyID<<"expire";
        QObject::connect(conprocess,SIGNAL(readReady(KProcIO *)),this,SLOT(expprocess(KProcIO *)));
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(expover(KProcess *)));
        conprocess->start(KProcess::NotifyOnExit,KProcess::AllOutput);

}

void KgpgInterface::expprocess(KProcIO *p)
{
        QString required=QString::null;

        while (p->readln(required,true)!=-1) {
                output+=required+"\n";

                if (required.find("USERID_HINT",0,false)!=-1) 
		updateIDs(required);
		
                if ((required.find("GOOD_PASSPHRASE")!=-1)) {
                        expSuccess=3;
                        step=2;
                }

                if (required.find("keygen.valid")!=-1) {
                        p->writeStdin(QString::number(expirationDelay));
                        required=QString::null;
                }

                if (required.find("passphrase.enter")!=-1) {
                        QCString signpass;
                        int code=KPasswordDialog::getPassword(signpass,i18n("<qt>Enter passphrase for <b>%1</b>:</qt>").arg(checkForUtf8(userIDs)));
                        if (code!=QDialog::Accepted) {
                                expSuccess=3;  /////  aborted by user mode
                                p->writeStdin("quit");
                                p->closeWhenDone();
                                return;
                        }
                        p->writeStdin(signpass,true);
                        required=QString::null;
                        //               step=2;
                }
                if ((step==2) && (required.find("keyedit.prompt")!=-1)) {
                        p->writeStdin("save");
                        required=QString::null;
                }
		if ((step==2) && (required.find("keyedit.save.okay")!=-1)) {
                        p->writeStdin("YES");
                        required=QString::null;
                }
                if (required.find("BAD_PASSPHRASE")!=-1) {
                        p->writeStdin("quit");
                        p->closeWhenDone();
                        expSuccess=2;  /////  bad passphrase
                }
                if ((required.find("GET_")!=-1) && (expSuccess!=2)) /////// gpg asks for something unusal, turn to konsole mode
                {
                        expSuccess=1;  /////  switching to console mode
                        p->writeStdin("quit");
                        p->closeWhenDone();

                }
        }
}



void KgpgInterface::expover(KProcess *)
{
        if ((expSuccess==3) || (expSuccess==2))
                emit expirationFinished(expSuccess);  ////   signature successful or bad passphrase
        else {
                KDetailedConsole *q=new KDetailedConsole(0,"sign_error",i18n("<qt><b>Changing expiration failed.</b><br>"
                                    "Do you want to try changing the key expiration in console mode?</qt>"),output);
                if (q->exec()==QDialog::Accepted)
                        KMessageBox::sorry(0,"work in progress...");
                //openSignConsole();
                else
                        emit expirationFinished(0);
        }
}


///////////////////////////////////////////////////////////////    change key trust


void KgpgInterface::KgpgTrustExpire(QString keyID,QString keyTrust)
{
        if (keyTrust==i18n("Don't know"))
                trustValue=1;
        if (keyTrust==i18n("Do NOT trust"))
                trustValue=2;
        if (keyTrust==i18n("Marginally"))
                trustValue=3;
        if (keyTrust==i18n("Fully"))
                trustValue=4;
        if (keyTrust==i18n("Ultimately"))
                trustValue=5;

        output=QString::null;
        KProcIO *conprocess=new KProcIO();
        *conprocess<<"gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--command-fd=0"<<"--status-fd=2";
        *conprocess<<"--edit-key"<<keyID<<"trust";
        QObject::connect(conprocess,SIGNAL(readReady(KProcIO *)),this,SLOT(trustprocess(KProcIO *)));
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(trustover(KProcess *)));
        conprocess->start(KProcess::NotifyOnExit,KProcess::AllOutput);

}

void KgpgInterface::trustprocess(KProcIO *p)
{
        QString required=QString::null;

        while (p->readln(required,true)!=-1) {
                output+=required+"\n";

                if (required.find("edit_ownertrust.set_ultimate.okay")!=-1) {
                        p->writeStdin("YES");
                        required=QString::null;
                }

                if (required.find("edit_ownertrust.value")!=-1) {
                        p->writeStdin(QString::number(trustValue));
                        required=QString::null;
                }

                if (required.find("keyedit.prompt")!=-1) {
                        p->writeStdin("save");
                        required=QString::null;
                }

                if (required.find("GET_")!=-1) /////// gpg asks for something unusal, turn to konsole mode
                {
                        expSuccess=1;  /////  switching to console mode
                        p->writeStdin("quit");
                        p->closeWhenDone();

                }
        }
}



void KgpgInterface::trustover(KProcess *)
{
        emit trustfinished();
}


///////////////////////////////////////////////////////////////    change passphrase


void KgpgInterface::KgpgChangePass(QString keyID)
{
        step=1;
        output=QString::null;
        message=QString::null;
        KProcIO *conprocess=new KProcIO();
        *conprocess<<"gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--no-use-agent"<<"--command-fd=0"<<"--status-fd=2";
        *conprocess<<"--edit-key"<<keyID<<"passwd";
        QObject::connect(conprocess,SIGNAL(readReady(KProcIO *)),this,SLOT(passprocess(KProcIO *)));
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(passover(KProcess *)));
        conprocess->start(KProcess::NotifyOnExit,KProcess::AllOutput);

}

void KgpgInterface::passprocess(KProcIO *p)
{
        QString required=QString::null;

        while (p->readln(required,true)!=-1) {
                output+=required+"\n";

                if (required.find("USERID_HINT",0,false)!=-1)
		updateIDs(required);

                if ((step>2) && (required.find("keyedit.prompt")!=-1)) {			
			if (step==3)
			{
			emit passwordChanged();
                        p->writeStdin("save");
			}
			else p->writeStdin("quit");
                        required=QString::null;
                }

                if ((required.find("GOOD_PASSPHRASE")!=-1) && (step==2))
                        step=3;

                if ((required.find("BAD_PASSPHRASE")!=-1) && (step==2)) {
                        step=1;
                        message=i18n("<b>Bad passphrase</b>. Try again<br>");
                }

                if ((required.find("passphrase.enter")!=-1)) {
                        if (userIDs.isEmpty())
                                userIDs=i18n("[No user id found]");
                        userIDs.replace(QRegExp("<"),"&lt;");

                        if (step==1) {
                                QCString passphrase;
                                int code=KPasswordDialog::getPassword(passphrase,i18n("<qt>%1 Enter passphrase for <b>%2</b></qt>").arg(message).arg(checkForUtf8(userIDs)));
                                if (code!=QDialog::Accepted) {
                                        p->writeStdin("quit");
                                        //				 p->closeWhenDone();
                                        emit processaborted(true);
                                        delete p;
                                        return;
                                }
                                p->writeStdin(passphrase,true);
                                step=2;
                        }

                        if (step==3) {
                                QCString passphrase;
                                int code=KPasswordDialog::getNewPassword(passphrase,i18n("<qt>Enter new passphrase for <b>%1</b><br>If you forget this passphrase, all your encrypted files and messages will be lost !<br></qt>").arg(userIDs));
                                if (code!=QDialog::Accepted) {
					step=4;
                                        p->writeStdin("quit");
                                        p->writeStdin("quit");
                                        p->closeWhenDone();
                                        emit processaborted(true);
                                        return;
                                }
                                p->writeStdin(passphrase,true);
                                userIDs=QString::null;
                        }

                        required=QString::null;
                }


                if (required.find("GET_")!=-1) /////// gpg asks for something unusal, turn to konsole mode
                {
                        p->writeStdin("quit");
                        p->closeWhenDone();

                }
        }
}



void KgpgInterface::passover(KProcess *)
{
        //emit trustfinished();
}



//////////////////////////////////////////////////////////////    key export

QString KgpgInterface::getKey(QStringList IDs, bool attributes)
{
        keyString=QString::null;
        KProcIO *proc=new KProcIO();
        *proc<< "gpg"<<"--no-tty"<<"--no-secmem-warning";
        *proc<<"--export"<<"--armor";
        if (!attributes)
                *proc<<"--export-options"<<"no-include-attributes";

        for ( QStringList::Iterator it = IDs.begin(); it != IDs.end(); ++it )
                *proc << *it;
        QObject::connect(proc, SIGNAL(readReady(KProcIO *)),this, SLOT(slotReadKey(KProcIO *)));
        proc->start(KProcess::Block,false);
        return keyString;
}


void KgpgInterface::slotReadKey(KProcIO *p)
{
        QString outp;
        while (p->readln(outp)!=-1)
                if (!outp.startsWith("gpg:")) keyString+=outp+"\n";
}


//////////////////////////////////////////////////////////////    key import

void KgpgInterface::importKeyURL(KURL url)
{
        /////////////      import a key

        if( KIO::NetAccess::download( url, tempKeyFile ) ) {
                message=QString::null;
                KProcIO *conprocess=new KProcIO();
                *conprocess<< "gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--status-fd=2"<<"--import";
                *conprocess<<"--allow-secret-key-import";
                *conprocess<<tempKeyFile;
                QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(importURLover(KProcess *)));
                QObject::connect(conprocess, SIGNAL(readReady(KProcIO *)),this, SLOT(importprocess(KProcIO *)));
                conprocess->start(KProcess::NotifyOnExit,true);
        }
}

void KgpgInterface::importKey(QString keystr)
{
        /////////////      import a key
        message=QString::null;
        KProcIO *conprocess=new KProcIO();
        *conprocess<< "gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--status-fd=2"<<"--import";
        *conprocess<<"--allow-secret-key-import";
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(importover(KProcess *)));
        QObject::connect(conprocess, SIGNAL(readReady(KProcIO *)),this, SLOT(importprocess(KProcIO *)));
        conprocess->start(KProcess::NotifyOnExit,true);
        conprocess->writeStdin(keystr, true);
        conprocess->closeWhenDone();
}

void KgpgInterface::importover(KProcess *)
{
QStringList importedKeysIds;
QString resultMessage;
bool secretImport=false;
kdDebug(2100)<<"Importing is over"<<endl;
        QString parsedOutput=message;
        QStringList importedKeys;

        while (parsedOutput.find("IMPORTED")!=-1) {
                parsedOutput.remove(0,parsedOutput.find("IMPORTED")+8);
		importedKeys<<parsedOutput.section("\n",0,0).stripWhiteSpace();
		importedKeysIds<<parsedOutput.stripWhiteSpace().section(' ',0,0);
        }

        if (message.find("IMPORT_RES")!=-1) {
                parsedOutput=message.section("IMPORT_RES",-1,-1).stripWhiteSpace();
		QStringList messageList=QStringList::split(" ",parsedOutput,true);
                	
                resultMessage=i18n("<qt>%1 key(s) processed.<br></qt>").arg(messageList[0]);
                if (messageList[4]!="0")
                        resultMessage+=i18n("<qt>%1 key(s) unchanged.<br></qt>").arg(messageList[4]);
                if (messageList[7]!="0")
                        resultMessage+=i18n("<qt>%1 signature(s) imported.<br></qt>").arg(messageList[7]);
                if (messageList[1]!="0")
                        resultMessage+=i18n("<qt>%1 key(s) without ID.<br></qt>").arg(messageList[1]);
                if (messageList[3]!="0")
                        resultMessage+=i18n("<qt>%1 RSA key(s) imported.<br></qt>").arg(messageList[3]);
                if (messageList[5]!="0")
                        resultMessage+=i18n("<qt>%1 user ID(s) imported.<br></qt>").arg(messageList[5]);
                if (messageList[6]!="0")
                        resultMessage+=i18n("<qt>%1 subkey(s) imported.<br></qt>").arg(messageList[6]);
                if (messageList[8]!="0")
                        resultMessage+=i18n("<qt>%1 revocation certificate(s) imported.<br></qt>").arg(messageList[8]);
                if (messageList[9]!="0")
			{
                        resultMessage+=i18n("<qt>%1 secret key(s) processed.<br></qt>").arg(messageList[9]);
			secretImport=true;
			}
                if (messageList[10]!="0")
                        resultMessage+=i18n("<qt><b>%1 secret key(s) imported.</b><br></qt>").arg(messageList[10]);
                if (messageList[11]!="0")
                        resultMessage+=i18n("<qt>%1 secret key(s) unchanged.<br></qt>").arg(messageList[11]);
                if (messageList[12]!="0")
                        resultMessage+=i18n("<qt>%1 secret key(s) not imported.<br></qt>").arg(messageList[12]);
                if (messageList[2]!="0")
                        resultMessage+=i18n("<qt><b>%1 key(s) imported:</b><br></qt>").arg(messageList[2]);
			
		if (secretImport) resultMessage+=i18n("<qt><br><b>You have imported a secret key.</b> <br>"
									"Please note that imported secret keys are not trusted by default.<br>"
									"To fully use this secret key for signing and encryption, you must edit the key (double click on it) and set its trust to Full or Ultimate.</qt>");
        } else
                resultMessage=i18n("No key imported... \nCheck detailed log for more infos");
        KDetailedInfo *m_box=new KDetailedInfo(0,"import_result",resultMessage,message,importedKeys);
        m_box->setMinimumWidth(300);
        m_box->exec();
        //KMessageBox::information(0,message);
        emit importfinished(importedKeysIds);
}

void KgpgInterface::importURLover(KProcess *p)
{
        KIO::NetAccess::removeTempFile(tempKeyFile);
        importover(p);
        //KMessageBox::information(0,message);
        //emit importfinished();
}

void KgpgInterface::importprocess(KProcIO *p)
{
        QString outp;
        while (p->readln(outp)!=-1) {
                if (outp.find("http-proxy")==-1)
                        message+=outp+"\n";
        }
}


///////////////////////////////////////////////////////////////////////////////////////   User ID's


void KgpgInterface::KgpgAddUid(QString keyID,QString name,QString email,QString comment)
{
uidName=name;
uidComment=comment;
uidEmail=email;
output=QString::null;
addSuccess=true;

        KProcIO *conprocess=new KProcIO();
        *conprocess<< "gpg"<<"--no-tty"<<"--status-fd=2"<<"--command-fd=0";
        *conprocess<<"--edit-key"<<keyID<<"adduid";
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(adduidover(KProcess *)));
        QObject::connect(conprocess, SIGNAL(readReady(KProcIO *)),this, SLOT(adduidprocess(KProcIO *)));
        conprocess->start(KProcess::NotifyOnExit,true);
}

void KgpgInterface::adduidover(KProcess *)
{
if (addSuccess) emit addUidFinished();
else emit addUidError(output);
}

void KgpgInterface::adduidprocess(KProcIO *p)
{
        QString required=QString::null;
        while (p->readln(required,true)!=-1) {
                output+=required+"\n";
                if (required.find("USERID_HINT",0,false)!=-1) 
		updateIDs(required);

                if (required.find("keygen.name")!=-1)  {
                        p->writeStdin(uidName);
                        required=QString::null;
                }
		
		if (required.find("keygen.email")!=-1)  {
                        p->writeStdin(uidEmail);
                        required=QString::null;
                }
		
		if (required.find("keygen.comment")!=-1)  {
                        p->writeStdin(uidComment);
                        required=QString::null;
                }

                if (required.find("passphrase.enter")!=-1) {
                        QCString delpass;
                        int code=KPasswordDialog::getPassword(delpass,i18n("<qt>Enter passphrase for <b>%1</b>:</qt>").arg(checkForUtf8(userIDs)));
                        if (code!=QDialog::Accepted) {
                                //deleteSuccess=false;
                                p->writeStdin("quit");
                                p->closeWhenDone();
                                return;
                        }
                        p->writeStdin(delpass,true);
                        required=QString::null;

                }

		if (required.find("keyedit.prompt")!=-1) {
		       p->writeStdin("save");
                        required=QString::null;
		}

		if ((required.find("GET_")!=-1)) /////// gpg asks for something unusal, turn to konsole mode
                {
                        kdDebug(2100)<<"unknown request"<<endl;
                        addSuccess=false;  /////  switching to console mode
                        p->writeStdin("quit");
                        p->closeWhenDone();

                }
        }
}









/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  photo id's

void KgpgInterface::KgpgGetPhotoList(QString keyID)
{
photoList.clear();
output=QString::null;
photoCount=1;
userIDs=keyID;

        KProcIO *conprocess=new KProcIO();
        *conprocess<< "gpg"<<"--no-tty"<<"--status-fd=2"<<"--command-fd=0";
        *conprocess<<"--with-colon"<<"--list-keys"<<keyID;
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(photoreadover(KProcess *)));
        QObject::connect(conprocess, SIGNAL(readReady(KProcIO *)),this, SLOT(photoreadprocess(KProcIO *)));
        conprocess->start(KProcess::NotifyOnExit,true);
}

void KgpgInterface::photoreadprocess(KProcIO *p)
{
        QString required=QString::null;
        while (p->readln(required,true)!=-1) {
                output+=required+"\n";
                if (required.startsWith("uat") || required.startsWith("uid")) photoCount++;
}
}


void KgpgInterface::photoreadover(KProcess *)
{
for (int i=1;i<photoCount+1;i++)
if (isPhotoId(i)) photoList+=QString::number(i);

emit signalPhotoList(photoList);
}

bool KgpgInterface::isPhotoId(int uid)
{
KTempFile *kgpginfotmp=new KTempFile();
                kgpginfotmp->setAutoDelete(true);
                QString pgpgOutput="cp %i "+kgpginfotmp->name();

KProcIO *conprocess=new KProcIO();
        *conprocess<< "gpg"<<"--no-tty"<<"--status-fd=2"<<"--command-fd=0";
        *conprocess<<"--photo-viewer"<<QFile::encodeName(pgpgOutput)<<"--edit-key"<<userIDs<<"uid"<<QString::number(uid)<<"showphoto";
        conprocess->start(KProcess::Block);
	if (kgpginfotmp->file()->size()>0)
	{
	kgpginfotmp->unlink();
	return true;
	}
	kgpginfotmp->unlink();
	return false;
}

void KgpgInterface::KgpgDeletePhoto(QString keyID,QString uid)
{
	delSuccess=true;
	output=QString::null;
        KProcIO *conprocess=new KProcIO();
        *conprocess<< "gpg"<<"--no-tty"<<"--status-fd=2"<<"--command-fd=0";
        *conprocess<<"--edit-key"<<keyID<<"uid"<<uid<<"deluid";
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(delphotoover(KProcess *)));
        QObject::connect(conprocess, SIGNAL(readReady(KProcIO *)),this, SLOT(delphotoprocess(KProcIO *)));
        conprocess->start(KProcess::NotifyOnExit,true);
}

void KgpgInterface::delphotoover(KProcess *)
{
if (delSuccess) emit delPhotoFinished();
else emit delPhotoError(output);
}

void KgpgInterface::delphotoprocess(KProcIO *p)
{
        QString required=QString::null;
        while (p->readln(required,true)!=-1) {
                output+=required+"\n";
                if (required.find("USERID_HINT",0,false)!=-1)
                                updateIDs(required);

                if (required.find("keyedit.remove.uid.okay")!=-1)  {
                        p->writeStdin("YES");
                        required=QString::null;
                }

                if (required.find("passphrase.enter")!=-1) {
                        QCString delpass;
                        int code=KPasswordDialog::getPassword(delpass,i18n("<qt>Enter passphrase for <b>%1</b>:</qt>").arg(checkForUtf8(userIDs)));
                        if (code!=QDialog::Accepted) {
                                //deleteSuccess=false;
                                p->writeStdin("quit");
                                p->closeWhenDone();
                                return;
                        }
                        p->writeStdin(delpass,true);
                        required=QString::null;

                }

		if (required.find("keyedit.prompt")!=-1) {
		       p->writeStdin("save");
                        required=QString::null;
		}

		if ((required.find("GET_")!=-1)) /////// gpg asks for something unusal, turn to konsole mode
                {
                        kdDebug(2100)<<"unknown request"<<endl;
                        delSuccess=false;
                        p->writeStdin("quit");
                        p->closeWhenDone();

                }
        }
}


void KgpgInterface::KgpgAddPhoto(QString keyID,QString imagePath)
{
photoUrl=imagePath;
output=QString::null;
addSuccess=true;
        KProcIO *conprocess=new KProcIO();
        *conprocess<< "gpg"<<"--no-tty"<<"--status-fd=2"<<"--command-fd=0";
        *conprocess<<"--edit-key"<<keyID<<"addphoto";
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(addphotoover(KProcess *)));
        QObject::connect(conprocess, SIGNAL(readReady(KProcIO *)),this, SLOT(addphotoprocess(KProcIO *)));
        conprocess->start(KProcess::NotifyOnExit,true);
}

void KgpgInterface::addphotoover(KProcess *)
{
if (addSuccess) emit addPhotoFinished();
else emit addPhotoError(output);
}

void KgpgInterface::addphotoprocess(KProcIO *p)
{
        QString required=QString::null;
        while (p->readln(required,true)!=-1) {
                output+=required+"\n";
                if (required.find("USERID_HINT",0,false)!=-1)
		updateIDs(required);
		
                if (required.find("photoid.jpeg.add")!=-1)  {
                        p->writeStdin(photoUrl);
                        required=QString::null;
                }
		
		if (required.find("photoid.jpeg.size")!=-1)  {
			if (KMessageBox::questionYesNo(0,i18n("This image is very large. Use it anyway?"))==KMessageBox::Yes)
                        p->writeStdin("Yes");
			else 
			{
			p->writeStdin("No");
			p->writeStdin("");
			p->writeStdin("quit");
			}
                        required=QString::null;
                }

                if (required.find("passphrase.enter")!=-1) {
                        QCString delpass;
                        int code=KPasswordDialog::getPassword(delpass,i18n("<qt>Enter passphrase for <b>%1</b>:</qt>").arg(checkForUtf8(userIDs)));
                        if (code!=QDialog::Accepted) {
                                //deleteSuccess=false;
                                p->writeStdin("quit");
                                p->closeWhenDone();
                                return;
                        }
                        p->writeStdin(delpass,true);
                        required=QString::null;

                }

		if (required.find("keyedit.prompt")!=-1) {
		       p->writeStdin("save");
                        required=QString::null;
		}

		if ((required.find("GET_")!=-1)) /////// gpg asks for something unusal, turn to konsole mode
                {
                        kdDebug(2100)<<"unknown request"<<endl;
                        p->writeStdin("quit");
			addSuccess=false;
                        p->closeWhenDone();

                }
        }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  key revocation

void KgpgInterface::KgpgRevokeKey(QString keyID,QString revokeUrl,int reason,QString description)
{
        revokeReason=reason;
        revokeSuccess=false;
        revokeDescription=description;
        certificateUrl=revokeUrl;
        output=QString::null;
        KProcIO *conprocess=new KProcIO();
        *conprocess<< "gpg"<<"--no-tty"<<"--status-fd=2"<<"--logger-fd=2"<<"--command-fd=0";
        if (!revokeUrl.isEmpty())
                *conprocess<<"-o"<<revokeUrl;
        *conprocess<<"--gen-revoke"<<keyID;
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(revokeover(KProcess *)));
        QObject::connect(conprocess, SIGNAL(readReady(KProcIO *)),this, SLOT(revokeprocess(KProcIO *)));
        conprocess->start(KProcess::NotifyOnExit,true);
}

void KgpgInterface::revokeover(KProcess *)
{
        if (!revokeSuccess)
                KMessageBox::detailedSorry(0,i18n("Creation of the revocation certificate failed..."),output);
        else {
                output=output.section("-----BEGIN",1);
                output.prepend("-----BEGIN");
                output=output.section("BLOCK-----",0);
                emit revokecertificate(output);
                if (!certificateUrl.isEmpty())
                        emit revokeurl(certificateUrl);
        }
}

void KgpgInterface::revokeprocess(KProcIO *p)
{
        QString required=QString::null;
        while (p->readln(required,true)!=-1) {
                output+=required+"\n";

                if (required.find("USERID_HINT",0,false)!=-1)
		updateIDs(required);
		
                if ((required.find("GOOD_PASSPHRASE")!=-1))
                        revokeSuccess=true;

                if ((required.find("gen_revoke.okay")!=-1) || (required.find("ask_revocation_reason.okay")!=-1) || (required.find("openfile.overwrite.okay")!=-1)) {
                        p->writeStdin("YES");
                        required=QString::null;
                }

                if (required.find("ask_revocation_reason.code")!=-1) {
                        p->writeStdin(QString::number(revokeReason));
                        required=QString::null;
                }

                if (required.find("passphrase.enter")!=-1) {
                        QCString signpass;
                        int code=KPasswordDialog::getPassword(signpass,i18n("<qt>Enter passphrase for <b>%1</b>:</qt>").arg(checkForUtf8(userIDs)));
                        if (code!=QDialog::Accepted) {
                                expSuccess=3;  /////  aborted by user mode
                                p->writeStdin("quit");
                                p->closeWhenDone();
                                return;
                        }
                        p->writeStdin(signpass,true);
                        required=QString::null;

                }
                if (required.find("ask_revocation_reason.text")!=-1) {
                        //		kdDebug(2100)<<"description"<<endl;
                        p->writeStdin(revokeDescription);
                        revokeDescription=QString::null;
                        required=QString::null;
                }
                if ((required.find("GET_")!=-1)) /////// gpg asks for something unusal, turn to konsole mode
                {
                        kdDebug(2100)<<"unknown request"<<endl;
                        expSuccess=1;  /////  switching to console mode
                        p->writeStdin("quit");
                        p->closeWhenDone();

                }
        }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////   parsing of ./gnupg/options file

QString KgpgInterface::getGpgSetting(QString name,QString configFile)
{
        name=name.stripWhiteSpace()+" ";
        QFile qfile(QFile::encodeName(configFile));
        if (qfile.open(IO_ReadOnly) && (qfile.exists())) {
                QString result;
                QTextStream t( &qfile );
                result=t.readLine();
                while (result!=NULL) {
                        if (result.stripWhiteSpace().startsWith(name)) {
                                result=result.stripWhiteSpace();
                                result.remove(0,name.length());
                                result=result.stripWhiteSpace();
                                return result.section(" ",0,0);
                        }
                        result=t.readLine();
                }
                qfile.close();
        }
        return QString::null;
}

void KgpgInterface::delGpgGroup(QString name, QString configFile)
{
        QString textToWrite;
        QFile qfile(QFile::encodeName(configFile));
        if (qfile.open(IO_ReadOnly) && (qfile.exists())) {
                QString result;
                QTextStream t( &qfile );
                result=t.readLine();
                while (result!=NULL) {
                        if (result.stripWhiteSpace().startsWith("group ")) {
                                QString result2=result.stripWhiteSpace();
                                result2.remove(0,6);
                                result2=result2.stripWhiteSpace();
                                if (result2.startsWith(name) && (result2.remove(0,name.length()).stripWhiteSpace().startsWith("=")))
                                        result=QString::null;
                        }
                        textToWrite+=result+"\n";
                        result=t.readLine();
                }
                qfile.close();
                if (qfile.open(IO_WriteOnly)) {
                        QTextStream t( &qfile);
                        t << textToWrite;
                        qfile.close();
                }
        }
}

void KgpgInterface::setGpgGroupSetting(QString name,QStringList values, QString configFile)
{
        QString textToWrite;
        bool found=false;
        QFile qfile(QFile::encodeName(configFile));
        kdDebug(2100)<<"Changing group: "<<name<<endl;
        if (qfile.open(IO_ReadOnly) && (qfile.exists())) {
                QString result;
                QTextStream t( &qfile );
                result=t.readLine();
                while (result!=NULL) {
                        if (result.stripWhiteSpace().startsWith("group ")) {
                                QString result2=result.stripWhiteSpace();
                                result2.remove(0,6);
                                result2=result2.stripWhiteSpace();
                                if (result2.startsWith(name) && (result2.remove(0,name.length()).stripWhiteSpace().startsWith("="))) {
//                                        kdDebug(2100)<<"Found group: "<<name<<endl;
                                        //kdDebug(2100)<<"New values: "<<values<<endl;
                                        result=QString("group %1=%2").arg(name).arg(values.join(" "));
                                        found=true;
                                }
                        }
                        textToWrite+=result+"\n";
                        result=t.readLine();
                }
                qfile.close();
                if (!found)
                        textToWrite+="\n"+QString("group %1=%2").arg(name).arg(values.join(" "));

                if (qfile.open(IO_WriteOnly)) {
                        QTextStream t( &qfile);
                        t << textToWrite;
                        qfile.close();
                }
        }
}



QStringList KgpgInterface::getGpgGroupSetting(QString name,QString configFile)
{

        QFile qfile(QFile::encodeName(configFile));
        if (qfile.open(IO_ReadOnly) && (qfile.exists())) {
                QString result;
                QTextStream t( &qfile );
                result=t.readLine();
                while (result!=NULL) {
                        result=result.stripWhiteSpace();
                        if (result.startsWith("group ")) {
                                kdDebug(2100)<<"Found 1 group"<<endl;
                                result.remove(0,6);
                                if (result.stripWhiteSpace().startsWith(name)) {
                                        kdDebug(2100)<<"Found group: "<<name<<endl;
                                        result=result.section('=',1);
                                        result=result.section('#',0,0);
                                        return QStringList::split (" ",result);
                                }
                        }
                        result=t.readLine();
                }
                qfile.close();
        }
        return QString::null;
}

QStringList KgpgInterface::getGpgGroupNames(QString configFile)
{
        QStringList groups;
        QFile qfile(QFile::encodeName(configFile));
        if (qfile.open(IO_ReadOnly) && (qfile.exists())) {
                QString result;
                QTextStream t( &qfile );
                result=t.readLine();
                while (result!=NULL) {
                        result=result.stripWhiteSpace();
                        if (result.startsWith("group ")) {
                                result.remove(0,6);
                                groups<<result.section("=",0,0).stripWhiteSpace();
                        }
                        result=t.readLine();
                }
                qfile.close();
        }
        return groups;
}


bool KgpgInterface::getGpgBoolSetting(QString name,QString configFile)
{
        name=name;
        QFile qfile(QFile::encodeName(configFile));
        if (qfile.open(IO_ReadOnly) && (qfile.exists())) {
                QString result;
                QTextStream t( &qfile );
                result=t.readLine();
                while (result!=NULL) {
                        if (result.stripWhiteSpace().startsWith(name)) {
                                return true;
                        }
                        result=t.readLine();
                }
                qfile.close();
        }
        return false;
}

void KgpgInterface::setGpgSetting(QString name,QString value,QString url)
{
        name=name+" ";
        QString textToWrite;
        bool found=false;
        QFile qfile(QFile::encodeName(url));

        if (qfile.open(IO_ReadOnly) && (qfile.exists())) {
                QString result;
                QTextStream t( &qfile );
                result=t.readLine();
                while (result!=NULL) {
                        if (result.stripWhiteSpace().startsWith(name)) {
                                if (!value.isEmpty())
                                        result=name+" "+value;
                                else
                                        result=QString::null;
                                found=true;
                        }
                        textToWrite+=result+"\n";
                        result=t.readLine();
                }
                qfile.close();
                if ((!found) && (!value.isEmpty()))
                        textToWrite+="\n"+name+" "+value;

                if (qfile.open(IO_WriteOnly)) {
                        QTextStream t( &qfile);
                        t << textToWrite;
                        qfile.close();
                }
        }
}


void KgpgInterface::setGpgBoolSetting(QString name,bool enable,QString url)
{
        QString textToWrite;
        bool found=false;
        QFile qfile(QFile::encodeName(url));

        if (qfile.open(IO_ReadOnly) && (qfile.exists())) {
                QString result;
                QTextStream t( &qfile );
                result=t.readLine();
                while (result!=NULL) {
                        if (result.stripWhiteSpace().startsWith(name)) {
                                if (enable)
                                        result=name;
                                else
                                        result=QString::null;
                                found=true;
                        }
                        textToWrite+=result+"\n";
                        result=t.readLine();
                }
                qfile.close();
                if ((!found) && (enable))
                        textToWrite+=name;

                if (qfile.open(IO_WriteOnly)) {
                        QTextStream t( &qfile);
                        t << textToWrite;
                        qfile.close();
                }
        }
}

QString KgpgInterface::checkForUtf8(QString txt)
{
        //    code borrowed from gpa
        const char *s;

        /* Make sure the encoding is UTF-8.
         * Test structure suggested by Werner Koch */
        if (txt.isEmpty())
                return QString::null;

        for (s = txt.ascii(); *s && !(*s & 0x80); s++)
                ;
        if (*s && !strchr (txt.ascii(), 0xc3) && (txt.find("\\x")==-1))
                return txt;

        /* The string is not in UTF-8 */
        //if (strchr (txt.ascii(), 0xc3)) return (txt+" +++");
        if (txt.find("\\x")==-1)
                return QString::fromUtf8(txt.ascii());
        //        if (!strchr (txt.ascii(), 0xc3) || (txt.find("\\x")!=-1)) {
        for ( int idx = 0 ; (idx = txt.find( "\\x", idx )) >= 0 ; ++idx ) {
                char str[2] = "x";
                str[0] = (char) QString( txt.mid( idx + 2, 2 ) ).toShort( 0, 16 );
                txt.replace( idx, 4, str );
        }
        if (!strchr (txt.ascii(), 0xc3))
                return QString::fromUtf8(txt.ascii());  // perform Utf8 twice, or some keys display badly
        else
                return QString::fromUtf8(QString::fromUtf8(txt.ascii()).ascii());  // perform Utf8 twice, or some keys display badly
}


#include "kgpginterface.moc"
