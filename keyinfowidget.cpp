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
 *   (at your gpgOutpution) any later version.                             *
 *                                                                         *
 ***************************************************************************/

#include <QVBoxLayout>
#include <QPushButton>
#include <QDateTime>
#include <QCheckBox>
#include <QLayout>
#include <QPixmap>
#include <QRegExp>
#include <QImage>
#include <QLabel>
#include <QFile>

#include <kpassivepopup.h>
#include <kactivelabel.h>
#include <kdatepicker.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <ktempfile.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <kprocess.h>
#include <kservice.h>
#include <klocale.h>
#include <kprocio.h>
#include <kaction.h>
#include <ktrader.h>

#include "keyinfowidget.h"
#include "keyproperties.h"
#include "kgpgsettings.h"
#include "kgpginterface.h"

KgpgKeyInfo::KgpgKeyInfo(QWidget *parent, const char *name, QString sigkey)
           : KDialogBase(Swallow, i18n("Key Properties"), Close, Close, parent, name, true)
{
    m_hasphoto = false;
    m_keywaschanged = false;
    m_isunlimited = false;

    m_prop = new KeyProperties();
    setMainWidget(m_prop);

    bool isSecret = false;
    KgpgInterface *interface = new KgpgInterface();
    KgpgListKeys keys = interface->readSecretKeys(true, QStringList(sigkey));
    if (keys.count() > 0)
        isSecret = true;
    delete interface;

    if (!isSecret)
    {
        m_prop->changeExp->hide();
        m_prop->changePass->hide();
    }

    loadKey(sigkey);

    if (m_hasphoto)
    {
        KgpgInterface *photoProcess = new KgpgInterface();
        connect(photoProcess, SIGNAL(signalPhotoList(QStringList)), this, SLOT(slotSetMainPhoto(QStringList)));
        photoProcess->KgpgGetPhotoList(m_displayedkeyid);
    }
    else
        m_prop->comboId->setEnabled(false);

    connect(m_prop->changeExp, SIGNAL(clicked()), this, SLOT(slotChangeExp()));
    connect(this, SIGNAL(closeClicked()), this, SLOT(slotPreOk()));
    connect(m_prop->changePass,SIGNAL(clicked()), this, SLOT(slotChangePass()));
    connect(m_prop->comboId, SIGNAL(activated (const QString &)), this, SLOT(slotReloadMainPhoto(const QString &)));
    connect(m_prop->cbDisabled, SIGNAL(toggled(bool)), this, SLOT(slotDisableKey(bool)));
    connect(m_prop->kCOwnerTrust, SIGNAL(activated (int)), this, SLOT(slotChangeTrust(int)));
    connect(this, SIGNAL(changeMainPhoto(const QPixmap&)), this, SLOT(slotSetPhoto(const QPixmap&)));
}

void KgpgKeyInfo::loadKey(const QString &Keyid)
{
    KgpgInterface *interface = new KgpgInterface();
    KgpgListKeys listkeys = interface->readPublicKeys(true, QStringList(Keyid));
    delete interface;
    KgpgKeyPtr key = listkeys.at(0);

    m_prop->tLAlgo->setText(KgpgKey::algorithme(key->gpgkeyalgo));

    QString tr = KgpgKey::trust(key->gpgkeytrust);
    QColor trustcolor = KgpgKey::color(key->gpgkeytrust);
    if (key->gpgkeytrust == 'd')
        m_prop->cbDisabled->setChecked(true);
    if (!key->gpgkeyvalide)
    {
        tr = KgpgKey::trust('d');
        trustcolor = KgpgKey::color('d');
        m_prop->cbDisabled->setChecked(true);
    }
    m_prop->kLTrust->setText(tr);
    m_prop->pixmapTrust->setPaletteBackgroundColor(trustcolor);

    m_hasphoto = key->gpghasphoto;
    m_prop->tLID->setText(key->gpgfullid);
    m_displayedkeyid = key->gpgkeyid;
    m_prop->tLCreation->setText(KGlobal::locale()->formatDate(key->gpgkeycreation));
    if (key->gpgkeyunlimited)
    {
        m_isunlimited = true;
        m_prop->tLExpiration->setText(i18n("Unlimited"));
    }
    else
    {
        m_isunlimited = false;
        m_prop->tLExpiration->setText(KGlobal::locale()->formatDate(key->gpgkeyexpiration));
        m_date = key->gpgkeyexpiration;
    }

    m_prop->tLLength->setText(key->gpgkeysize);
    m_prop->kCOwnerTrust->setCurrentItem(KgpgKey::ownerTrustIndex(key->gpgkeyownertrust));

    if (!key->gpgkeymail.isEmpty())
        m_prop->tLMail->setText("<qt><a href=mailto:" + key->gpgkeymail + ">" + key->gpgkeymail + "</a></qt>");
    else
        m_prop->tLMail->setText(i18n("none"));

    if (!key->gpgkeycomment.isEmpty())
        m_prop->tLComment->setText(KgpgInterface::checkForUtf8(key->gpgkeycomment));
    else
        m_prop->tLComment->setText(i18n("none"));

    m_prop->tLName->setText("<qt><b>" + KgpgInterface::checkForUtf8(key->gpgkeyname) + "</b></qt>");
    m_prop->lEFinger->setText(key->gpgkeyfingerprint);
}

