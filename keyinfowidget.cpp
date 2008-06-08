/***************************************************************************
                          keyinfowidget.cpp  -  description
                             -------------------
    begin                : Mon Nov 18 2002
    copyright          : (C) 2002 by Jean-Baptiste Mardelle
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

#include <klocale.h>
#include <kprocess.h>
#include <qdatetime.h>
#include <qfile.h>
#include <kprocio.h>
#include <kpassivepopup.h>
#include <kaction.h>
#include <qregexp.h>
#include <ktempfile.h>
#include <qimage.h>
#include <kdatepicker.h>
#include <qpushbutton.h>
#include <kcombobox.h>
#include <qlabel.h>
#include <kiconloader.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <kactivelabel.h>
#include <klineedit.h>
#include <ktrader.h>
#include <kservice.h>
#include <kmessagebox.h>

#include "keyinfowidget.h"
#include "keyproperties.h"
#include "kgpgsettings.h"
#include "kgpginterface.h"

KgpgKeyInfo::KgpgKeyInfo(QWidget *parent, const char *name,QString sigkey):KDialogBase( Swallow, i18n("Key Properties"), Close, Close, parent, name,true)
{

        FILE *pass;
        char line[200]="";
        QString gpgOutput,fullID;
        hasPhoto=false;
	bool isSecret=false;
	keyWasChanged=false;

	prop=new KeyProperties();
	setMainWidget(prop);
	QString gpgcmd="gpg --no-tty --no-secmem-warning --with-colon --list-secret-key "+KShellProcess::quote(sigkey);
        pass=popen(QFile::encodeName(gpgcmd),"r");
        while ( fgets( line, sizeof(line), pass)) {
                gpgOutput=line;
                if (gpgOutput.startsWith("sec")) isSecret=true;
               }
	 pclose(pass);

        if (!isSecret) {
                prop->changeExp->hide();
                prop->changePass->hide();
		        }
	loadKey(sigkey);

        if (hasPhoto) {
	KgpgInterface *photoProcess=new KgpgInterface();
        photoProcess->KgpgGetPhotoList(displayedKeyID);
	connect(photoProcess,SIGNAL(signalPhotoList(QStringList)),this,SLOT(slotSetMainPhoto(QStringList)));
        }
	else
	prop->comboId->setEnabled(false);

	connect(prop->changeExp,SIGNAL(clicked()),this,SLOT(slotChangeExp()));
        connect(this,SIGNAL(closeClicked()),this,SLOT(slotPreOk()));
        connect(prop->changePass,SIGNAL(clicked()),this,SLOT(slotChangePass()));
	connect(prop->comboId,SIGNAL(activated (const QString &)),this,SLOT(reloadMainPhoto(const QString &)));
	connect(prop->cbDisabled,SIGNAL(toggled(bool)),this,SLOT(slotDisableKey(bool)));
	connect(prop->kCOwnerTrust,SIGNAL(activated (int)),this,SLOT(slotChangeTrust(int)));
	connect(this,SIGNAL(changeMainPhoto(const QPixmap&)),this,SLOT(slotSetPhoto(const QPixmap&)));

	//prop->setMinimumSize(prop->sizeHint());
}

void KgpgKeyInfo::slotDisableKey(bool isOn)
{
KProcess kp;

	kp<<"gpg"
        <<"--no-tty"
	<<"--edit-key"
        <<displayedKeyID;
if (isOn) kp<<"disable";
else kp<<"enable";
	kp<<"save";
        kp.start(KProcess::Block);
loadKey(displayedKeyID);
keyWasChanged=true;
}

void KgpgKeyInfo::loadKey(QString Keyid)
{
QColor trustColor;
QString fingervalue=QString::null;
FILE *pass;
char line[200]="";
QString gpgOutput,fullID;

QString gpgcmd="gpg --no-tty --no-secmem-warning --with-colon --with-fingerprint --list-key "+KShellProcess::quote(Keyid);

        pass=popen(QFile::encodeName(gpgcmd),"r");
        while ( fgets( line, sizeof(line), pass)) {
                gpgOutput=QString::fromUtf8(line);
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
			prop->tLAlgo->setText(algo);

                        const QString trust=gpgOutput.section(':',1,1);
                        QString tr;
                        switch( trust[0] ) {
                        case 'o':
                                tr= i18n("Unknown");
				trustColor=KGpgSettings::colorUnknown();
                                break;
                        case 'i':
                                tr= i18n("Invalid");
				trustColor=KGpgSettings::colorBad();
                                break;
                        case 'd':
                                tr=i18n("Disabled");
				trustColor=KGpgSettings::colorBad();
				prop->cbDisabled->setChecked(true);
                                break;
                        case 'r':
                                tr=i18n("Revoked");
				trustColor=KGpgSettings::colorRev();
                                break;
                        case 'e':
                                tr=i18n("Expired");
				trustColor=KGpgSettings::colorBad();
                                break;
                        case 'q':
                                tr=i18n("Undefined");
				trustColor=KGpgSettings::colorUnknown();
                                break;
                        case 'n':
                                tr=i18n("None");
				trustColor=KGpgSettings::colorUnknown();
                                break;
                        case 'm':
                                tr=i18n("Marginal");
				trustColor=KGpgSettings::colorBad();
                                break;
                        case 'f':
                                tr=i18n("Full");
				trustColor=KGpgSettings::colorGood();
                                break;
                        case 'u':
                                tr=i18n("Ultimate");
				trustColor=KGpgSettings::colorGood();
                                break;
                        default:
                                tr="?";
				trustColor=KGpgSettings::colorUnknown();
                                break;
                        }

			if (gpgOutput.section(':',11,11).find("D",0,true)!=-1)  // disabled key
			{
				tr=i18n("Disabled");
				trustColor=KGpgSettings::colorBad();
				prop->cbDisabled->setChecked(true);
			}

                        prop->kLTrust->setText(tr);
                        prop->pixmapTrust->setPaletteBackgroundColor(trustColor);

			fullID=gpgOutput.section(':',4,4);
                        displayedKeyID=fullID.right(8);
                        prop->tLID->setText(fullID);

                        QString fullname=gpgOutput.section(':',9,9);

                        QDate date = QDate::fromString(gpgOutput.section(':',5,5), Qt::ISODate);
                        prop->tLCreation->setText(KGlobal::locale()->formatDate(date));

			if (gpgOutput.section(':',6,6).isEmpty()) expirationDate=i18n("Unlimited");
			else
			{
			date = QDate::fromString(gpgOutput.section(':',6,6), Qt::ISODate);
			expirationDate=KGlobal::locale()->formatDate(date);
			}
                        prop->tLExpiration->setText(expirationDate);

                        prop->tLLength->setText(gpgOutput.section(':',2,2));

                        const QString otrust=gpgOutput.section(':',8,8);
			int ownerTrust=0;

			/*	Don't know=1; Do NOT trust=2; Marginally=3; Fully=4; Ultimately=5;   */

                        switch( otrust[0] ) {
                        case 'f':
                                ownerTrust=3;
                                break;
                        case 'u':
                                ownerTrust=4;
                                break;
                        case 'm':
                                ownerTrust=2;
                                break;
                        case 'n':
                                ownerTrust=1;
                                break;
                        default:
                                ownerTrust=0;
                                break;
                        }
                        prop->kCOwnerTrust->setCurrentItem(ownerTrust);

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
				prop->tLMail->setText("<qt><a href=mailto:"+kmail+">"+kmail+"</a></qt>");
                        } else
                                prop->tLMail->setText(i18n("none"));

                        QString kname=fullname.section('<',0,0);
                        if (fullname.find("(")!=-1) {
                                kname=kname.section('(',0,0);
                                QString comment=fullname.section('(',1,1);
                                comment=comment.section(')',0,0);
                                prop->tLComment->setText(KgpgInterface::checkForUtf8(comment));
                        } else
                                prop->tLComment->setText(i18n("none"));

			prop->tLName->setText("<qt><b>"+KgpgInterface::checkForUtf8(kname).replace(QRegExp("<"),"&lt;")+"</b></qt>");

                }
		if (gpgOutput.startsWith("fpr") && (fingervalue.isNull())) {
                        fingervalue=gpgOutput.section(':',9,9);
                        // format fingervalue in 4-digit groups
                        uint len = fingervalue.length();
                        if ((len > 0) && (len % 4 == 0))
                                for (uint n = 0; 4*(n+1) < len; n++)
                                        fingervalue.insert(5*n+4, ' ');
					prop->lEFinger->setText(fingervalue);
                }
        }
        pclose(pass);
}

