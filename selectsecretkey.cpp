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
#include <k3listview.h>
#include <kcombobox.h>
#include <klocale.h>

#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "selectsecretkey.h"

KgpgSelectSecretKey::KgpgSelectSecretKey(QWidget *parent, const bool &signkey, const int &countkey)
                   : KDialog(parent, i18n("Private Key List"), Ok | Cancel)
{
    QWidget *page = new QWidget(this);

    KIconLoader *loader = KGlobal::iconLoader();
    QPixmap keyPair = loader->loadIcon("kgpg_key2", K3Icon::Small, 20);

    QLabel *label = new QLabel(i18n("Choose secret key for signing:"), page);

    m_keyslist = new K3ListView(page);
    m_keyslist->addColumn(i18n("Name"), 200);
    m_keyslist->addColumn(i18n("Email"), 200);
    m_keyslist->addColumn(i18n("ID"), 100);
    m_keyslist->setFullWidth(true);
    m_keyslist->setRootIsDecorated(true);
    m_keyslist->setShowSortIndicator(true);
    m_keyslist->setAllColumnsShowFocus(true);

    QVBoxLayout *vbox = new QVBoxLayout(page);
    vbox->addWidget(label);
    vbox->addWidget(m_keyslist);

    m_signkey = signkey;
    m_localsign = 0;
    m_terminalsign = 0;
    m_signtrust = 0;
    if (m_signkey)
    {
        QLabel *signchecklabel = new QLabel("<qt>" + i18np("How carefully have you checked that the key really "
                                           "belongs to the person with whom you wish to communicate:",
                                           "How carefully have you checked that the %n keys really "
                                           "belong to the people with whom you wish to communicate:", countkey), page);

        m_signtrust = new KComboBox(page);
        m_signtrust->addItem(i18n("I Will Not Answer"));
        m_signtrust->addItem(i18n("I Have Not Checked at All"));
        m_signtrust->addItem(i18n("I Have Done Casual Checking"));
        m_signtrust->addItem(i18n("I Have Done Very Careful Checking"));

        m_localsign = new QCheckBox(i18n("Local signature (cannot be exported)"), page);
        m_terminalsign = new QCheckBox(i18n("Do not sign all user id's (open terminal)"), page);

        vbox->addWidget(signchecklabel);
        vbox->addWidget(m_signtrust);
        vbox->addWidget(m_localsign);
        vbox->addWidget(m_terminalsign);

        if (countkey != 1)
            m_terminalsign->setEnabled(false);
    }

    QString defaultKeyID = KGpgSettings::defaultKey().right(8);
    QString fullname;
    QString line;

    bool selectedok = false;
    KgpgInterface *interface = new KgpgInterface();
    KgpgListKeys list1 = interface->readSecretKeys(true);

    for (int i = 0; i < list1.size(); ++i)
    {
        KgpgKey key = list1.at(i);
        QString id = key.id();

        bool dead = true;

        /* Public key */
        KgpgListKeys list2 = interface->readPublicKeys(true, QStringList(id));
        KgpgKey key2 = list2.at(0);

        if (key2.trust() == 'f' || key2.trust() == 'u')
            dead = false;

        if (!key2.valide())
            dead = true;
        /**************/

        if (!dead && !(key.name().isEmpty()))
        {
            QString keyName = key.name();

            keyName = KgpgInterface::checkForUtf8(keyName);
            Q3ListViewItem *item = new Q3ListViewItem(m_keyslist, keyName, key.email(), id);
            Q3ListViewItem *sub = new Q3ListViewItem(item, i18n("Expiration:"), key.expiration());
            sub->setSelectable(false);
            item->setPixmap(0, keyPair);
            if (!defaultKeyID.isEmpty() && id == defaultKeyID)
            {
                m_keyslist->setSelected(item, true);
                selectedok = true;
            }
        }
    }
    delete interface;

    if (!selectedok)
        m_keyslist->setSelected(m_keyslist->firstChild(), true);

    setMinimumSize(550, 200);
    slotSelectionChanged();
    setMainWidget(page);

    connect(m_keyslist, SIGNAL(doubleClicked(Q3ListViewItem *, const QPoint &, int)), this, SLOT(slotOk(Q3ListViewItem *)));
    connect(m_keyslist, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()));
}

QString KgpgSelectSecretKey::getKeyID() const
{
    if (m_keyslist->currentItem() == 0)
        return QString::null;

    return m_keyslist->currentItem()->text(2);
}

QString KgpgSelectSecretKey::getKeyMail() const
{
    if (m_keyslist->currentItem() == 0)
        return QString::null;

    QString username;
    username = m_keyslist->currentItem()->text(0);
    username = username.simplified();
    return username;
}

int KgpgSelectSecretKey::getSignTrust() const
{
    if (m_signkey)
        return m_signtrust->currentIndex();
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

void KgpgSelectSecretKey::slotSelectionChanged()
{
    enableButtonOk(!m_keyslist->selectedItems().isEmpty());
}

void KgpgSelectSecretKey::slotOk(Q3ListViewItem *item)
{
    if (item != 0 && item->depth() == 0)
        slotButtonClicked(Ok);
}

#include "selectsecretkey.moc"
