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

#include "keygener.h"

KgpgKeyGenerate::KgpgKeyGenerate(QWidget *parent, const char *name)
               : KDialogBase(Plain, i18n("Key Generation"), User1 | Ok | Cancel, Ok, parent, name, true)
{
    m_expert = false;

    setButtonText(User1, i18n("&Expert mode"));
    setButtonTip(User1, i18n("Go to the expert mode"));
    setButtonWhatsThis(User1, "If you go to the expert mode, you will use the command line to create your key.");

    QFrame *page = plainPage();
    QGroupBox *vgroup = new QGroupBox(i18n("Generate Key Pair"), page);

    QLabel *nameLabel = new QLabel(i18n("&Name:"), vgroup);
    m_kname = new KLineEdit("", vgroup);
    nameLabel->setBuddy(m_kname);
    m_kname->setFocus();
    connect(m_kname, SIGNAL(textChanged(const QString&)), this, SLOT(slotActivateOk()));

    QLabel *emailLabel = new QLabel(i18n("E&mail:"), vgroup);
    m_mail = new KLineEdit("", vgroup);
    emailLabel->setBuddy(m_mail);

    QLabel *commentLabel = new QLabel(i18n("Commen&t (optional):"), vgroup);
    m_comment = new KLineEdit("", vgroup);
    commentLabel->setBuddy(m_comment);

    QLabel *expLabel = new QLabel(i18n("Expiration:"), vgroup);
    KHBox *hgroup = new KHBox(vgroup);
    hgroup->setFrameShape(QFrame::StyledPanel);
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
    vlayout->setSpacing(spacingHint());
    vlayout->setMargin(marginHint());
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
    vgroup->setLayout(vlayout);

    QVBoxLayout *vbox = new QVBoxLayout(page);
    vbox->setSpacing(0);
    vbox->setMargin(0);
    vbox->addWidget(vgroup);
    page->setLayout(vbox);

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

bool KgpgKeyGenerate::getmode() const
{
    return m_expert;
}

QString KgpgKeyGenerate::getkeytype() const
{
    return m_keykind->currentText();
}

QString KgpgKeyGenerate::getkeysize() const
{
    return m_keysize->currentText();
}

int KgpgKeyGenerate::getkeyexp() const
{
    return m_keyexp->currentItem();
}

QString KgpgKeyGenerate::getkeynumb() const
{
    if (m_numb->text() != QString::null)
        return m_numb->text();
    else
        return QString();
}

QString KgpgKeyGenerate::getkeyname() const
{
    if (m_kname->text() != QString::null)
        return(m_kname->text());
    else
        return QString();
}

QString KgpgKeyGenerate::getkeymail() const
{
    if (m_mail->text() != QString::null)
        return(m_mail->text());
    else
        return QString();
}

QString KgpgKeyGenerate::getkeycomm() const
{
    if (m_comment->text() != QString::null)
        return(m_comment->text());
    else
        return QString();
}

#include "keygener.moc"
