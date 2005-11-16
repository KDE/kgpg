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
#include <QStringList>
#include <QCheckBox>
#include <QColor>
#include <QImage>

#include <kpassivepopup.h>
#include <kdatepicker.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <klocale.h>

#include "keyinfowidget.h"
#include "keyproperties.h"
#include "kgpginterface.h"

KgpgKeyInfo::KgpgKeyInfo(const QString &sigkey, QWidget *parent, const char *name)
           : KDialogBase(Swallow, i18n("Key Properties"), Close, Close, parent, name, true)
{
    m_hasphoto = false;
    m_keywaschanged = false;
    m_isunlimited = false;

    m_prop = new KeyProperties();
    setMainWidget(m_prop);

    KgpgInterface *interface = new KgpgInterface();
    KgpgListKeys keys = interface->readSecretKeys(true, QStringList(sigkey));
    delete interface;

    bool issecret = false;
    if (keys.size() > 0)
        issecret = true;

    if (!issecret)
    {
        m_prop->changeExp->hide();
        m_prop->changePass->hide();
    }

    loadKey(sigkey);
    if (!m_hasphoto)
        m_prop->comboId->setEnabled(false);
    else
        slotReloadMainPhoto(m_prop->comboId->currentText());

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
    KgpgKey key = listkeys.at(0);

    m_prop->tLAlgo->setText(KgpgKey::algorithmeToString(key.algorithme()));

    QString tr = KgpgKey::trustToString(key.trust());
    QColor trustcolor = KgpgKey::color(key.trust());
    if (key.trust() == 'd')
        m_prop->cbDisabled->setChecked(true);
    if (!key.valide())
    {
        tr = KgpgKey::trustToString('d');
        trustcolor = KgpgKey::color('d');
        m_prop->cbDisabled->setChecked(true);
    }
    m_prop->kLTrust->setText(tr);
    m_prop->pixmapTrust->setPaletteBackgroundColor(trustcolor);

    QStringList photolist = key.photoList();
    if (!photolist.isEmpty())
    {
        m_hasphoto = true;
        m_prop->comboId->insertStringList(photolist);
    }

    m_prop->tLID->setText(key.fullId());
    m_displayedkeyid = key.id();
    m_prop->tLCreation->setText(key.creation());
    if (key.unlimited())
    {
        m_isunlimited = true;
        m_prop->tLExpiration->setText(i18n("Unlimited"));
    }
    else
    {
        m_isunlimited = false;
        m_prop->tLExpiration->setText(key.expiration());
        m_date = key.expirationDate();
    }

    m_prop->tLLength->setText(key.size());
    m_prop->kCOwnerTrust->setCurrentItem(KgpgKey::ownerTrustIndex(key.ownerTrust()));

    if (!key.email().isEmpty())
        m_prop->tLMail->setText("<qt><a href=mailto:" + key.email() + ">" + key.email() + "</a></qt>");
    else
        m_prop->tLMail->setText(i18n("none"));

    if (!key.comment().isEmpty())
        m_prop->tLComment->setText(KgpgInterface::checkForUtf8(key.comment()));
    else
        m_prop->tLComment->setText(i18n("none"));

    m_prop->tLName->setText("<qt><b>" + KgpgInterface::checkForUtf8(key.name()) + "</b></qt>");
    m_prop->lEFinger->setText(key.fingerprint());
}

void KgpgKeyInfo::slotPreOk()
{
    if (m_keywaschanged)
        emit keyNeedsRefresh();
    accept();
}

void KgpgKeyInfo::slotDisableKey(const bool &ison)
{
    KgpgInterface *interface = new KgpgInterface;
    connect (interface, SIGNAL(changeDisableFinished(KgpgInterface*)), this, SLOT(slotDisableKeyFinished(KgpgInterface*)));
    interface->changeDisable(m_displayedkeyid, ison);
}

void KgpgKeyInfo::slotDisableKeyFinished(KgpgInterface *interface)
{
    delete interface;
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

void KgpgKeyInfo::slotCheckDate(const QDate &date)
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

void KgpgKeyInfo::slotEnableDate(const bool &ison)
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

void KgpgKeyInfo::slotInfoExpirationChanged(const int &res, KgpgInterface *interface)
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
        QString infomessage = i18n("Could not change expiration");
        QString infotext = i18n("Bad passphrase. Expiration of the key has not been changed.");
        KMessageBox::error(this, infotext, infomessage);
    }
}

void KgpgKeyInfo::slotChangePass()
{
    KgpgInterface *interface = new KgpgInterface();
    interface->changePass(m_displayedkeyid);
    connect(interface, SIGNAL(changePassFinished(int, KgpgInterface*)), this, SLOT(slotInfoPasswordChanged(int, KgpgInterface*)));
}

void KgpgKeyInfo::slotInfoPasswordChanged(const int &res, KgpgInterface *interface)
{
    delete interface;

    if (res == 2)
        KPassivePopup::message(i18n("Passphrase for the key was changed"), QString::null, KGlobal::iconLoader()->loadIcon("kgpg", KIcon::Desktop), this);

    if (res == 1)
        KMessageBox::error(this, i18n("Bad old passphrase, the passphrase for the key was not changed"), i18n("Could not change passphrase"));
}

void KgpgKeyInfo::slotChangeTrust(const int &newtrust)
{
    KgpgInterface *interface = new KgpgInterface();
    interface->changeTrust(m_displayedkeyid, newtrust);
    connect(interface, SIGNAL(changeTrustFinished(KgpgInterface*)), this, SLOT(slotInfoTrustChanged(KgpgInterface*)));
}

void KgpgKeyInfo::slotInfoTrustChanged(KgpgInterface *interface)
{
    delete interface;
    m_keywaschanged = true;
    loadKey(m_displayedkeyid);
}

void KgpgKeyInfo::slotReloadMainPhoto(const QString &uid)
{
    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(loadPhotoFinished(QPixmap, KgpgInterface*)), this, SLOT(slotMainImageRead(QPixmap, KgpgInterface*)));
    interface->loadPhoto(m_displayedkeyid, uid);
}

void KgpgKeyInfo::slotMainImageRead(const QPixmap &pixmap, KgpgInterface *interface)
{
    delete interface;
    emit changeMainPhoto(pixmap);
}

void KgpgKeyInfo::slotSetPhoto(const QPixmap &pixmap)
{
    QImage dup = pixmap.convertToImage();
    QPixmap dup2;
    dup2.convertFromImage(dup. scaled(m_prop->pLPhoto->width(), m_prop->pLPhoto->height(), Qt::KeepAspectRatio));
    m_prop->pLPhoto->setPixmap(dup2);
}

#include "keyinfowidget.moc"
