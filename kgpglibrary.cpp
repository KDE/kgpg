/***************************************************************************
                          kgpglibrary.cpp  -  description
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

#include <qhbox.h>
#include <qvbox.h>

#include <klocale.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <krun.h>

#include "kgpglibrary.h"

KgpgLibrary::KgpgLibrary(bool pgpExtension)
{
        if (pgpExtension)
                extension=".pgp";
        else
                extension=".gpg";
}

KgpgLibrary::~KgpgLibrary()
{}


void KgpgLibrary::slotFileEnc(KURL::List urls,QStringList opts,QString defaultKey)
{
        /////////////////////////////////////////////////////////////////////////  encode file file
        if (!urls.empty()) {
                urlselecteds=urls;
                if (defaultKey.isEmpty()) {
			QString fileNames=urls.first().filename();
			if (urls.count()>1) fileNames+=",...";
                        popupPublic *dialogue=new popupPublic(0,"Public keys",fileNames,true);
                        connect(dialogue,SIGNAL(selectedKey(QStringList,QStringList,bool,bool)),this,SLOT(startencode(QStringList,QStringList,bool,bool)));
                        dialogue->exec();
                        delete dialogue;
                } else
                        startencode(defaultKey,opts,false,false);
        }
}

void KgpgLibrary::startencode(QStringList encryptKeys,QStringList encryptOptions,bool shred,bool symetric)
{
        KURL::List::iterator it;
        for ( it = urlselecteds.begin(); it != urlselecteds.end(); ++it )
                fastencode(*it,encryptKeys,encryptOptions,shred,symetric);
}


void KgpgLibrary::fastencode(KURL &fileToCrypt,QStringList selec,QStringList encryptOptions,bool shred,bool symetric)
{
        //////////////////              encode from file
        if ((selec.isEmpty()) && (!symetric)) {
                KMessageBox::sorry(0,i18n("You have not chosen an encryption key."));
                return;
        }
        urlselected=fileToCrypt;
        KURL dest;
        if (encryptOptions.find("--armor")!=encryptOptions.end())
                dest.setPath(urlselected.path()+".asc");
        else
                dest.setPath(urlselected.path()+extension);

        QFile fgpg(dest.path());

        if (fgpg.exists()) {
                KgpgOverwrite *over=new KgpgOverwrite(0,"overwrite",dest);
                if (over->exec()==QDialog::Accepted)
                        dest.setFileName(over->getfname());
                else
                        return;
        }

        KgpgInterface *cryptFileProcess=new KgpgInterface();
        cryptFileProcess->KgpgEncryptFile(selec,urlselected,dest,encryptOptions,symetric);
        connect(cryptFileProcess,SIGNAL(processstarted()),this,SLOT(processpopup2()));
        if (shred)
                connect(cryptFileProcess,SIGNAL(encryptionfinished(KURL)),this,SLOT(shredpreprocessenc(KURL)));
        else
                connect(cryptFileProcess,SIGNAL(encryptionfinished(KURL)),this,SLOT(processenc(KURL)));
        connect(cryptFileProcess,SIGNAL(errormessage(QString)),this,SLOT(processencerror(QString)));
}

void KgpgLibrary::shredpreprocessenc(KURL fileToShred)
{
	delete pop;
	pop=0L;		
shredprocessenc(fileToShred);
}

void KgpgLibrary::shredprocessenc(KURL fileToShred)
{
	if (!fileToShred.isLocalFile())
{
KMessageBox::sorry(0,i18n("<qt>File <b>%1</b> is a remote file.<br>You cannot shred it.</qt>").arg(fileToShred.path()));
return;
}
	KPassivePopup *shredPopup=new KPassivePopup();
 	shredPopup->setAutoDelete(false);
	QVBox *vbox=shredPopup->standardView(i18n("Shredding"),i18n("<qt>Shredding file <b>%1</b></qt>").arg(fileToShred.filename()),KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop));
	   
	   shredProgressBar= new KProgress(vbox);
    shredPopup->setView( vbox );
    shredPopup->show();
    kapp->processEvents();
        shredProgressBar->setTotalSteps(QFile(fileToShred.path()).size());

KShred *shredres=new KShred(fileToShred.path());
        connect(shredres,SIGNAL(processedSize(KIO::filesize_t)),this,SLOT(setShredProgress(KIO::filesize_t)));
        if (shredres->shred())
                delete shredPopup;
        else
                KMessageBox::sorry(0,i18n("<qt><b>ERROR</b> during file shredding.<br>File <b>%1</b> was not securely deleted. Please check your permissions.<qt>").arg(fileToShred.path()));

}


void KgpgLibrary::setShredProgress(KIO::filesize_t shredSize)
{
shredProgressBar->setProgress((int) shredSize);

}


void KgpgLibrary::processenc(KURL)
{
        delete pop;
	pop=0L;
}

void KgpgLibrary::processencerror(QString mssge)
{
        delete pop;
	pop=0L;
        KMessageBox::detailedSorry(0,i18n("Encryption failed."),mssge);
}



void KgpgLibrary::slotFileDec(KURL srcUrl,KURL destUrl,QStringList customDecryptOption)
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
        pop = new KPassivePopup();
        pop->setView(i18n("Processing decryption"),i18n("Please wait..."),KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop));
        pop->show();
        QRect qRect(QApplication::desktop()->screenGeometry());
        int iXpos=qRect.width()/2-pop->width()/2;
        int iYpos=qRect.height()/2-pop->height()/2;
        pop->move(iXpos,iYpos);
}

void KgpgLibrary::processpopup2()
{
        pop = new KPassivePopup();
        pop->setView(i18n("Processing encryption"),i18n("Please wait..."),KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop));
        pop->show();
        QRect qRect(QApplication::desktop()->screenGeometry());
        int iXpos=qRect.width()/2-pop->width()/2;
        int iYpos=qRect.height()/2-pop->height()/2;
        pop->move(iXpos,iYpos);

}

void KgpgLibrary::processdecover()
{
        //       if (popIsDisplayed)
        delete pop;
	pop=0L;
        emit decryptionOver();
}


void KgpgLibrary::processdecerror(QString mssge)
{
        ///// test if file is a public key
        QFile qfile(QFile::encodeName(urlselected.path()));
        if (qfile.open(IO_ReadOnly)) {
                QTextStream t( &qfile );
                QString result(t.read());
                qfile.close();
                //////////////     if  pgp data found, decode it
                if (result.startsWith("-----BEGIN PGP PUBLIC KEY BLOCK")) {//////  dropped file is a public key, ask for import
                        int result=KMessageBox::warningContinueCancel(0,i18n("<p>The file <b>%1</b> is a public key.<br>Do you want to import it ?</p>").arg(urlselected.path()),i18n("Warning"));
                        if (result==KMessageBox::Cancel)
                                return;
                        else {
                                KgpgInterface *importKeyProcess=new KgpgInterface();
                                importKeyProcess->importKeyURL(urlselected);
				connect(importKeyProcess,SIGNAL(importfinished(QStringList)),this,SIGNAL(importOver(QStringList)));
				
                                return;
                        }
                } else if (result.startsWith("-----BEGIN PGP PRIVATE KEY BLOCK")) {//////  dropped file is a public key, ask for import
                        qfile.close();
                        KMessageBox::information(0,i18n("<p>The file <b>%1</b> is a private key block. Please use KGpg key manager to import it.</p>").arg(urlselected.path()));
                        return;
                }
        }
        KMessageBox::detailedSorry(0,i18n("Decryption failed."),mssge);
}



#include "kgpglibrary.moc"
