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
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "keyinfodialog.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QCheckBox>
#include <QPainter>
#include <QPixmap>
#include <QImage>

#include <KToolInvocation>
#include <KPassivePopup>
#include <KDatePicker>
#include <KMessageBox>
#include <KUrlLabel>
#include <KStandardDirs>
#include <KIconEffect>
#include <KLocale>

#include "kgpginterface.h"
#include "convert.h"
#include "images.h"

using namespace KgpgCore;

KgpgTrustLabel::KgpgTrustLabel(QWidget *parent, const QString &text, const QColor &color)
              : QWidget(parent)
{
    m_text_w = new QLabel(this);
    m_text_w->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_color_w = new QLabel(this);
    m_color_w->setLineWidth(1);
    m_color_w->setFrameShape(QFrame::Box);
    m_color_w->setAutoFillBackground(true);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setMargin(2);
    layout->addWidget(m_text_w);
    layout->addWidget(m_color_w);

    m_text = text;
    m_color = color;
    change();
}

void KgpgTrustLabel::setText(const QString &text)
{
    m_text = text;
    change();
}

void KgpgTrustLabel::setColor(const QColor &color)
{
    m_color = color;
    change();
}

QString KgpgTrustLabel::text() const
{
    return m_text;
}

QColor KgpgTrustLabel::color() const
{
    return m_color;
}

void KgpgTrustLabel::change()
{
    m_text_w->setText(m_text);

    QPalette palette = m_color_w->palette();
    palette.setColor(m_color_w->backgroundRole(), m_color);
    m_color_w->setPalette(palette);
}

