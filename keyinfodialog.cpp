/*
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

#include "kgpgkey.h"
#include "convert.h"
#include "images.h"
#include "kgpgchangekey.h"
#include "kgpgchangepass.h"
#include "kgpgitemnode.h"
#include "kgpgitemmodel.h"
#include "selectexpirydate.h"

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

KgpgKeyInfo::KgpgKeyInfo(KGpgKeyNode *node, KGpgItemModel *model, QWidget *parent)
	: KDialog(parent),
	keychange(new KGpgChangeKey(node)),
	m_key(node->getKey()),
	m_node(node),
	m_model(model),
	m_changepass(NULL),
	m_keywaschanged(false)
{
	Q_ASSERT(m_model != NULL);
	Q_ASSERT(m_node != NULL);

    setButtons(Ok | Apply | Cancel);
    setDefaultButton(Ok);
    setModal(true);
    enableButtonApply(false);

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

    connect(m_node, SIGNAL(expanded()), SLOT(slotKeyExpanded()));
    m_node->expand();
}

KgpgKeyInfo::~KgpgKeyInfo()
{
	if (keychange)
		keychange->selfdestruct(false);
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
    m_photo->setWhatsThis(i18n("<qt><b>Photo:</b><p>A photo can be included with a public key for extra security. The photo can be used as an additional method of authenticating the key. However, it should not be relied upon as the only form of authentication.</p></qt>"));

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

void KgpgKeyInfo::reloadNode()
{
	const QString kid(m_node->getId());

	// this will delete m_node
	m_key = NULL;
	m_model->refreshKey(m_node);

	m_node = m_model->getRootNode()->findKey(kid);
	if (m_node != NULL) {
		m_key = m_node->getKey();
		displayKey();
	} else {
		KMessageBox::error(this, i18n("<qt>The requested key is not present in the keyring anymore.<br />Perhaps it was deleted by another application</qt>"), i18n("Key not found"));
		m_keywaschanged = false;
		close();
	}
}

void KgpgKeyInfo::displayKey()
{
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
    m_algorithm->setText(Convert::toString(m_key->algorithm()) + " / " + Convert::toString(m_key->encryptionAlgorithm()));
    m_algorithm->setWhatsThis(i18n("<qt>The left part is the algorithm used by the <b>signature</b> key. The right part is the algorithm used by the <b>encryption</b> key.</qt>"));
    m_creation->setText(m_key->creation());
    m_expiration->setText(m_key->expiration());
    m_trust->setText(tr);
    m_trust->setColor(trustcolor);
    m_length->setText(QString::number(m_key->size()) + " / " + QString::number(m_key->encryptionSize()));
    m_length->setWhatsThis(i18n("<qt>The left part is the size of the <b>signature</b> key. The right part is the size of the <b>encryption</b> key.</qt>"));
    m_fingerprint->setText(m_key->fingerprintBeautified());

    if (m_key->comment().isEmpty())
        m_comment->setText(i18nc("no key comment", "none"));
    else
        m_comment->setText(m_key->comment());

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
	int i = uid.toInt();
	QPixmap pixmap = m_node->getUid(i)->toUatNode()->getPixmap();
	QImage img = pixmap.toImage();
	pixmap = QPixmap::fromImage(img.scaled(m_photo->width(), m_photo->height(), Qt::KeepAspectRatio));
	m_photo->setPixmap(pixmap);
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
	QPointer<SelectExpiryDate> dialog = new SelectExpiryDate(this, m_key->expirationDate());
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

void KgpgKeyInfo::slotInfoPasswordChanged(int result)
{
	switch (result) {
	case KGpgTransaction::TS_OK:
		KPassivePopup::message(i18n("Passphrase for the key was changed"), QString(), Images::kgpg(), this);
		break;
	case KGpgTransaction::TS_BAD_PASSPHRASE:
		KMessageBox::error(this, i18n("Bad old passphrase, the passphrase for the key was not changed"), i18n("Could not change passphrase"));
		break;
	case KGpgTransaction::TS_USER_ABORTED:
		break;
	default:
		KMessageBox::error(this, i18n("KGpg was unable to change the passphrase.<br />Please see the detailed log for more information."));
	}

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
		reloadNode();
	}
	setControlEnable(true);
}

void KgpgKeyInfo::slotPreCancel()
{
	if (m_keywaschanged && m_node)
		emit keyNeedsRefresh(m_node);
	reject();
}

void KgpgKeyInfo::slotKeyExpanded()
{
	// the counting starts at 1 and that is the primary uid which can't be a photo id
	int i = 2;
	const KGpgSignableNode *uat;

	m_photoid->clear();

	while ((uat = m_node->getUid(i++)) != NULL) {
		if (uat->getType() != KgpgCore::ITYPE_UAT)
			continue;

		m_photoid->addItem(uat->getId());
	}

	bool hasphoto = (m_photoid->count() > 0);

	m_photoid->setVisible(hasphoto);
	m_photoid->setEnabled(hasphoto);
	if (hasphoto)
		slotLoadPhoto(m_photoid->currentText());
}

#include "keyinfodialog.moc"
