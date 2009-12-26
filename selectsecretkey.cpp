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

#include "images.h"
#include "kgpginterface.h"
#include "kgpgitemmodel.h"
#include "KGpgRootNode.h"
#include "kgpgsettings.h"
#include "selectkeyproxymodel.h"

using namespace KgpgCore;

KgpgSelectSecretKey::KgpgSelectSecretKey(QWidget *parent, KGpgItemModel *model, const int countkey)
                   : KDialog(parent)
{
    setCaption(i18n("Private Key List"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);
    QWidget *page = new QWidget(this);

    QLabel *label = new QLabel(i18n("Choose secret key for signing:"), page);

    m_proxy = new SelectSecretKeyProxyModel(this);
    m_proxy->setKeyModel(model);

    m_keyslist = new QTableView(page);
    m_keyslist->setModel(m_proxy);
    m_keyslist->setSortingEnabled(true);
    m_keyslist->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_keyslist->resizeColumnsToContents();

    QVBoxLayout *vbox = new QVBoxLayout(page);
    vbox->addWidget(label);
    vbox->addWidget(m_keyslist);

    if (countkey > 0) {
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
	} else {
		m_localsign = 0;
		m_terminalsign = 0;
		m_signtrust = 0;
	}

	KGpgNode *nd = model->getRootNode()->findKey(KGpgSettings::defaultKey());
	if (nd != NULL) {
		QModelIndex sidx = model->nodeIndex(nd);
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