KgpgDateDialog::KgpgDateDialog(QWidget *parent, const bool &unlimited, QDate date)
              : KDialog(parent)
{
    setCaption(i18n("Choose New Expiration"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);
    setModal(true);

    QWidget *page = new QWidget(this);
    m_unlimited = new QCheckBox(i18nc("Key has unlimited lifetime", "Unlimited"), page);

    if (unlimited)
        date = QDate::currentDate();

    m_datepicker = new KDatePicker(date, page);
    if (unlimited)
    {
        m_datepicker->setEnabled(false);
        m_unlimited->setChecked(true);
    }

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setSpacing(3);
    layout->addWidget(m_datepicker);
    layout->addWidget(m_unlimited);

    connect(m_unlimited, SIGNAL(toggled(bool)), this, SLOT(slotEnableDate(bool)));
    connect(m_datepicker, SIGNAL(dateChanged(QDate)), this, SLOT(slotCheckDate(QDate)));
    connect(m_datepicker, SIGNAL(dateEntered(QDate)), this, SLOT(slotCheckDate(QDate)));

    setMainWidget(page);
    show();
}

QDate KgpgDateDialog::date() const
{
    return m_datepicker->date();
}

bool KgpgDateDialog::unlimited() const
{
    return m_unlimited->isChecked();
}

void KgpgDateDialog::slotCheckDate(const QDate &date)
{
    enableButtonOk(date >= QDate::currentDate());
}

void KgpgDateDialog::slotEnableDate(const bool &ison)
{
    if (ison)
    {
        m_datepicker->setEnabled(false);
        enableButtonOk(true);
    }
    else
    {
        m_datepicker->setEnabled(true);
        enableButtonOk(m_datepicker->date() >= QDate::currentDate());
    }
}

KgpgKeyInfo::KgpgKeyInfo(const QString &keyid, QWidget *parent)
           : KDialog(parent)
{
    setButtons(Close);
    setDefaultButton(Close);
    setModal(true);

    m_keyid = keyid;
    m_keywaschanged = false;

    QWidget *page = new QWidget(this);
    QWidget *top = new QWidget(page);
    QWidget *right = new QWidget(top);

    QGroupBox *gr_properties = _keypropertiesGroup(top);
    QGroupBox *gr_photo = _photoGroup(right);
    QGroupBox *gr_buttons = _buttonsGroup(right);
    QGroupBox *gr_fingerprint = _fingerprintGroup(page);

    QVBoxLayout *layout_right = new QVBoxLayout(right);
    layout_right->setSpacing(spacingHint());
    layout_right->setMargin(0);
    layout_right->addWidget(gr_photo);
    layout_right->addWidget(gr_buttons);

    QHBoxLayout *layout_top = new QHBoxLayout(top);
    layout_top->setSpacing(spacingHint());
    layout_top->setMargin(0);
    layout_top->addWidget(gr_properties);
    layout_top->addWidget(right);

    QVBoxLayout *layout_page = new QVBoxLayout(page);
    layout_page->setSpacing(spacingHint());
    layout_page->setMargin(0);
    layout_page->addWidget(top);
    layout_page->addWidget(gr_fingerprint);

    setMainWidget(page);

    connect(m_owtrust, SIGNAL(activated(int)), this, SLOT(slotChangeTrust(int)));
    connect(m_photoid, SIGNAL(activated (const QString &)), this, SLOT(slotLoadPhoto(const QString &)));
    connect(m_email, SIGNAL(leftClickedUrl(const QString &)), this, SLOT(slotOpenUrl(const QString &)));
    connect(this, SIGNAL(closeClicked()), this, SLOT(slotPreOk()));

    loadKey();
    if (!m_hasphoto)
        m_photoid->setEnabled(false);
    else
        slotLoadPhoto(m_photoid->currentText());
}

QGroupBox* KgpgKeyInfo::_keypropertiesGroup(QWidget *parent)
{
    QGroupBox *group = new QGroupBox(i18n("Key properties"), parent);

    /************************/
    /* --- name / email --- */

    QWidget *widget_name = new QWidget(group);

    m_name = new QLabel(widget_name);
    m_name->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_email = new KUrlLabel(widget_name);
    m_email->setUnderline(false);
    m_email->setTextInteractionFlags(Qt::TextSelectableByMouse);

    /**********************/
    /* --- properties --- */

    QWidget *widget_properties = new QWidget(group);

    QLabel *tl_id = new QLabel(i18n("Key ID:"), widget_properties);
    QLabel *tl_comment = new QLabel(i18n("Comment:"), widget_properties);
    QLabel *tl_creation = new QLabel(i18n("Creation:"), widget_properties);
    QLabel *tl_expiration = new QLabel(i18n("Expiration:"), widget_properties);
    QLabel *tl_trust = new QLabel(i18n("Trust:"), widget_properties);
    QLabel *tl_owtrust = new QLabel(i18n("Owner trust:"), widget_properties);
    QLabel *tl_algorithm = new QLabel(i18n("Algorithm:"), widget_properties);
    QLabel *tl_length = new QLabel(i18n("Length:"), widget_properties);

    m_id = new QLabel(widget_properties);
    m_comment = new QLabel(widget_properties);
    m_creation = new QLabel(widget_properties);
    m_expiration = new QLabel(widget_properties);
    m_trust = new KgpgTrustLabel(widget_properties);
    m_owtrust = new KComboBox(widget_properties);
    m_algorithm = new QLabel(widget_properties);
    m_length = new QLabel(widget_properties);

    m_owtrust->addItem(i18n("Don't Know"));
    m_owtrust->addItem(i18n("Do NOT Trust"));
    m_owtrust->addItem(i18n("Marginally"));
    m_owtrust->addItem(i18n("Fully"));
    m_owtrust->addItem(i18n("Ultimately"));

    m_id->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_comment->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_creation->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_expiration->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_algorithm->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_length->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QHBoxLayout *layout_name = new QHBoxLayout(widget_name);
    layout_name->setMargin(0);
    layout_name->setSpacing(spacingHint());
    layout_name->addWidget(m_name);
    layout_name->addWidget(m_email);
    layout_name->addStretch();

    QGridLayout *layout_properties = new QGridLayout(widget_properties);
    layout_properties->setMargin(0);
    layout_properties->setSpacing(spacingHint());
    layout_properties->addWidget(tl_id, 0, 0, Qt::AlignRight);
    layout_properties->addWidget(m_id, 0, 1);
    layout_properties->addWidget(tl_comment, 1, 0, Qt::AlignRight);
    layout_properties->addWidget(m_comment, 1, 1);
    layout_properties->addWidget(tl_creation, 2, 0, Qt::AlignRight);
    layout_properties->addWidget(m_creation, 2, 1);
    layout_properties->addWidget(tl_expiration, 3, 0, Qt::AlignRight);
    layout_properties->addWidget(m_expiration, 3, 1);
    layout_properties->addWidget(tl_trust, 4, 0, Qt::AlignRight);
    layout_properties->addWidget(m_trust, 4, 1);
    layout_properties->addWidget(tl_owtrust, 5, 0, Qt::AlignRight);
    layout_properties->addWidget(m_owtrust, 5, 1);
    layout_properties->addWidget(tl_algorithm, 6, 0, Qt::AlignRight);
    layout_properties->addWidget(m_algorithm, 6, 1);
    layout_properties->addWidget(tl_length, 7, 0, Qt::AlignRight);
    layout_properties->addWidget(m_length, 7, 1);
    layout_properties->setColumnStretch(1, 1);
    layout_properties->setRowStretch(8, 1);

    QVBoxLayout *layout_keyproperties = new QVBoxLayout(group);
    layout_keyproperties->addWidget(widget_name);
    layout_keyproperties->addWidget(widget_properties);

    return group;
}

QGroupBox* KgpgKeyInfo::_photoGroup(QWidget *parent)
{
    QGroupBox *group = new QGroupBox(i18n("Photo"), parent);
    m_photo = new QLabel(i18n("No Photo"), group);
    m_photoid = new KComboBox(group);

    m_photo->setMinimumSize(120, 140);
    m_photo->setMaximumSize(32767, 140);
    m_photo->setLineWidth(2);
    m_photo->setAlignment(Qt::AlignCenter);
    m_photo->setFrameShape(QFrame::Box);
    m_photo->setWhatsThis("<qt><b>Photo:</b><p>A photo can be included with a public key for extra security. The photo can be used as an additional method of authenticating the key. However, it should not be relied upon as the only form of authentication.</p></qt>");

    QVBoxLayout *layout = new QVBoxLayout(group);
    layout->setMargin(marginHint());
    layout->setSpacing(spacingHint());
    layout->addWidget(m_photo);
    layout->addWidget(m_photoid);
    layout->addStretch();

    return group;
}

QGroupBox* KgpgKeyInfo::_buttonsGroup(QWidget *parent)
{
    KgpgInterface *interface = new KgpgInterface();
    KgpgKeyList keys = interface->readSecretKeys(QStringList(m_keyid));
    delete interface;
    bool isscretkey = keys.size() != 0;

    QPushButton *expiration = 0;
    QPushButton *password = 0;

    QGroupBox *group = new QGroupBox(parent);
    m_disable = new QCheckBox(i18n("Disable key"), group);

    if (isscretkey)
    {
        expiration = new QPushButton(i18n("Change Expiration..."), group);
        password = new QPushButton(i18n("Change Passphrase..."), group);

        connect(expiration, SIGNAL(clicked()), this, SLOT(slotChangeDate()));
        connect(password, SIGNAL(clicked()), this, SLOT(slotChangePass()));
    }
    connect(m_disable, SIGNAL(toggled(bool)), this, SLOT(slotDisableKey(bool)));

    QVBoxLayout *layout = new QVBoxLayout(group);
    layout->setMargin(marginHint());
    layout->setSpacing(spacingHint());
    layout->addWidget(m_disable);

    if (isscretkey)
    {
        layout->addWidget(expiration);
        layout->addWidget(password);
    }

    return group;
}

QGroupBox* KgpgKeyInfo::_fingerprintGroup(QWidget *parent)
{
    QGroupBox *group = new QGroupBox(i18n("Fingerprint"), parent);
    m_fingerprint = new QLabel(group);
    m_fingerprint->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QVBoxLayout *layout = new QVBoxLayout(group);
    layout->setMargin(marginHint());
    layout->setSpacing(spacingHint());
    layout->addWidget(m_fingerprint);

    return group;
}

void KgpgKeyInfo::loadKey()
{
    KgpgInterface *interface = new KgpgInterface();
    KgpgKeyList listkeys = interface->readPublicKeys(true, m_keyid);
    delete interface;

    KgpgKey key = listkeys.at(0);
    KgpgKeySub subkey;

    // Get the first encryption subkey
    KgpgKeySubListPtr sublist = key.subList();
    for (int i = 0; i < sublist->count(); ++i)
    {
        KgpgKeySub temp = sublist->at(i);
        if (temp.type() == SKT_ENCRYPTION)
        {
            subkey = temp;
            break;
        }
    }

    QString name = key.name();
    setCaption(name);
    m_name->setText("<qt><b>" + name + "</b></qt>");

    if (key.email().isEmpty())
    {
        m_email->setText(i18nc("no email address", "none"));
        m_email->setUrl("");
    }
    else
    {
        m_email->setText("<qt><b>&lt;" + key.email() + "&gt;</b></qt>");
        m_email->setUrl("mailto:" + key.email());
    }

    KgpgKeyTrust keytrust = key.valid() ? key.trust() : TRUST_DISABLED;
    QString tr = Convert::toString(keytrust);
    QColor trustcolor = Convert::toColor(keytrust);

    m_id->setText(key.fullId());
    m_algorithm->setText(Convert::toString(key.algorithm()) + " / " + Convert::toString(subkey.algorithm()));
    m_algorithm->setWhatsThis("<qt>The left part is the algorithm used by the <b>signature</b> key. The right part is the algorithm used by the <b>encryption</b> key.</qt>");
    m_creation->setText(key.creation());
    m_expiration->setText(key.expiration());
    m_trust->setText(tr);
    m_trust->setColor(trustcolor);
    m_length->setText(QString::number(key.size()) + " / " + QString::number(subkey.size()));
    m_length->setWhatsThis("<qt>The left part is the size of the <b>signature</b> key. The right part is the size of the <b>encryption</b> key.</qt>");
    m_fingerprint->setText(key.fingerprintBeautified());

    if (key.comment().isEmpty())
        m_comment->setText(i18nc("no key comment", "none"));
    else
        m_comment->setText(key.comment());

    QStringList photolist = key.photoList();
    m_photoid->clear();
    if (photolist.isEmpty())
    {
        m_photoid->setVisible(false);
        m_hasphoto = false;
    }
    else
    {
        m_photoid->setVisible(true);
        m_hasphoto = true;
        m_photoid->addItems(photolist);
    }

    switch (key.ownerTrust())
    {
        case OWTRUST_NONE:
            m_owtrust->setCurrentIndex(1);
            break;

        case OWTRUST_MARGINAL:
            m_owtrust->setCurrentIndex(2);
            break;

        case OWTRUST_FULL:
            m_owtrust->setCurrentIndex(3);
            break;

        case OWTRUST_ULTIMATE:
            m_owtrust->setCurrentIndex(4);
            break;

        case OWTRUST_UNDEFINED:
        default:
            m_owtrust->setCurrentIndex(0);
            break;
    }

    if (!key.valid())
        m_disable->setChecked(true);

    m_isunlimited = key.unlimited();
    m_expirationdate = key.expirationDate();
}

void KgpgKeyInfo::slotOpenUrl(const QString &url) const
{
    KToolInvocation::invokeBrowser(url);
}

void KgpgKeyInfo::slotLoadPhoto(const QString &uid)
{
    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(loadPhotoFinished(QPixmap, KgpgInterface*)), this, SLOT(slotSetPhoto(QPixmap, KgpgInterface*)));
    interface->loadPhoto(m_keyid, uid);
}