void KgpgKeyInfo::slotSetMainPhoto(QStringList list)
{
prop->comboId->insertStringList(list);
reloadMainPhoto(prop->comboId->currentText());
}

void KgpgKeyInfo::reloadMainPhoto(const QString &uid)
{

                kgpginfotmp=new KTempFile();
                kgpginfotmp->setAutoDelete(true);
                QString pgpgOutput="cp %i "+kgpginfotmp->name();
                KProcIO *p=new KProcIO();
                *p<<"gpg"<<"--no-tty"<<"--show-photos"<<"--photo-viewer"<<QFile::encodeName(pgpgOutput);
		*p<<"--edit-key"<<displayedKeyID<<"uid"<<uid<<"showphoto";
		QObject::connect(p, SIGNAL(readReady(KProcIO *)),this, SLOT(finishphotoreadprocess(KProcIO *)));
                QObject::connect(p, SIGNAL(processExited(KProcess *)),this, SLOT(slotMainImageRead(KProcess *)));
                p->start(KProcess::NotifyOnExit,true);

}


void KgpgKeyInfo::slotMainImageRead(KProcess *p)
{
	p->deleteLater();
	QPixmap pixmap;
        pixmap.load(kgpginfotmp->name());
	emit changeMainPhoto(pixmap);
        kgpginfotmp->unlink();
}


