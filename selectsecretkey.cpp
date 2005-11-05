/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>

#include <kiconloader.h>
#include <klistview.h>
#include <kcombobox.h>
#include <klocale.h>

#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "selectsecretkey.h"

KgpgSelectSecretKey::KgpgSelectSecretKey(QWidget *parent, const char *name, const bool &signkey, const int &countkey)
                   : KDialogBase(parent, name, true, i18n("Private Key List"), Ok | Cancel)
{
    QString keyname;

    QWidget *page = new QWidget(this);
    QLabel *labeltxt;
    KIconLoader *loader = KGlobal::iconLoader();

    QPixmap keyPair = loader->loadIcon("kgpg_key2", KIcon::Small, 20);

    setMinimumSize(550, 200);
    m_keyslistpr = new KListView(page);
    m_keyslistpr->setRootIsDecorated(true);
    m_keyslistpr->addColumn(i18n("Name"), 200);
    m_keyslistpr->addColumn(i18n("Email"), 200);
    m_keyslistpr->addColumn(i18n("ID"), 100);
    m_keyslistpr->setShowSortIndicator(true);
    m_keyslistpr->setFullWidth(true);
    m_keyslistpr->setAllColumnsShowFocus(true);

    labeltxt = new QLabel(i18n("Choose secret key for signing:"), page);
    QVBoxLayout *vbox = new QVBoxLayout(page);

    vbox->addWidget(labeltxt);
    vbox->addWidget(m_keyslistpr);

    m_signkey = false;
    if (signkey)
    {
        m_signkey = true;
        QLabel *signchecklabel = new QLabel("<qt>" + i18n("How carefully have you checked that the key really "
                                           "belongs to the person with whom you wish to communicate:",
                                           "How carefully have you checked that the %1 keys really "
                                           "belong to the people with whom you wish to communicate:", countkey), page);

        vbox->addWidget(signchecklabel);

        m_signtrust = new KComboBox(page);
        m_signtrust->insertItem(i18n("I Will Not Answer"));
        m_signtrust->insertItem(i18n("I Have Not Checked at All"));
        m_signtrust->insertItem(i18n("I Have Done Casual Checking"));
        m_signtrust->insertItem(i18n("I Have Done Very Careful Checking"));
        vbox->addWidget(m_signtrust);

        m_localsign = new QCheckBox(i18n("Local signature (cannot be exported)"), page);
        vbox->addWidget(m_localsign);

        QCheckBox *m_terminalsign = new QCheckBox(i18n("Do not sign all user id's (open terminal)"), page);
        vbox->addWidget(m_terminalsign);
        if (countkey != 1)
            m_terminalsign->setEnabled(false);
    }

    QString defaultKeyID = KGpgSettings::defaultKey().right(8);

    QString fullname;
    QString line;

    bool selectedok = false;
    KgpgInterface *inter1 = new KgpgInterface();
    KgpgListKeys list1 = inter1->readSecretKeys(true);
    delete inter1;

    for (int i = 0; i < list1.count(); ++i)
    {
        KgpgKeyPtr keyptr = list1.at(i);
        QString id = keyptr->gpgkeyid;

        QString val;
        if (keyptr->gpgkeyunlimited)
            val = i18n("Unlimited");
        else
            val = KGlobal::locale()->formatDate(keyptr->gpgkeyexpiration);

        bool dead = true;
        QString trust;

        /* Public key */
        KgpgInterface *inter2 = new KgpgInterface();
        KgpgListKeys list2 = inter2->readPublicKeys(true, QStringList(id));
        delete inter2;
        KgpgKeyPtr keyptr2 = list2.at(0);

        trust = KgpgKey::trust(keyptr2->gpgkeytrust);
        if ((keyptr2->gpgkeytrust == 'f') || (keyptr2->gpgkeytrust == 'u'))
            dead = false;

        if (keyptr2->gpgkeyvalide == false)
            dead = true;
        /**************/

        if (!dead && !(keyptr->gpgkeyname.isEmpty()))
        {
            QString keyMail = keyptr->gpgkeymail;
            QString keyName = keyptr->gpgkeyname;

            keyName = KgpgInterface::checkForUtf8(keyName);
            Q3ListViewItem *item = new Q3ListViewItem(m_keyslistpr, keyName, keyMail, id);
            //Q3ListViewItem *sub= new Q3ListViewItem(item,i18n("ID: %1, trust: %2, expiration: %3").arg(id).arg(trust).arg(val));
            Q3ListViewItem *sub = new Q3ListViewItem(item, i18n("Expiration:"), val);
            sub->setSelectable(false);
            item->setPixmap(0, keyPair);
            if ((!defaultKeyID.isEmpty()) && (keyptr->gpgkeyid == defaultKeyID))
            {
                m_keyslistpr->setSelected(item, true);
                selectedok = true;
            }
        }
    }

    connect(m_keyslistpr, SIGNAL(doubleClicked(Q3ListViewItem *, const QPoint &, int)), this,SLOT(slotOk()));
    connect(m_keyslistpr, SIGNAL(clicked(Q3ListViewItem *)), this, SLOT(slotSelect(Q3ListViewItem *)));
    connect(m_keyslistpr, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));

    if (!selectedok)
        m_keyslistpr->setSelected(m_keyslistpr->firstChild(), true);

    selectionChanged();
    setMainWidget(page);
}

QString KgpgSelectSecretKey::getKeyID() const
{
    if (m_keyslistpr->currentItem() == 0)
        return(QString::null);

    return(m_keyslistpr->currentItem()->text(2));
}

QString KgpgSelectSecretKey::getKeyMail() const
{
    if (m_keyslistpr->currentItem() == 0)
        return(QString::null);
    else
    {
        QString username;
        username = m_keyslistpr->currentItem()->text(0);
        username = username.simplified();
        return username;
    }
}

int KgpgSelectSecretKey::getSignTrust() const
{
    if (m_signkey)
        return m_signtrust->currentItem();
    return -1;
}

bool KgpgSelectSecretKey::isLocalSign() const
{
    if (m_signkey)
        return m_localsign->isChecked();
    return false;
}

bool KgpgSelectSecretKey::isTerminalSign() const
{
    if (m_signkey)
        return m_terminalsign->isChecked();
    return false;
}

void KgpgSelectSecretKey::slotOk()
{
    if (m_keyslistpr->currentItem() == 0)
        reject();
    else
        accept();
}

void KgpgSelectSecretKey::slotSelect(Q3ListViewItem *item)
{
    if (item == 0)
        return;

    if (item->depth() != 0)
    {
        m_keyslistpr->setSelected(item->parent(), true);
        m_keyslistpr->setCurrentItem(item->parent());
    }
}

void KgpgSelectSecretKey::selectionChanged()
{
    enableButtonOK(!m_keyslistpr->selectedItems().isEmpty());
}

#include "selectsecretkey.moc"