void KgpgKeyInfo::slotSetPhoto(const QPixmap &pixmap, KgpgInterface *interface)
{
    delete interface;

    QImage img = pixmap.toImage();
    QPixmap pix = QPixmap::fromImage(img.scaled(m_photo->width(), m_photo->height(), Qt::KeepAspectRatio));
    m_photo->setPixmap(pix);
}

void KgpgKeyInfo::slotPreOk()
{
    if (m_keywaschanged)
        emit keyNeedsRefresh(m_keyid);
    accept();
}

void KgpgKeyInfo::slotChangeDate()
{
    KgpgDateDialog *dialog = new KgpgDateDialog(this, m_isunlimited, m_expirationdate);
    if (dialog->exec() == QDialog::Accepted)
    {
        KgpgInterface *process = new KgpgInterface();
        connect(process, SIGNAL(keyExpireFinished(int, KgpgInterface*)), this, SLOT(slotInfoExpirationChanged(int, KgpgInterface*)));

        if (dialog->unlimited())
            process->keyExpire(m_keyid, QDate());
        else
            process->keyExpire(m_keyid, dialog->date());
    }
    delete dialog;
}

void KgpgKeyInfo::slotInfoExpirationChanged(const int &res, KgpgInterface *interface)
{
    delete interface;

    if (res == 2)
    {
        m_keywaschanged = true;
        loadKey();
    }
    else
    if (res == 1)
        KMessageBox::error(this, i18n("Could not change expiration"), i18n("Bad passphrase. Expiration of the key has not been changed."));
}

