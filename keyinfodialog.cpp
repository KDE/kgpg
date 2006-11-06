/***************************************************************************
                          keyinfodialog.cpp  -  description
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

#include "keyinfodialog.h"
#include "kgpginterface.h"
#include "convert.h"

using namespace KgpgCore;

KgpgKeyInfo::KgpgKeyInfo(const QString &sigkey, QWidget *parent)
           : KDialog(parent)
{
    setCaption(i18n("Key Properties"));
    setButtons(Close);
    setDefaultButton(Close);
    setModal(true);

    m_hasphoto = false;
    m_keywaschanged = false;
    m_isunlimited = false;

    m_prop = new KeyProperties();
    setMainWidget(m_prop);

    KgpgInterface *interface = new KgpgInterface();
    KeyList keys = interface->readSecretKeys(true, QStringList(sigkey));
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
    KeyList listkeys = interface->readPublicKeys(true, QStringList(Keyid));
    delete interface;
    Key key = listkeys.at(0);

    m_prop->tLAlgo->setText(Convert::toString(key.algorithme()));

    QString tr = Convert::toString(key.trust());
    QColor trustcolor = Convert::toColor(key.trust());
    if (key.trust() == TRUST_DISABLED)
        m_prop->cbDisabled->setChecked(true);
    if (!key.valide())
    {
        tr = Convert::toString(TRUST_DISABLED);
        trustcolor = Convert::toColor(TRUST_DISABLED);
        m_prop->cbDisabled->setChecked(true);
    }
    m_prop->kLTrust->setText(tr);

    QPalette palette;
    palette.setColor(m_prop->pixmapTrust->backgroundRole(), trustcolor);
    m_prop->pixmapTrust->setPalette(palette);

    QStringList photolist = key.photoList();
    if (!photolist.isEmpty())
    {
        m_hasphoto = true;
        m_prop->comboId->addItems(photolist);
    }

    m_prop->tLID->setPlainText(key.fullId());
    m_displayedkeyid = key.id();
    m_prop->tLCreation->setPlainText(key.creation());
    if (key.unlimited())
    {
        m_isunlimited = true;
        m_prop->tLExpiration->setPlainText(i18n("Unlimited"));
    }
    else
    {
        m_isunlimited = false;
        m_prop->tLExpiration->setPlainText(key.expiration());
        m_date = key.expirationDate();
    }

    m_prop->tLLength->setText(key.size());
    //m_prop->kCOwnerTrust->setCurrentIndex(KgpgKey::ownerTrustIndex(key.ownerTrust())); // FIXME

    if (!key.email().isEmpty())
        m_prop->tLMail->setPlainText("<qt><a href=mailto:" + key.email() + ">" + key.email() + "</a></qt>");
    else
        m_prop->tLMail->setPlainText(i18n("none"));

    if (!key.comment().isEmpty())
        m_prop->tLComment->setPlainText(KgpgInterface::checkForUtf8(key.comment()));
    else
        m_prop->tLComment->setPlainText(i18n("none"));

    m_prop->tLName->setPlainText("<qt><b>" + KgpgInterface::checkForUtf8(key.name()) + "</b></qt>");
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
    m_chdate = new KDialog(this );
    m_chdate->setCaption( i18n("Choose New Expiration") );
    m_chdate->setButtons(  Ok | Cancel);
    m_chdate->setDefaultButton( Ok );
    m_chdate->setModal(true);
    QWidget *page = new QWidget(m_chdate);
    m_kb = new QCheckBox(i18n("Unlimited"), page);

    if (m_isunlimited)
    {
        m_kdt = new KDatePicker(page);
        m_kb->setChecked(true);
        m_kdt->setEnabled(false);
    }
    else
        m_kdt = new KDatePicker(m_date, page);

    QVBoxLayout *vbox = new QVBoxLayout(page);
    vbox->setSpacing(3);
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
    m_chdate->enableButtonOk(date >= QDate::currentDate());
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
        m_chdate->enableButtonOk(true);
    }
    else
    {
        m_kdt->setEnabled(true);
        m_chdate->enableButtonOk(m_kdt->date() >= QDate::currentDate());
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
            m_prop->tLExpiration->setPlainText(i18n("Unlimited"));
        }
        else
        {
            m_isunlimited = false;
            m_date = m_kdt->date();
            m_prop->tLExpiration->setPlainText(KGlobal::locale()->formatDate(m_kdt->date()));
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
        KPassivePopup::message(i18n("Passphrase for the key was changed"), QString::null, KGlobal::iconLoader()->loadIcon("kgpg", K3Icon::Desktop), this);

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
    QImage dup = pixmap.toImage();
    QPixmap dup2 = QPixmap::fromImage( dup.scaled(m_prop->pLPhoto->width(), m_prop->pLPhoto->height(), Qt::KeepAspectRatio));
    m_prop->pLPhoto->setPixmap(dup2);
}

#include "keyinfodialog.moc"
