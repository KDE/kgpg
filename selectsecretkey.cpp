/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "selectsecretkey.h"

#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QTableView>

#include <KComboBox>
#include <KLocale>

#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "images.h"
#include "selectkeyproxymodel.h"
#include "kgpgitemmodel.h"

using namespace KgpgCore;

KgpgSelectSecretKey::KgpgSelectSecretKey(QWidget *parent, const bool &signkey, const int &countkey, KGpgItemModel *model)
                   : KDialog(parent)
{
    setCaption(i18n("Private Key List"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);
    QWidget *page = new QWidget(this);

    QLabel *label = new QLabel(i18n("Choose secret key for signing:"), page);

    KGpgItemModel *md;
    if (model == NULL) {
       md = new KGpgItemModel(this);
       md->refreshKeys();
    } else
       md = model;

    m_proxy = new SelectSecretKeyProxyModel(this);
    m_proxy->setKeyModel(md);

    m_keyslist = new QTableView(page);
    m_keyslist->setModel(m_proxy);
    m_keyslist->setSortingEnabled(true);
    m_keyslist->setSelectionBehavior(QAbstractItemView::SelectRows);
    for (int i = 0; i < 4; i++)
       m_keyslist->resizeColumnToContents(i);

    QVBoxLayout *vbox = new QVBoxLayout(page);
    vbox->addWidget(label);
    vbox->addWidget(m_keyslist);

    m_localsign = 0;        // must be set to 0 if signkey is false
    m_terminalsign = 0;     // must be set to 0 if signkey is false
    m_signtrust = 0;        // must be set to 0 if signkey is false
    if (signkey)
    {
        QLabel *signchecklabel = new QLabel(i18np("How carefully have you checked that the key really "
                                           "belongs to the person with whom you wish to communicate:",
                                           "How carefully have you checked that the %1 keys really "
                                           "belong to the people with whom you wish to communicate:", countkey), page);
        signchecklabel->setWordWrap(true);

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

	KGpgNode *nd = md->getRootNode()->findKey(KGpgSettings::defaultKey());
	if (nd != NULL) {
		QModelIndex sidx = md->nodeIndex(nd);
		QModelIndex pidx = m_proxy->mapFromSource(sidx);
		m_keyslist->selectionModel()->setCurrentIndex(pidx, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
	}

    setMinimumSize(550, 200);
    slotSelectionChanged();
    setMainWidget(page);

    connect(m_keyslist->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(slotSelectionChanged()));
    connect(m_keyslist, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotOk()));
}

KgpgSelectSecretKey::~KgpgSelectSecretKey()
{
}

QString KgpgSelectSecretKey::getKeyID() const
{
    if (!m_keyslist->selectionModel()->hasSelection())
        return QString();
    return m_proxy->nodeForIndex(m_keyslist->selectionModel()->selectedIndexes().at(0))->getId();
}

QString KgpgSelectSecretKey::getKeyMail() const
{
    if (!m_keyslist->selectionModel()->hasSelection())
        return QString();
    return m_proxy->nodeForIndex(m_keyslist->selectionModel()->selectedIndexes().at(0))->getEmail();
}

int KgpgSelectSecretKey::getSignTrust() const
{
    if (m_signtrust)
        return m_signtrust->currentIndex();
    return -1;
}

bool KgpgSelectSecretKey::isLocalSign() const
{
    return m_localsign && m_localsign->isChecked();
}

bool KgpgSelectSecretKey::isTerminalSign() const
{
    return m_terminalsign && m_terminalsign->isChecked();
}

void KgpgSelectSecretKey::slotSelectionChanged()
{
    enableButtonOk(m_keyslist->selectionModel()->hasSelection());
}

void KgpgSelectSecretKey::slotOk()
{
    if (m_keyslist->selectionModel()->hasSelection())
        slotButtonClicked(Ok);
}

#include "selectsecretkey.moc"