void KgpgKeyInfo::slotDisableKey(const bool &ison)
{
    KgpgInterface *interface = new KgpgInterface;
    connect (interface, SIGNAL(changeDisableFinished(KgpgInterface*, int)), this, SLOT(slotDisableKeyFinished(KgpgInterface*, int)));
    interface->changeDisable(m_keyid, ison);
}

void KgpgKeyInfo::slotDisableKeyFinished(KgpgInterface *interface, int)
{
    delete interface;
    loadKey();
    m_keywaschanged = true;
}

void KgpgKeyInfo::slotChangePass()
{
    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(changePassFinished(int, KgpgInterface*)), this, SLOT(slotInfoPasswordChanged(int, KgpgInterface*)));
    interface->changePass(m_keyid);
}

void KgpgKeyInfo::slotInfoPasswordChanged(const int &res, KgpgInterface *interface)
{
    delete interface;

    if (res == 2)
        KPassivePopup::message(i18n("Passphrase for the key was changed"), QString(), Images::kgpg(), this);

    if (res == 1)
        KMessageBox::error(this, i18n("Bad old passphrase, the passphrase for the key was not changed"), i18n("Could not change passphrase"));
}

void KgpgKeyInfo::slotChangeTrust(const int &newtrust)
{
    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(changeTrustFinished(KgpgInterface*)), this, SLOT(slotInfoTrustChanged(KgpgInterface*)));
    interface->changeTrust(m_keyid, KgpgKeyOwnerTrust(newtrust + 1));
}

void KgpgKeyInfo::slotInfoTrustChanged(KgpgInterface *interface)
{
    delete interface;
    m_keywaschanged = true;
    loadKey();
}

#include "keyinfodialog.moc"
