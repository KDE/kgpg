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

#include "kgpginterface.h"


#include <kmessagebox.h>
#include <kapplication.h>
#include <klocale.h>
#include <kpassdlg.h>
#include <qdialog.h>

KgpgInterface::KgpgInterface()
{}


void KgpgInterface::KgpgEncryptFile(QString userIDs,KURL srcUrl,KURL destUrl, QString Options, bool symetrical)
{
  file=destUrl;
  encError==false;
  KProcIO *proc=new KProcIO();
  userIDs=userIDs.stripWhiteSpace();
  Options=Options.stripWhiteSpace();
  if (symetrical==false)
    {
      *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning";
      if (Options!="") *proc<<Options;
      *proc<<"--output"<<destUrl.path().local8Bit()<<"-e";

      int ct=userIDs.find(" ");
      while (ct!=-1)  // if multiple keys...
        {
          *proc<<"--recipient"<<userIDs.section(' ',0,0);
          userIDs.remove(0,ct+1);
          ct=userIDs.find(" ");
        }
      *proc<<"--recipient"<<userIDs<<srcUrl.path().local8Bit();
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
      if (Options!="")
        *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<Options<<"--output"<<destUrl.path().local8Bit()<<"--passphrase-fd"<<QString::number(ppass[0])<<"-c"<<srcUrl.path().local8Bit();
      else
        *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--output"<<destUrl.path().local8Bit()<<"--passphrase-fd"<<QString::number(ppass[0])<<"-c"<<srcUrl.path().local8Bit();
    }

  /////////  when process ends, update dialog infos
  QObject::connect(proc, SIGNAL(processExited(KProcess *)),this,SLOT(encryptfin(KProcess *)));
  QObject::connect(proc,SIGNAL(receivedStderr(KProcess *, char *, int)),this,SLOT(encrypterror(KProcess *, char *, int)));
  proc->start(KProcess::NotifyOnExit,true);
  //encryptfin(proc);
}


KgpgInterface::~KgpgInterface()
{}


void KgpgInterface::encryptfin(KProcess *p)
{
  QFile qfile(file.path().local8Bit());
  if (!qfile.exists())
    {
      if (encError==false) emit encryptionfinished(false);
      encError=true;
    }
  else emit encryptionfinished(true);
}

void KgpgInterface::encrypterror(KProcess *p, char *buf, int buflen)
{
  if (encError==false)
    {
      encError=true;
      emit encryptionfinished(false);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

QString KgpgInterface::KgpgEncryptText(QString text,QString userIDs, QString Options)
{
  FILE *fp;
  QString encResult="",dests="",gpgcmd="";
  char buffer[200];
  
  userIDs=userIDs.stripWhiteSpace();
  Options=Options.stripWhiteSpace();


  int ct=userIDs.find(" ");
  while (ct!=-1)  // if multiple keys...
    {
      dests+=" --recipient "+userIDs.section(' ',0,0);
      userIDs.remove(0,ct+1);
      ct=userIDs.find(" ");
    }
  dests+=" --recipient "+userIDs;

  int i=0;
  while(i!=-1)
    {
      i=text.find("$",i,FALSE);
      if (i!=-1)
        {
          text.insert(i,"\\");
          i+=2;
        }
    }
  gpgcmd="echo \""+text+"\" | gpg --no-secmem-warning --no-tty "+Options+" -e "+dests;
  //////////   encode with untrusted keys or armor if checked by user
  fp = popen(gpgcmd, "r");
  while ( fgets( buffer, sizeof(buffer), fp))
    encResult+=buffer;
  pclose(fp);
  if (encResult!="") return encResult;
  else return "";
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QString KgpgInterface::KgpgDecryptText(QString text,QString userID)
{
  FILE *fp,*pass;
  QString encResult="",gpgcmd="";
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
  fp = popen(gpgcmd, "r");
  while ( fgets( buffer, sizeof(buffer), fp))
    encResult+=buffer;
  pclose(fp);
  }
  if (encResult!="") return encResult;
  else return "";
}





