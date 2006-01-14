/***************************************************************************
                          keygen.cpp  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright            : (C) 2002 by Jean-Baptiste Mardelle
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

#include <QVBoxLayout>
#include <QWhatsThis>
#include <QGroupBox>
#include <QWidget>
#include <QLabel>

#include <kmessagebox.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>
#include <khbox.h>

#include "kgpgkeygenerate.h"

KgpgKeyGenerate::KgpgKeyGenerate(QWidget *parent, const char *name)
               : KDialog(parent, i18n("Key Generation"), User1 | Ok | Cancel)
{
    setModal(true);
    setDefaultButton(Cancel);

    setButtonText(User1, i18n("&Expert mode"));
    setButtonTip(User1, i18n("Go to the expert mode"));
    setButtonWhatsThis(User1, "If you go to the expert mode, you will use the command line to create your key.");

    m_expert = false;

    QGroupBox *vgroup = new QGroupBox(i18n("Generate Key Pair"), this);

    QLabel *nameLabel = new QLabel(i18n("&Name:"), vgroup);
    m_kname = new KLineEdit("", vgroup);
    nameLabel->setBuddy(m_kname);
    m_kname->setFocus();
    connect(m_kname, SIGNAL(textChanged(const QString&)), this, SLOT(slotEnableOk()));

    QLabel *emailLabel = new QLabel(i18n("E&mail:"), vgroup);
    m_mail = new KLineEdit("", vgroup);
    emailLabel->setBuddy(m_mail);

    QLabel *commentLabel = new QLabel(i18n("Commen&t (optional):"), vgroup);
    m_comment = new KLineEdit("", vgroup);
    commentLabel->setBuddy(m_comment);

    QLabel *expLabel = new QLabel(i18n("Expiration:"), vgroup);
    KHBox *hgroup = new KHBox(vgroup);
    hgroup->setFrameShape(QFrame::StyledPanel);
    hgroup->setMargin(marginHint());
    hgroup->setSpacing(spacingHint());
    m_numb = new KLineEdit("0", hgroup);
    m_numb->setMaxLength(4);
    m_numb->setDisabled(true);

    m_keyexp = new KComboBox(hgroup);
    m_keyexp->insertItem(i18n("Never"), 0);
    m_keyexp->insertItem(i18n("Days"), 1);
    m_keyexp->insertItem(i18n("Weeks"), 2);
    m_keyexp->insertItem(i18n("Months"), 3);
    m_keyexp->insertItem(i18n("Years"), 4);
    m_keyexp->setMinimumSize(m_keyexp->sizeHint());
    connect(m_keyexp, SIGNAL(activated(int)), this, SLOT(slotActivateExp(int)));

    QLabel *sizeLabel = new QLabel(i18n("&Key size:"), vgroup);
    m_keysize = new KComboBox(vgroup);
    m_keysize->insertItem("768");
    m_keysize->insertItem("1024");
    m_keysize->insertItem("2048");
    m_keysize->insertItem("4096");
    m_keysize->setCurrentItem("1024");
    m_keysize->setMinimumSize(m_keysize->sizeHint());
    sizeLabel->setBuddy(m_keysize);

    QLabel *algoLabel = new QLabel(i18n("&Algorithm:"), vgroup);
    m_keykind = new KComboBox(vgroup);
    m_keykind->insertItem("DSA & ElGamal");
    m_keykind->insertItem("RSA");
    m_keykind->setMinimumSize(m_keykind->sizeHint());
    algoLabel->setBuddy(m_keykind);

    QVBoxLayout *vlayout = new QVBoxLayout(vgroup);
    vlayout->addWidget(nameLabel);
    vlayout->addWidget(m_kname);
    vlayout->addWidget(emailLabel);
    vlayout->addWidget(m_mail);
    vlayout->addWidget(commentLabel);
    vlayout->addWidget(m_comment);
    vlayout->addWidget(expLabel);
    vlayout->addWidget(hgroup);
    vlayout->addWidget(sizeLabel);
    vlayout->addWidget(m_keysize);
    vlayout->addWidget(algoLabel);
    vlayout->addWidget(m_keykind);
    vlayout->addStretch();
    vgroup->setLayout(vlayout);

    setMainWidget(vgroup);

    slotEnableOk();
    updateGeometry();
    show();
}

void KgpgKeyGenerate::slotOk()
{
    if (QString(m_kname->text()).simplified().isEmpty())
    {
        KMessageBox::sorry(0, i18n("You must give a name."));
        return;
    }

    QString vmail = m_mail->text();
    if (vmail.isEmpty())
    {
        int result;
        result = KMessageBox::warningContinueCancel(this, i18n("You are about to create a key with no email address"));
        if (result != KMessageBox::Continue)
            return;
    }
    else
    if ((vmail.find(" ") != -1) || (vmail.find(".") == -1) || (vmail.find("@") == -1))
    {
        KMessageBox::sorry(0, i18n("Email address not valid"));
        return;
    }

    accept();
}

void KgpgKeyGenerate::slotUser1()
{
    m_expert = true;
    accept();
}

void KgpgKeyGenerate::slotActivateExp(const int &state)
{
    if (state == 0)
        m_numb->setDisabled(true);
    else
        m_numb->setDisabled(false);
}

void KgpgKeyGenerate::slotEnableOk()
{
    enableButtonOK(!QString(m_kname->text()).simplified().isEmpty());
}

bool KgpgKeyGenerate::getMode() const
{
    return m_expert;
}

Kgpg::KeyAlgo KgpgKeyGenerate::getKeyAlgo() const
{
    if (m_keykind->currentText() == "RSA")
        return Kgpg::RSA;
    else
        return Kgpg::DSA_ELGAMAL;
}

uint KgpgKeyGenerate::getKeySize() const
{
    return m_keysize->currentText().toUInt();
}

uint KgpgKeyGenerate::getKeyExp() const
{
    return m_keyexp->currentItem();
}

uint KgpgKeyGenerate::getKeyNumber() const
{
    if (m_numb->text() != QString::null)
        return m_numb->text().toUInt();
    else
        return 0;
}

QString KgpgKeyGenerate::getKeyName() const
{
    if (m_kname->text() != QString::null)
        return(m_kname->text());
    else
        return QString();
}

QString KgpgKeyGenerate::getKeyEmail() const
{
    if (m_mail->text() != QString::null)
        return(m_mail->text());
    else
        return QString();
}

QString KgpgKeyGenerate::getKeyComment() const
{
    if (m_comment->text() != QString::null)
        return(m_comment->text());
    else
        return QString();
}

#include "kgpgkeygenerate.moc"
