/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2009,2010,2012,2013,2014 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgkeygenerate.h"

#include "core/convert.h"
#include "core/emailvalidator.h"

#include <KComboBox>
#include <KDebug>
#include <KHBox>
#include <KLineEdit>
#include <KLocale>
#include <KMessageBox>
#include <QGroupBox>
#include <QIntValidator>
#include <QLabel>
#include <QVBoxLayout>
#include <QWhatsThis>
#include <QWidget>

using namespace KgpgCore;

KgpgKeyGenerate::KgpgKeyGenerate(QWidget *parent)
	: KDialog(parent),
	m_expert(false)
{
    setupUi(this);

    setButtons(User1 | Ok | Cancel);

    setButtonText(User1, i18n("&Expert Mode"));
    setButtonToolTip(User1, i18n("Go to Expert Mode"));
    setButtonWhatsThis(User1, i18n( "If you go to expert mode, you will use the command line to create your key." ));

    connect(m_kname, SIGNAL(textChanged(QString)), this, SLOT(slotEnableOk()));

    KHBox *hgroup = new KHBox(vgroup);
    hgroup->setFrameShape(QFrame::StyledPanel);
    hgroup->setMargin(marginHint());
    hgroup->setSpacing(spacingHint());
    m_days->setParent(hgroup);
    QIntValidator *validator = new QIntValidator(m_days);
    validator->setBottom(0);
    m_days->setValidator(validator);
    m_days->setMaxLength(4);
    m_days->setDisabled(true);

    m_keyexp = new KComboBox(hgroup);
    m_keyexp->addItem(i18nc("Key will not expire", "Never"), 0);
    m_keyexp->addItem(i18n("Days"), 1);
    m_keyexp->addItem(i18n("Weeks"), 2);
    m_keyexp->addItem(i18n("Months"), 3);
    m_keyexp->addItem(i18n("Years"), 4);
    m_keyexp->setMinimumSize(m_keyexp->sizeHint());
    connect(m_keyexp, SIGNAL(activated(int)), this, SLOT(slotEnableDays(int)));

    qobject_cast<QVBoxLayout *>(vgroup->layout())->insertWidget(7, hgroup);

    m_keysize->addItem(i18n("1024"));
    m_keysize->addItem(i18n("2048"));
    m_keysize->addItem(i18n("4096"));
    m_keysize->setCurrentIndex(1); // 2048
    m_keysize->setMinimumSize(m_keysize->sizeHint());

    m_keykind->addItem(KgpgCore::Convert::toString(KgpgCore::ALGO_DSA_ELGAMAL));
    m_keykind->addItem(KgpgCore::Convert::toString(KgpgCore::ALGO_RSA_RSA));
    m_keykind->addItem(KgpgCore::Convert::toString(KgpgCore::ALGO_RSA));
    m_keykind->setCurrentIndex(1); // RSA+RSA
    slotEnableCaps(m_keykind->currentIndex());
    m_keykind->setMinimumSize(m_keykind->sizeHint());

    setMainWidget(vgroup);

    slotEnableOk();
    updateGeometry();
    show();

    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
    connect(this, SIGNAL(user1Clicked()), this, SLOT(slotUser1()));
    connect(m_keykind, SIGNAL(activated(int)), SLOT(slotEnableCaps(int)));
}

void KgpgKeyGenerate::slotButtonClicked(int button)
{
    if (button == Ok)
        slotOk();
    else
    if (button == User1)
        slotUser1();
    else
    if (button == Cancel)
        reject();
}

void KgpgKeyGenerate::slotOk()
{
    if (m_kname->text().simplified().isEmpty())
    {
        KMessageBox::sorry(this, i18n("You must give a name."));
        return;
    }

    if (m_kname->text().simplified().length() < 5)
    {
        KMessageBox::sorry(this, i18n("The name must have at least 5 characters"));
        return;
    }

    if (m_kname->text().simplified().at(0).isDigit())
    {
        KMessageBox::sorry(this, i18n("The name must not start with a digit"));
        return;
    }

    QString vmail = m_mail->text();
    if (vmail.isEmpty())
    {
        int result = KMessageBox::warningContinueCancel(this, i18n("You are about to create a key with no email address"));
        if (result != KMessageBox::Continue)
            return;
    } else {
        int pos = 0;
        if (EmailValidator().validate(vmail, pos) == QValidator::Invalid)
        {
            KMessageBox::sorry(this, i18n("Email address not valid"));
            return;
        }
    }

    accept();
}

void KgpgKeyGenerate::slotUser1()
{
    m_expert = true;
    accept();
}

void KgpgKeyGenerate::slotEnableDays(const int state)
{
    m_days->setDisabled(state == 0);
}

void KgpgKeyGenerate::slotEnableCaps(const int state)
{
    // currently only supported for standalone RSA keys
    capabilities->setDisabled(state != 2);
}

void KgpgKeyGenerate::slotEnableOk()
{
	enableButtonOk((m_kname->text().simplified().length() >= 5) &&
			!m_kname->text().simplified().at(0).isDigit());
}

bool KgpgKeyGenerate::isExpertMode() const
{
    return m_expert;
}

KgpgCore::KgpgKeyAlgo KgpgKeyGenerate::algo() const
{
	if (m_keykind->currentText() == KgpgCore::Convert::toString(KgpgCore::ALGO_RSA))
		return KgpgCore::ALGO_RSA;
	else if (m_keykind->currentText() == KgpgCore::Convert::toString(KgpgCore::ALGO_RSA_RSA))
		return KgpgCore::ALGO_RSA_RSA;
	else
		return KgpgCore::ALGO_DSA_ELGAMAL;
}

KgpgCore::KgpgSubKeyType KgpgKeyGenerate::caps() const
{
	KgpgCore::KgpgSubKeyType ret;

	if (!capabilities->isEnabled())
		return ret;

	if (capAuth->isChecked())
		ret |= KgpgCore::SKT_AUTHENTICATION;
	if (capCert->isChecked())
		ret |= KgpgCore::SKT_CERTIFICATION;
	if (capEncrypt->isChecked())
		ret |= KgpgCore::SKT_ENCRYPTION;
	if (capSign->isChecked())
		ret |= KgpgCore::SKT_SIGNATURE;

	return ret;
}

uint KgpgKeyGenerate::size() const
{
    return m_keysize->currentText().toUInt();
}

char KgpgKeyGenerate::expiration() const
{
	switch (m_keyexp->currentIndex()) {
	case 1:
		return 'd';
	case 2:
		return 'w';
	case 3:
		return 'm';
	case 4:
	default:
		return 'y';
	}
}

uint KgpgKeyGenerate::days() const
{
    if (m_days->text().isEmpty())
        return 0;
    return m_days->text().toUInt();
}

QString KgpgKeyGenerate::name() const
{
    if (m_kname->text().isEmpty())
        return QString();
    return m_kname->text();
}

QString KgpgKeyGenerate::email() const
{
    if (m_mail->text().isEmpty())
        return QString();
    return m_mail->text();
}

QString KgpgKeyGenerate::comment() const
{
    if (m_comment->text().isEmpty())
        return QString();
    return m_comment->text();
}

#include "kgpgkeygenerate.moc"