KgpgKeyInfo::~KgpgKeyInfo()
{
}

void KgpgKeyInfo::slotSetPhoto(const QPixmap &pix)
{
QImage dup=pix.convertToImage();
QPixmap dup2;
dup2.convertFromImage(dup.scale(prop->pLPhoto->width(),prop->pLPhoto->height(),QImage::ScaleMin));
prop->pLPhoto->setPixmap(dup2);
}




void KgpgKeyInfo::finishphotoreadprocess(KProcIO *p)
{
        QString required=QString::null;
        while (p->readln(required,true)!=-1)
		if (required.find("keyedit.prompt")!=-1) {
                        p->writeStdin("quit");
			p->closeWhenDone();

                }
}


void KgpgKeyInfo::openPhoto()
{
			 KTrader::OfferList offers = KTrader::self()->query("image/jpeg", "Type == 'Application'");
 			KService::Ptr ptr = offers.first();
 			//KMessageBox::sorry(0,ptr->desktopEntryName());
                        KProcIO *p=new KProcIO();
                        *p<<"gpg"<<"--show-photos"<<"--photo-viewer"<<QFile::encodeName(ptr->desktopEntryName()+" %i")<<"--list-keys"<<displayedKeyID;
                        p->start(KProcess::DontCare,true);
}

