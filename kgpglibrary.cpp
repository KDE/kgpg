/***************************************************************************
 *   Copyright (C) 2003 by bjUTHOR$                                        *
 *   bj@altern.org                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

 #include "kgpglibrary.h"

 KgpgLibrary::KgpgLibrary()
{}

KgpgLibrary::~KgpgLibrary()
{}


 //void KgpgLibrary::slotFileEnc(KURL url,QString opts,QString defaultKey)
 void KgpgLibrary::slotFileEnc(KURL::List urls,QString opts,QString defaultKey)
{
  /////////////////////////////////////////////////////////////////////////  encode file file
 if (!urls.empty())
    {
      urlselecteds=urls;
      if (defaultKey.isEmpty())
        {
          popupPublic *dialogue=new popupPublic(0,"Public keys","files",true);
          connect(dialogue,SIGNAL(selectedKey(QString &,QString,bool,bool)),this,SLOT(startencode(QString &,QString,bool,bool)));
          dialogue->exec();
          delete dialogue;
        }
      else startencode(defaultKey,opts,false,false);
    }
}

void KgpgLibrary::startencode(QString &selec,QString encryptOptions,bool shred,bool symetric)
{
KURL::List::iterator it;
for ( it = urlselecteds.begin(); it != urlselecteds.end(); ++it )
fastencode(*it,selec,encryptOptions,shred,symetric);

}


void KgpgLibrary::fastencode(KURL &fileToCrypt,QString &selec,QString encryptOptions,bool shred,bool symetric)
{
  //////////////////              encode from file
  if ((selec==NULL) && (!symetric))
    {
      KMessageBox::sorry(0,i18n("You have not chosen an encryption key."));
      return;
    }
urlselected=fileToCrypt;
  KURL dest;

  if (encryptOptions.find("--armor")!=-1) dest.setPath(urlselected.path()+".asc");
  else dest.setPath(urlselected.path()+".gpg");

  QFile fgpg(dest.path());

  if (fgpg.exists())
    {
      KgpgOverwrite *over=new KgpgOverwrite(0,"overwrite",dest);
      if (over->exec()==QDialog::Accepted)
        dest.setFileName(over->getfname());
      else
        return;
    }

  KgpgInterface *cryptFileProcess=new KgpgInterface();
  cryptFileProcess->KgpgEncryptFile(selec,urlselected,dest,encryptOptions,symetric);

  connect(cryptFileProcess,SIGNAL(processaborted(bool)),this,SLOT(processenc(bool)));
  connect(cryptFileProcess,SIGNAL(processstarted()),this,SLOT(processpopup2()));
  if (shred)
    connect(cryptFileProcess,SIGNAL(encryptionfinished()),this,SLOT(shredprocessenc()));
  else
    connect(cryptFileProcess,SIGNAL(encryptionfinished()),this,SLOT(processenc()));
	connect(cryptFileProcess,SIGNAL(errormessage(QString)),this,SLOT(processencerror(QString)));
}

void KgpgLibrary::shredprocessenc()
{
#if (KDE_VERSION >= 310)
delete pop;
#else
delete clippop;
#endif
      KShred *shredres=new KShred(urlselected.path());
      if (shredres->shred()==false) KMessageBox::sorry(0,i18n("The source file could not be shredded.\nCheck your permissions."));
    
}


void KgpgLibrary::processenc()
{
#if (KDE_VERSION >= 310)
delete pop;
#else
delete clippop;
#endif
}

void KgpgLibrary::processencerror(QString mssge)
{
#if (KDE_VERSION >= 310)
delete pop;
#else
delete clippop;
#endif
  KMessageBox::detailedSorry(0,i18n("Encryption failed."),mssge);
}



void KgpgLibrary::slotFileDec(KURL srcUrl,KURL destUrl,QString customDecryptOption)
{
  //////////////////////////////////////////////////////////////////    decode file from konqueror or menu
  KgpgInterface *decryptFileProcess=new KgpgInterface();
  urlselected=srcUrl;
  popIsDisplayed=false;
  decryptFileProcess->KgpgDecryptFile(srcUrl,destUrl,customDecryptOption);
  connect(decryptFileProcess,SIGNAL(processaborted(bool)),this,SLOT(processdecover()));
  connect(decryptFileProcess,SIGNAL(processstarted()),this,SLOT(processpopup()));
  connect(decryptFileProcess,SIGNAL(decryptionfinished()),this,SLOT(processdecover()));
  connect(decryptFileProcess,SIGNAL(errormessage(QString)),this,SLOT(processdecerror(QString)));
}

void KgpgLibrary::processpopup()
{
popIsDisplayed=true;
#if (KDE_VERSION >= 310)
pop = new KPassivePopup();
      pop->setView(i18n("Processing decryption"),i18n("Please wait..."),KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop));
	  	pop->show();	  
	  	QRect qRect(QApplication::desktop()->screenGeometry());
		int iXpos=qRect.width()/2-pop->width()/2;
		int iYpos=qRect.height()/2-pop->height()/2;
      	pop->move(iXpos,iYpos);
#else
	clippop = new QDialog( 0,0,false,WStyle_Customize | WStyle_NormalBorder);
              QVBoxLayout *vbox=new QVBoxLayout(clippop,3);
              QLabel *tex=new QLabel(clippop);
              tex->setText(i18n("<b>Processing decryption</b>"));
			  QLabel *tex2=new QLabel(clippop);
			  tex2->setText(i18n("Please wait..."));
              vbox->addWidget(tex);
			  vbox->addWidget(tex2);
              clippop->setMinimumWidth(250);
              clippop->adjustSize();
			  clippop->show();
#endif
}

void KgpgLibrary::processpopup2()
{
#if (KDE_VERSION >= 310)
 pop = new KPassivePopup();
      pop->setView(i18n("Processing encryption"),i18n("Please wait..."),KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop));
	  	pop->show();	  
	  	QRect qRect(QApplication::desktop()->screenGeometry());
		int iXpos=qRect.width()/2-pop->width()/2;
		int iYpos=qRect.height()/2-pop->height()/2;
      	pop->move(iXpos,iYpos);
#else
	clippop = new QDialog(0,0,false,WStyle_Customize | WStyle_NormalBorder);
              QVBoxLayout *vbox=new QVBoxLayout(clippop,3);
              QLabel *tex=new QLabel(clippop);
              tex->setText(i18n("<b>Processing encryption</b>"));
			  QLabel *tex2=new QLabel(clippop);
			  tex2->setText(i18n("Please wait..."));
              vbox->addWidget(tex);
			  vbox->addWidget(tex2);
              clippop->setMinimumWidth(250);
              clippop->adjustSize();
			  clippop->show();
#endif
}

void KgpgLibrary::processdecover()
{
if (popIsDisplayed)
{
#if (KDE_VERSION >= 310)
delete pop;
#else
delete clippop;
#endif
}
}

void KgpgLibrary::processdecerror(QString mssge)
{
if (popIsDisplayed)
{
#if (KDE_VERSION >= 310)
delete pop;
#else
delete clippop;
#endif
}
///// test if file is a public key
QFile qfile(QFile::encodeName(urlselected.path()));
if (qfile.open(IO_ReadOnly))
{
QTextStream t( &qfile );
QString result(t.read());
qfile.close();
      //////////////     if  pgp data found, decode it
      if (result.startsWith("-----BEGIN PGP PUBLIC KEY BLOCK"))
      {//////  dropped file is a public key, ask for import
     

        int result=KMessageBox::warningContinueCancel(0,i18n("<p>The file <b>%1</b> is a public key.<br>Do you want to import it ?</p>").arg(urlselected.path()),i18n("Warning"));
        if (result==KMessageBox::Cancel) return;
        else
        {
		KgpgInterface *importKeyProcess=new KgpgInterface();
  		importKeyProcess->importKeyURL(urlselected);
		
		return;
	} 
}
	else if (result.startsWith("-----BEGIN PGP PRIVATE KEY BLOCK"))
      {//////  dropped file is a public key, ask for import
        qfile.close();
		KMessageBox::information(0,i18n("<p>The file <b>%1</b> is a private key block. Please use KGpg key manager to import it.</p>").arg(urlselected.path()));
  return;
}
}
  KMessageBox::detailedSorry(0,i18n("Decryption failed."),mssge);
}



#include "kgpglibrary.moc"