void KgpgKeyInfo::slotPreOk()
{
    if (m_keywaschanged)
        emit keyNeedsRefresh();
    accept();
}

// TODO put this method in KgpgInterface
void KgpgKeyInfo::slotDisableKey(const bool &ison)
{
    KProcIO kp;
    kp << "gpg" << "--no-tty" << "--edit-key" << m_displayedkeyid;

    if (ison)
        kp << "disable";
    else
        kp << "enable";

    kp << "save";

    kp.start(KProcess::Block);
    loadKey(m_displayedkeyid);
    m_keywaschanged = true;
}

void KgpgKeyInfo::slotChangeExp()
{
    m_chdate = new KDialogBase(this, "choose_date", true, i18n("Choose New Expiration"), Ok | Cancel);
    QWidget *page = new QWidget(m_chdate);
    m_kb = new QCheckBox(i18n("Unlimited"), page);

    if (m_isunlimited)
    {
        m_kdt = new KDatePicker(page);
        m_kb->setChecked(true);
        m_kdt->setEnabled(false);
    }
    else
        m_kdt = new KDatePicker(page, m_date);

    QVBoxLayout *vbox = new QVBoxLayout(page, 3);
    vbox->addWidget(m_kdt);
    vbox->addWidget(m_kb);

    connect(m_kb, SIGNAL(toggled(bool)), this, SLOT(slotEnableDate(bool)));
    connect(m_chdate, SIGNAL(okClicked()), this, SLOT(slotChangeDate()));
    connect(m_kdt, SIGNAL(dateChanged(QDate)), this, SLOT(slotCheckDate(QDate)));
    connect(m_kdt, SIGNAL(dateEntered(QDate)), this, SLOT(slotCheckDate(QDate)));

    m_chdate->setMainWidget(page);
    m_chdate->show();
}

void KgpgKeyInfo::slotCheckDate(QDate date)
{
    m_chdate->enableButtonOK(date >= QDate::currentDate());
}

void KgpgKeyInfo::slotChangeDate()
{
    KgpgInterface *KeyExpirationProcess = new KgpgInterface();
    connect(KeyExpirationProcess, SIGNAL(keyExpireFinished(int, KgpgInterface*)), this, SLOT(slotInfoExpirationChanged(int, KgpgInterface*)));

    if (m_kb->isChecked())
        KeyExpirationProcess->keyExpire(m_displayedkeyid, QDate::currentDate(), true);
    else
        KeyExpirationProcess->keyExpire(m_displayedkeyid, m_kdt->date(), false);
}

void KgpgKeyInfo::slotEnableDate(bool ison)
{
    if (ison)
    {
        m_kdt->setEnabled(false);
        m_chdate->enableButtonOK(true);
    }
    else
    {
        m_kdt->setEnabled(true);
        m_chdate->enableButtonOK(m_kdt->date() >= QDate::currentDate());
    }
}

void KgpgKeyInfo::slotInfoExpirationChanged(int res, KgpgInterface *interface)
{
    delete interface;

    if (res == 2)
    {
        m_keywaschanged = true;
        if (m_kb->isChecked())
        {
            m_isunlimited = true;
            m_date = QDate::currentDate();
            m_prop->tLExpiration->setText(i18n("Unlimited"));
        }
        else
        {
            m_isunlimited = false;
            m_date = m_kdt->date();
            m_prop->tLExpiration->setText(KGlobal::locale()->formatDate(m_kdt->date()));
        }
    }

    if (res == 1)
    {
        QString infoMessage = i18n("Could not change expiration");
        QString infoText = i18n("Bad passphrase");
        KPassivePopup::message(infoMessage, infoText, KGlobal::iconLoader()->loadIcon("kgpg", KIcon::Desktop), this);
    }
}

