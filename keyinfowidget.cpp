/***************************************************************************
                          keyinfowidget.cpp  -  description
                             -------------------
    begin                : Mon Nov 18 2002
    copyright            : (C) 2002 by y0k0
    email                : bj@altern.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your gpgOutpution) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

 #include "keyinfowidget.h"


KgpgKeyInfo::KgpgKeyInfo(QWidget *parent, const char *name,QString sigkey):KeyProperties( parent, name)
{
	QColor trustColor;
        QString fingervalue;
        FILE *pass;
        char line[200]="";
        QString gpgOutput,fullID;
        bool hasPhoto=false;
	bool isSecret=false;

	QString gpgcmd="gpg --no-tty --no-secmem-warning --with-colon --list-secret-key "+KShellProcess::quote(sigkey.local8Bit());

        pass=popen(QFile::encodeName(gpgcmd),"r");
        while ( fgets( line, sizeof(line), pass)) {
                gpgOutput=line;
                if (gpgOutput.startsWith("sec")) isSecret=true;
               }
	 pclose(pass);

        if (!isSecret) {
                changeExp->hide();//setEnabled(true);
                changePass->hide();//setEnabled(true);
		        }

	gpgcmd="gpg --no-tty --no-secmem-warning --with-colon --with-fingerprint --list-key "+KShellProcess::quote(sigkey.local8Bit());

        pass=popen(QFile::encodeName(gpgcmd),"r");
        while ( fgets( line, sizeof(line), pass)) {
                gpgOutput=line;
                if (gpgOutput.startsWith("uat"))
                        hasPhoto=true;
                if (gpgOutput.startsWith("pub")) {
                        QString algo=gpgOutput.section(':',3,3);
                        switch( algo.toInt() ) {
                        case  1:
                                algo="RSA";
                                break;
                        case 16:
                        case 20:
                                algo="ElGamal";
                                break;
                        case 17:
                                algo="DSA";
                                break;
                        default:
                                algo=QString("#" + algo);
                                break;
                        }
			tLAlgo->setText(algo);

                        const QString trust=gpgOutput.section(':',1,1);
                        QString tr;
                        switch( trust[0] ) {
                        case 'o':
                                tr= i18n("Unknown");
				trustColor.setRgb(255,255,255);
                                break;
                        case 'i':
                                tr= i18n("Invalid");
				trustColor.setRgb(172,0,0);
                                break;
                        case 'd':
                                tr=i18n("Disabled");
				trustColor.setRgb(172,0,0);
                                break;
                        case 'r':
                                tr=i18n("Revoked");
				trustColor.setRgb(172,0,0);
                                break;
                        case 'e':
                                tr=i18n("Expired");
				trustColor.setRgb(172,0,0);
                                break;
                        case 'q':
                                tr=i18n("Undefined");
				trustColor.setRgb(255,255,255);
                                break;
                        case 'n':
                                tr=i18n("None");
				trustColor.setRgb(255,255,255);
                                break;
                        case 'm':
                                tr=i18n("Marginal");
				trustColor.setRgb(172,0,0);
                                break;
                        case 'f':
                                tr=i18n("Full");
				trustColor.setRgb(148,255,0);
                                break;
                        case 'u':
                                tr=i18n("Ultimate");
				trustColor.setRgb(148,255,0);
                                break;
                        default:
                                tr="?";
				trustColor.setRgb(255,255,255);
                                break;
                        }
                        kLTrust->setText(tr);
                        kLTrust->setPaletteBackgroundColor(trustColor);

			fullID=gpgOutput.section(':',4,4);
                        displayedKeyID=fullID.right(8);
                        tLID->setText(fullID);

                        QString fullname=gpgOutput.section(':',9,9);

                        QDate date = QDate::fromString(gpgOutput.section(':',5,5), Qt::ISODate);
                        tLCreation->setText(KGlobal::locale()->formatDate(date));

			if (gpgOutput.section(':',6,6)=="") expirationDate=i18n("Unlimited");
			else
			{
			date = QDate::fromString(gpgOutput.section(':',6,6), Qt::ISODate);
			expirationDate=KGlobal::locale()->formatDate(date);
			}
                        tLExpiration->setText(expirationDate);

                        tLLength->setText(gpgOutput.section(':',2,2));

                        const QString otrust=gpgOutput.section(':',8,8);
                        switch( otrust[0] ) {
                        case 'f':
                                ownerTrust=i18n("Fully");
                                break;
                        case 'u':
                                ownerTrust=i18n("Ultimately");
                                break;
                        case 'm':
                                ownerTrust=i18n("Marginally");
                                break;
                        case 'n':
                                ownerTrust=i18n("Do NOT trust");
                                break;
                        default:
                                ownerTrust=i18n("Don't know");
                                break;
                        }
                        kCOwnerTrust->setCurrentItem(ownerTrust);

                        if (fullname.find("<")!=-1) {
                                QString kmail=fullname;
				if (fullname.find(")")!=-1)
				kmail=kmail.section(')',1);
				kmail=kmail.section('<',1);
				kmail.truncate(kmail.length()-1);
				if (kmail.find("<")!=-1) ////////  several email addresses in the same key
				{
				kmail=kmail.replace(">",";");
				kmail.remove("<");
				}
				tLMail->setText("<qt><a href=mailto:"+kmail+">"+kmail+"</a></qt>");
                        } else
                                tLMail->setText(i18n("none"));

                        QString kname=fullname.section('<',0,0);
                        if (fullname.find("(")!=-1) {
                                kname=kname.section('(',0,0);
                                QString comment=fullname.section('(',1,1);
                                comment=comment.section(')',0,0);
                                tLComment->setText(KgpgInterface::checkForUtf8(comment));
                        } else
                                tLComment->setText(i18n("none"));

			tLName->setText("<qt><b>"+KgpgInterface::checkForUtf8(kname).replace(QRegExp("<"),"&lt;")+"</b></qt>");

                }
                if (gpgOutput.startsWith("fpr")) {
                        fingervalue=gpgOutput.section(':',9,9);
                        // format fingervalue in 4-digit groups
                        uint len = fingervalue.length();
                        if ((len > 0) && (len % 4 == 0))
                                for (uint n = 0; 4*(n+1) < len; n++)
                                        fingervalue.insert(5*n+4, ' ');
					lEFinger->setText(fingervalue);
                }
        }
        pclose(pass);

        if (hasPhoto) {
                kgpginfotmp=new KTempFile();
                kgpginfotmp->setAutoDelete(true);
                QString pgpgOutput="cp %i "+kgpginfotmp->name();
                KProcIO *p=new KProcIO();
                *p<<"gpg"<<"--show-photos"<<"--photo-viewer"<<QFile::encodeName(pgpgOutput)<<"--list-keys"<<fullID;
                QObject::connect(p, SIGNAL(processExited(KProcess *)),this, SLOT(slotinfoimgread(KProcess *)));
                p->start(KProcess::NotifyOnExit,true);
        }
	connect(changeExp,SIGNAL(clicked()),this,SLOT(slotChangeExp()));
        connect(buttonOk,SIGNAL(clicked()),this,SLOT(slotPreOk1()));
        connect(changePass,SIGNAL(clicked()),this,SLOT(slotChangePass()));
}

KgpgKeyInfo::~KgpgKeyInfo()
{
}

void KgpgKeyInfo::slotChangeExp()
{
chdate=new KDialogBase( this, "choose_date", true,i18n("Choose New Expiration"),KDialogBase::Ok | KDialogBase::Cancel);
QWidget *page = new QWidget(chdate);
kb= new QCheckBox(i18n("Unlimited"),page );

if (tLExpiration->text()==i18n("Unlimited"))
{
kdt= new KDatePicker( page );
kb->setChecked(true);
kdt->setEnabled(false);
}
else
kdt= new KDatePicker(page,KGlobal::locale()->readDate(tLExpiration->text()));
QVBoxLayout *vbox=new QVBoxLayout(page,3);
vbox->addWidget(kdt);
vbox->addWidget(kb);
connect(kb,SIGNAL(toggled(bool)),this,SLOT(slotEnableDate(bool)));
connect(chdate,SIGNAL(okClicked()),this,SLOT(slotChangeDate()));
chdate->setMainWidget(page);
chdate->show();
}

void KgpgKeyInfo::slotChangeDate()
{
if (kb->isChecked()) tLExpiration->setText(i18n("Unlimited"));
else tLExpiration->setText(KGlobal::locale()->formatDate(kdt->getDate()));
}

void KgpgKeyInfo::slotEnableDate(bool isOn)
{
if (isOn) kdt->setEnabled(false);
else kdt->setEnabled(true);
}

void KgpgKeyInfo::slotinfoimgread(KProcess *)
{
	QPixmap pixmap;
        pixmap.load(kgpginfotmp->name());
        pLPhoto->setPixmap(pixmap);
        kgpginfotmp->unlink();
}

void KgpgKeyInfo::slotChangePass()
{
        KgpgInterface *ChangeKeyPassProcess=new KgpgInterface();
        ChangeKeyPassProcess->KgpgChangePass(displayedKeyID);
}

void KgpgKeyInfo::slotPreOk1()
{
        if (expirationDate!=tLExpiration->text()) {
                KgpgInterface *KeyExpirationProcess=new KgpgInterface();
		if (tLExpiration->text()==i18n("Unlimited"))
                KeyExpirationProcess->KgpgKeyExpire(displayedKeyID,QDate::currentDate(),true);
		else
		KeyExpirationProcess->KgpgKeyExpire(displayedKeyID,KGlobal::locale()->readDate(tLExpiration->text()),false);
                connect(KeyExpirationProcess,SIGNAL(expirationFinished(int)),this,SLOT(slotPreOk2(int)));
        } else
                slotPreOk2(0);
}

void KgpgKeyInfo::slotPreOk2(int)
{
        if (ownerTrust!=kCOwnerTrust->currentText()) {
                KgpgInterface *KeyTrustProcess=new KgpgInterface();
                KeyTrustProcess->KgpgTrustExpire(displayedKeyID,kCOwnerTrust->currentText());
                connect(KeyTrustProcess,SIGNAL(trustfinished()),this,SLOT(accept()));
        } else
                accept();
}