void KgpgKeyInfo::slotChangeExp()
{
chdate=new KDialogBase( this, "choose_date", true,i18n("Choose New Expiration"),KDialogBase::Ok | KDialogBase::Cancel);
QWidget *page = new QWidget(chdate);
kb= new QCheckBox(i18n("Unlimited"),page );

if (prop->tLExpiration->text()==i18n("Unlimited"))
{
kdt= new KDatePicker( page );
kb->setChecked(true);
kdt->setEnabled(false);
}
else
kdt= new KDatePicker(page,KGlobal::locale()->readDate(prop->tLExpiration->text()));
QVBoxLayout *vbox=new QVBoxLayout(page,3);
vbox->addWidget(kdt);
vbox->addWidget(kb);
connect(kb,SIGNAL(toggled(bool)),this,SLOT(slotEnableDate(bool)));
connect(chdate,SIGNAL(okClicked()),this,SLOT(slotChangeDate()));
connect(kdt,SIGNAL(dateChanged(QDate)),this,SLOT(slotCheckDate(QDate)));
connect(kdt,SIGNAL(dateEntered(QDate)),this,SLOT(slotCheckDate(QDate)));

chdate->setMainWidget(page);
chdate->show();
}

void KgpgKeyInfo::slotCheckDate(QDate date)
{
chdate->enableButtonOK(date>=QDate::currentDate ());
}

void KgpgKeyInfo::slotChangeDate()
{
KgpgInterface *KeyExpirationProcess=new KgpgInterface();
		if (kb->isChecked())
                KeyExpirationProcess->KgpgKeyExpire(displayedKeyID,QDate::currentDate(),true);
		else
		KeyExpirationProcess->KgpgKeyExpire(displayedKeyID,kdt->date(),false);
                connect(KeyExpirationProcess,SIGNAL(expirationFinished(int)),this,SLOT(slotInfoExpirationChanged(int)));
}

void KgpgKeyInfo::slotEnableDate(bool isOn)
{
if (isOn)
{
kdt->setEnabled(false);
chdate->enableButtonOK(true);
}
else
{
kdt->setEnabled(true);
chdate->enableButtonOK(kdt->date()>=QDate::currentDate ());
}
}

void KgpgKeyInfo::slotinfoimgread(KProcess *)
{
	QPixmap pixmap;
        pixmap.load(kgpginfotmp->name());
	emit signalPhotoId(pixmap);
        kgpginfotmp->unlink();
}

void KgpgKeyInfo::slotChangePass()
{
        KgpgInterface *ChangeKeyPassProcess=new KgpgInterface();
        ChangeKeyPassProcess->KgpgChangePass(displayedKeyID);
	connect(ChangeKeyPassProcess,SIGNAL(passwordChanged()),this,SLOT(slotInfoPasswordChanged()));
}

void KgpgKeyInfo::slotChangeTrust(int newTrust)
{
        KgpgInterface *KeyTrustProcess=new KgpgInterface();
                KeyTrustProcess->KgpgTrustExpire(displayedKeyID,newTrust);
                connect(KeyTrustProcess,SIGNAL(trustfinished()),this,SLOT(slotInfoTrustChanged()));
}


void KgpgKeyInfo::slotInfoPasswordChanged()
{
KPassivePopup::message(i18n("Passphrase for the key was changed"),QString::null,KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop),this);
}

void KgpgKeyInfo::slotInfoTrustChanged()
{
keyWasChanged=true;
loadKey(displayedKeyID);
//KPassivePopup::message(i18n("Owner trust of the key was changed"),QString::null,KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop),this,0,600);
}

void KgpgKeyInfo::slotInfoExpirationChanged(int res)
{
QString infoMessage,infoText;
if (res==3)
{
keyWasChanged=true;
if (kb->isChecked()) prop->tLExpiration->setText(i18n("Unlimited"));
else prop->tLExpiration->setText(KGlobal::locale()->formatDate(kdt->date()));
}
if (res==2) {
infoMessage=i18n("Could not change expiration");infoText=i18n("Bad passphrase");
KPassivePopup::message(infoMessage,infoText,KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop),this);
}
}


void KgpgKeyInfo::slotPreOk()
{
if (keyWasChanged) emit keyNeedsRefresh();
accept();
}

#include "keyinfowidget.moc"
