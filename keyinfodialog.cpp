/**
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007 Jimmy Gilles <jimmygilles@gmail.com>
 * Copyright (C) 2008 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

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
#include <QGroupBox>
#include <QCheckBox>
#include <QPixmap>
#include <QImage>
#include <QApplication>

#include <KToolInvocation>
#include <KPassivePopup>
#include <KPushButton>
#include <KDatePicker>
#include <KMessageBox>
#include <KUrlLabel>
#include <KComboBox>
#include <KLocale>

#include "kgpginterface.h"
#include "convert.h"
#include "images.h"
#include "kgpgchangekey.h"
#include "kgpgchangepass.h"
#include "kgpgitemnode.h"

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

KgpgDateDialog::KgpgDateDialog(QWidget *parent, QDate date)
              : KDialog(parent)
{
    setCaption(i18n("Choose New Expiration"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);
    setModal(true);

    QWidget *page = new QWidget(this);
    m_unlimited = new QCheckBox(i18nc("Key has unlimited lifetime", "Unlimited"), page);

    if (date.isNull())
        date = QDate::currentDate();

    m_datepicker = new KDatePicker(date, page);
    if (date.isNull()) {
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
    if (m_unlimited->isChecked())
        return QDate();
    else
        return m_datepicker->date();
}

void KgpgDateDialog::slotCheckDate(const QDate &date)
{
    enableButtonOk(date >= QDate::currentDate());
}

void KgpgDateDialog::slotEnableDate(const bool &ison)
{
    m_datepicker->setEnabled(!ison);
    if (ison)
        enableButtonOk(true);
    else
        slotCheckDate(m_datepicker->date());
}

KgpgKeyInfo::KgpgKeyInfo(KGpgKeyNode *node, QWidget *parent)
           : KDialog(parent), keychange(new KGpgChangeKey(node))
{
    m_node = node;
    m_key = node->getKey();
    init();
}

KgpgKeyInfo::KgpgKeyInfo(KgpgCore::KgpgKey *key, QWidget *parent)
           : KDialog(parent), keychange(new KGpgChangeKey(key))
{
    m_node = NULL;
    m_key = key;
    init();
}

void KgpgKeyInfo::init()
{
    setButtons(Ok | Apply | Cancel);
    setDefaultButton(Ok);
    setModal(true);
    enableButtonApply(false);

    m_keywaschanged = false;

    m_changepass = NULL;

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
    connect(this, SIGNAL(okClicked()), this, SLOT(slotPreOk()));
    connect(this, SIGNAL(cancelClicked()), this, SLOT(slotPreCancel()));
    connect(this, SIGNAL(applyClicked()), SLOT(slotApply()));
    connect(keychange, SIGNAL(done(int)), SLOT(slotApplied(int)));

    displayKey();
    if (!m_hasphoto)
        m_photoid->setEnabled(false);
    else
        slotLoadPhoto(m_photoid->currentText());
}

KgpgKeyInfo::~KgpgKeyInfo()
{
	if (keychange)
		keychange->selfdestruct(false);
	if (!m_node)
		delete m_key;
	delete m_changepass;
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

    m_owtrust->addItem(i18n("I do not know"));
    m_owtrust->addItem(i18n("I do NOT trust"));
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
    QGroupBox *group = new QGroupBox(parent);
    m_disable = new QCheckBox(i18n("Disable key"), group);

    if (m_key->secret())
    {
        m_expirationbtn = new KPushButton(i18n("Change Expiration..."), group);
        m_password = new KPushButton(i18n("Change Passphrase..."), group);

        connect(m_expirationbtn, SIGNAL(clicked()), this, SLOT(slotChangeDate()));
        connect(m_password, SIGNAL(clicked()), this, SLOT(slotChangePass()));
    }
    else
    {
        m_password = 0;
        m_expirationbtn = 0;
    }

    connect(m_disable, SIGNAL(toggled(bool)), this, SLOT(slotDisableKey(bool)));

    QVBoxLayout *layout = new QVBoxLayout(group);
    layout->setMargin(marginHint());
    layout->setSpacing(spacingHint());
    layout->addWidget(m_disable);

    if (m_key->secret())
    {
        layout->addWidget(m_expirationbtn);
        layout->addWidget(m_password);
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

void KgpgKeyInfo::reloadKey()
{
	KgpgInterface *interface = new KgpgInterface();
	KgpgKeyList listkeys = interface->readPublicKeys(true, m_key->fullId());
	delete interface;

	Q_ASSERT(listkeys.count() > 0);

	delete m_key;
	m_key = new KgpgKey(listkeys.at(0));
	displayKey();
}

void KgpgKeyInfo::displayKey()
{
    KgpgKeySub subkey;

    // Get the first encryption subkey
    KgpgKeySubListPtr sublist = m_key->subList();
    for (int i = 0; i < sublist->count(); ++i)
    {
        KgpgKeySub temp = sublist->at(i);
        if (temp.type() == SKT_ENCRYPTION)
        {
            subkey = temp;
            break;
        }
    }

    QString name = m_key->name();
    setCaption(name);
    m_name->setText("<qt><b>" + name + "</b></qt>");

    if (m_key->email().isEmpty())
    {
        m_email->setText(i18nc("no email address", "none"));
        m_email->setUrl("");
    }
    else
    {
        m_email->setText("<qt><b>&lt;" + m_key->email() + "&gt;</b></qt>");
        m_email->setUrl("mailto:" + name + '<' + m_key->email() + '>');
    }

    KgpgKeyTrust keytrust = m_key->valid() ? m_key->trust() : TRUST_DISABLED;
    QString tr = Convert::toString(keytrust);
    QColor trustcolor = Convert::toColor(keytrust);

    m_id->setText(m_key->fullId());
    m_algorithm->setText(Convert::toString(m_key->algorithm()) + " / " + Convert::toString(subkey.algorithm()));
    m_algorithm->setWhatsThis("<qt>The left part is the algorithm used by the <b>signature</b> key. The right part is the algorithm used by the <b>encryption</b> key.</qt>");
    m_creation->setText(m_key->creation());
    m_expiration->setText(m_key->expiration());
    m_trust->setText(tr);
    m_trust->setColor(trustcolor);
    m_length->setText(QString::number(m_key->size()) + " / " + QString::number(subkey.size()));
    m_length->setWhatsThis("<qt>The left part is the size of the <b>signature</b> key. The right part is the size of the <b>encryption</b> key.</qt>");
    m_fingerprint->setText(m_key->fingerprintBeautified());

    if (m_key->comment().isEmpty())
        m_comment->setText(i18nc("no key comment", "none"));
    else
        m_comment->setText(m_key->comment());

    QStringList photolist = m_key->photoList();
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

    switch (m_key->ownerTrust())
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

    if (!m_key->valid())
        m_disable->setChecked(true);
}

void KgpgKeyInfo::slotOpenUrl(const QString &url) const
{
    KToolInvocation::invokeBrowser(url);
}

void KgpgKeyInfo::slotLoadPhoto(const QString &uid)
{
    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(loadPhotoFinished(QPixmap, KgpgInterface*)), this, SLOT(slotSetPhoto(QPixmap, KgpgInterface*)));
    interface->loadPhoto(m_key->fullId(), uid);
}

void KgpgKeyInfo::slotSetPhoto(const QPixmap &pixmap, KgpgInterface *interface)
{
    interface->deleteLater();

    QImage img = pixmap.toImage();
    QPixmap pix = QPixmap::fromImage(img.scaled(m_photo->width(), m_photo->height(), Qt::KeepAspectRatio));
    m_photo->setPixmap(pix);
}

void KgpgKeyInfo::slotPreOk()
{
	if (m_keywaschanged && m_node)
		emit keyNeedsRefresh(m_node);
	keychange->selfdestruct(true);
	keychange = NULL;
	accept();
}

void KgpgKeyInfo::slotChangeDate()
{
	KgpgDateDialog *dialog = new KgpgDateDialog(this, m_key->expirationDate());
	if (dialog->exec() == QDialog::Accepted) {
		keychange->setExpiration(dialog->date());
		enableButtonApply(keychange->wasChanged());
	}
	delete dialog;
}

void KgpgKeyInfo::slotDisableKey(const bool &ison)
{
	keychange->setDisable(ison);
	enableButtonApply(keychange->wasChanged());
}

void KgpgKeyInfo::slotChangePass()
{
	if (m_changepass == NULL) {
		m_changepass = new KGpgChangePass(this, m_key->fingerprint());

		connect(m_changepass, SIGNAL(done(int)), SLOT(slotInfoPasswordChanged(int)));
	}

	m_changepass->start();
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
}

void KgpgKeyInfo::slotInfoPasswordChanged(const int &res)
{
    if (res == 2)
        KPassivePopup::message(i18n("Passphrase for the key was changed"), QString(), Images::kgpg(), this);
    else if (res == 1)
        KMessageBox::error(this, i18n("Bad old passphrase, the passphrase for the key was not changed"), i18n("Could not change passphrase"));
    QApplication::restoreOverrideCursor();
}

void KgpgKeyInfo::slotChangeTrust(const int &newtrust)
{
	keychange->setOwTrust(KgpgKeyOwnerTrust(newtrust + 1));
	enableButtonApply(keychange->wasChanged());
}

void KgpgKeyInfo::setControlEnable(const bool &b)
{
    m_owtrust->setEnabled(b);
    m_disable->setEnabled(b);
    enableButtonApply(b && keychange->wasChanged());

    if (m_expirationbtn)
        m_expirationbtn->setEnabled(b);
    if (m_password)
        m_password->setEnabled(b);

    if (b)
        QApplication::restoreOverrideCursor();
    else
        QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
}

void KgpgKeyInfo::slotApply()
{
	setControlEnable(false);
	keychange->apply();
}

void KgpgKeyInfo::slotApplied(int result)
{
	if (result) {
		KMessageBox::error(this, i18n("Changing key properties failed."), i18n("Key properties"));
	} else {
		m_keywaschanged = true;
		reloadKey();
	}
	setControlEnable(true);
}

void KgpgKeyInfo::slotPreCancel()
{
	if (m_keywaschanged && m_node)
		emit keyNeedsRefresh(m_node);
	reject();
}

#include "keyinfodialog.moc"