void KgpgKeyInfo::slotChangePass()
{
    KgpgInterface *interface = new KgpgInterface();
    interface->changePass(m_displayedkeyid);
    connect(interface, SIGNAL(changePassFinished(int, KgpgInterface*)), this, SLOT(slotInfoPasswordChanged(int, KgpgInterface*)));
}

void KgpgKeyInfo::slotInfoPasswordChanged(int res, KgpgInterface *interface)
{
    delete interface;

     if (res == 2)
        KPassivePopup::message(i18n("Passphrase for the key was changed"), QString::null, KGlobal::iconLoader()->loadIcon("kgpg", KIcon::Desktop), this);

    if (res == 1)
        KPassivePopup::message(i18n("Bad old passphrase, the passphrase for the key was not changed"), QString::null, KGlobal::iconLoader()->loadIcon("kgpg", KIcon::Desktop), this);
}

void KgpgKeyInfo::slotChangeTrust(int newTrust)
{
    KgpgInterface *interface = new KgpgInterface();
    interface->changeTrust(m_displayedkeyid, newTrust);
    connect(interface, SIGNAL(changeTrustFinished(KgpgInterface*)), this, SLOT(slotInfoTrustChanged(KgpgInterface*)));
}

void KgpgKeyInfo::slotInfoTrustChanged(KgpgInterface *interface)
{
    delete interface;
    m_keywaschanged = true;
    loadKey(m_displayedkeyid);
}







void KgpgKeyInfo::slotSetMainPhoto(QStringList list)
{
    m_prop->comboId->insertStringList(list);
    slotReloadMainPhoto(m_prop->comboId->currentText());
}

void KgpgKeyInfo::slotReloadMainPhoto(const QString &uid)
{
    m_kgpginfotmp = new KTempFile();
    m_kgpginfotmp->setAutoDelete(true);

    QString pgpgOutput = "cp %i " + m_kgpginfotmp->name();

    KProcIO *p = new KProcIO();
    *p << "gpg" << "--no-tty" << "--show-photos" << "--photo-viewer" << QFile::encodeName(pgpgOutput);
    *p << "--edit-key" << m_displayedkeyid << "uid" << uid << "showphoto";

    connect(p, SIGNAL(readReady(KProcIO *)), this, SLOT(finishphotoreadprocess(KProcIO *)));
    connect(p, SIGNAL(processExited(KProcess *)), this, SLOT(slotMainImageRead(KProcess *)));
    p->start(KProcess::NotifyOnExit, true);
}

void KgpgKeyInfo::slotMainImageRead(KProcess *)
{
    QPixmap pixmap;
    pixmap.load(m_kgpginfotmp->name());
    emit changeMainPhoto(pixmap);
    m_kgpginfotmp->unlink();
}

void KgpgKeyInfo::slotSetPhoto(const QPixmap &pix)
{
    QImage dup = pix.convertToImage();
    QPixmap dup2;
    dup2.convertFromImage(dup. scaled(m_prop->pLPhoto->width(), m_prop->pLPhoto->height(), Qt::KeepAspectRatio));
    m_prop->pLPhoto->setPixmap(dup2);
}

void KgpgKeyInfo::finishphotoreadprocess(KProcIO *p)
{
    QString required = QString::null;
    while (p->readln(required, true) != -1)
        if (required.find("keyedit.prompt") != -1)
        {
            p->writeStdin(QByteArray("quit"));
            p->closeWhenDone();
        }
}

void KgpgKeyInfo::openPhoto()
{
    KTrader::OfferList offers = KTrader::self()->query("image/jpeg", "Type == 'Application'");
    KService::Ptr ptr = offers.first();
    //KMessageBox::sorry(0,ptr->desktopEntryName());

    KProcIO *p = new KProcIO();
    *p << "gpg" << "--show-photos" << "--photo-viewer" << QFile::encodeName(ptr->desktopEntryName() + " %i") << "--list-keys" << m_displayedkeyid;
    p->start(KProcess::DontCare, true);
}

void KgpgKeyInfo::slotinfoimgread(KProcess *)
{
    QPixmap pixmap;
    pixmap.load(m_kgpginfotmp->name());
    emit signalPhotoId(pixmap);
    m_kgpginfotmp->unlink();
}

#include "keyinfowidget.moc"
