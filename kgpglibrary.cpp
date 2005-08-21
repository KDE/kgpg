/***************************************************************************
                          kgpglibrary.cpp  -  description
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

#include <qhbox.h>
#include <qvbox.h>

#include <klocale.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <krun.h>
#include <qfile.h>
#include <kpassivepopup.h>
#include <kiconloader.h>
#include "kgpglibrary.h"
#include "popuppublic.h"
#include "kgpginterface.h"
#include <kio/renamedlg.h>

KgpgLibrary::KgpgLibrary(QWidget *parent, bool pgpExtension)
{
        if (pgpExtension)
                extension=".pgp";
        else
                extension=".gpg";
	popIsActive=false;
	panel=parent;
}

KgpgLibrary::~KgpgLibrary()
{}


void KgpgLibrary::slotFileEnc(KURL::List urls,QStringList opts,QStringList defaultKey,KShortcut goDefaultKey)
{
        /////////////////////////////////////////////////////////////////////////  encode file file
        if (!urls.empty()) {
                urlselecteds=urls;
                if (defaultKey.isEmpty()) {
			QString fileNames=urls.first().fileName();
			if (urls.count()>1) fileNames+=",...";
                        popupPublic *dialogue=new popupPublic(0,"Public keys",fileNames,true,goDefaultKey);
                        connect(dialogue,SIGNAL(selectedKey(QStringList,QStringList,bool,bool)),this,SLOT(startencode(QStringList,QStringList,bool,bool)));
                        dialogue->exec();
                        delete dialogue;
                } else
                        startencode(defaultKey,opts,false,false);
        }
}

void KgpgLibrary::startencode(QStringList encryptKeys,QStringList encryptOptions,bool shred,bool symetric)
{
	popIsActive=false;
        //KURL::List::iterator it;
	//filesToEncode=urlselecteds.count();
	_encryptKeys=encryptKeys;
	_encryptOptions=encryptOptions;
	_shred=shred;
	_symetric=symetric;
		fastencode(urlselecteds.first(),encryptKeys,encryptOptions,symetric);
}


void KgpgLibrary::fastencode(KURL &fileToCrypt,QStringList selec,QStringList encryptOptions,bool symetric)
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
			KIO::RenameDlg *over=new KIO::RenameDlg(0,i18n("File Already Exists"),QString::null,dest.path(),KIO::M_OVERWRITE);
		    	if (over->exec()==QDialog::Rejected)
	    		{
                	delete over;
			emit systemMessage(QString::null,true);
                	return;
            		}
	    		dest=over->newDestURL();
	    		delete over;
        }
	int filesToEncode=urlselecteds.count();
	if (filesToEncode>1)
	emit systemMessage(i18n("<b>%1 Files left.</b>\nEncrypting </b>%2").arg(filesToEncode).arg(urlselecteds.first().path()));
	else emit systemMessage(i18n("<b>Encrypting </b>%2").arg(urlselecteds.first().path()));
        KgpgInterface *cryptFileProcess=new KgpgInterface();
	pop = new KPassivePopup(panel);
        cryptFileProcess->KgpgEncryptFile(selec,urlselected,dest,encryptOptions,symetric);
         if (!popIsActive) 
	{
	//connect(cryptFileProcess,SIGNAL(processstarted(QString)),this,SLOT(processpopup2(QString)));
	popIsActive=true;	
	}
	connect(cryptFileProcess,SIGNAL(encryptionfinished(KURL)),this,SLOT(processenc(KURL)));
        connect(cryptFileProcess,SIGNAL(errormessage(QString)),this,SLOT(processencerror(QString)));
}

void KgpgLibrary::processpopup2(QString fileName)
{
        
	//pop->setTimeout(0);
        pop->setView(i18n("Processing encryption (%1)").arg(fileName),i18n("Please wait..."),KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop));
        pop->show();
        /*QRect qRect(QApplication::desktop()->screenGeometry());
        int iXpos=qRect.width()/2-pop->width()/2;
        int iYpos=qRect.height()/2-pop->height()/2;
        pop->move(iXpos,iYpos);*/

}

void KgpgLibrary::shredpreprocessenc(KURL fileToShred)
{
	popIsActive=false;
	emit systemMessage(QString::null);
	shredprocessenc(fileToShred);
}

void KgpgLibrary::shredprocessenc(KURL::List filesToShred)
{
emit systemMessage(i18n("Shredding %n file","Shredding %n files",filesToShred.count()));

KIO::Job *job;
job = KIO::del( filesToShred, true );
connect( job, SIGNAL( result( KIO::Job * ) ),SLOT( slotShredResult( KIO::Job * ) ) );	
}

void KgpgLibrary::slotShredResult( KIO::Job * job )
{
    emit systemMessage(QString::null);
    if (job && job->error())
    {
    job->showErrorDialog( (QWidget*)parent() );
    emit systemMessage(QString::null,true);
    KPassivePopup::message(i18n("KGpg Error"),i18n("Process halted, not all files were shredded."),KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop),panel,"kgpg_error",0);
    }
}


void KgpgLibrary::processenc(KURL)
{
	emit systemMessage(QString::null);
	if (_shred) shredprocessenc(urlselecteds.first());
	urlselecteds.pop_front ();
	if (urlselecteds.count()>0)
	fastencode(urlselecteds.first(),_encryptKeys,_encryptOptions,_symetric);
}

void KgpgLibrary::processencerror(QString mssge)
{
	popIsActive=false;
	emit systemMessage(QString::null,true);
	KMessageBox::detailedSorry(panel,i18n("<b>Process halted</b>.<br>Not all files were encrypted."),mssge);
}



void KgpgLibrary::slotFileDec(KURL srcUrl,KURL destUrl,QStringList customDecryptOption)
{
        //////////////////////////////////////////////////////////////////    decode file from konqueror or menu
        KgpgInterface *decryptFileProcess=new KgpgInterface();
        pop = new KPassivePopup();
	urlselected=srcUrl;
        decryptFileProcess->KgpgDecryptFile(srcUrl,destUrl,customDecryptOption);
        connect(decryptFileProcess,SIGNAL(processaborted(bool)),this,SLOT(processdecover()));
        connect(decryptFileProcess,SIGNAL(processstarted(QString)),this,SLOT(processpopup(QString)));
        connect(decryptFileProcess,SIGNAL(decryptionfinished()),this,SLOT(processdecover()));
        connect(decryptFileProcess,SIGNAL(errormessage(QString)),this,SLOT(processdecerror(QString)));
}

void KgpgLibrary::processpopup(QString fileName)
{
	emit systemMessage(i18n("Decrypting %1").arg(fileName));
	pop->setTimeout(0);
        pop->setView(i18n("Processing decryption"),i18n("Please wait..."),KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop));
        pop->show();
        QRect qRect(QApplication::desktop()->screenGeometry());
        int iXpos=qRect.width()/2-pop->width()/2;
        int iYpos=qRect.height()/2-pop->height()/2;
        pop->move(iXpos,iYpos);
}

void KgpgLibrary::processdecover()
{
	emit systemMessage(QString::null);
	delete pop;
        emit decryptionOver();
}


void KgpgLibrary::processdecerror(QString mssge)
{
	delete pop;
	emit systemMessage(QString::null);
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
