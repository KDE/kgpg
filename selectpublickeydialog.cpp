/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010,2011,2012
 *               Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "selectpublickeydialog.h"

#include "kgpgsettings.h"
#include "core/images.h"
#include "core/KGpgRootNode.h"
#include "model/kgpgitemmodel.h"
#include "model/selectkeyproxymodel.h"

#include <KAction>
#include <KActionCollection>
#include <KHBox>
#include <KLineEdit>
#include <KLocale>
#include <KVBox>
#include <QCheckBox>
#include <QLabel>
#include <QPainter>
#include <QTableView>
#include <QVBoxLayout>

using namespace KgpgCore;

KgpgSelectPublicKeyDlg::KgpgSelectPublicKeyDlg(QWidget *parent, KGpgItemModel *model, const KShortcut &goDefaultKey, const bool hideasciioption, const KUrl::List &files)
	: KDialog(parent),
	m_customoptions(NULL),
	imodel(model),
	m_files(files),
	m_hideasciioption(hideasciioption)
{
    setButtons(Details | Ok | Cancel);
    setDefaultButton(Ok);
    setButtonText(Details, i18n("O&ptions"));

    int fcount = files.count();
    bool fmode = (fcount > 0);

    switch (fcount) {
    case 0:
	setCaption(i18n("Select Public Key"));
	break;
    case 1:
	setCaption(i18n("Select Public Key for %1", files.first().fileName()));
	break;
    default:
	setCaption(i18np("Select Public Key for %2 and one more file", "Select Public Key for %2 and %1 more files", files.count() - 1, files.first().fileName()));
    }

    QWidget *page = new QWidget(this);

    m_searchbar = new KHBox(page);
    m_searchbar->setSpacing(spacingHint());
    m_searchbar->setFrameShape(QFrame::StyledPanel);

    QLabel *searchlabel = new QLabel(i18n("&Search: "), m_searchbar);

    m_searchlineedit = new KLineEdit(m_searchbar);
    m_searchlineedit->setClearButtonShown(true);
    searchlabel->setBuddy(m_searchlineedit);

    iproxy = new SelectKeyProxyModel(this);
    iproxy->setKeyModel(imodel);

    m_keyslist = new QTableView(page);
    m_keyslist->setSortingEnabled(true);
    m_keyslist->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_keyslist->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_keyslist->setModel(iproxy);
    m_keyslist->resizeColumnsToContents();
    m_keyslist->setWhatsThis(i18n("<b>Public keys list</b>: select the key that will be used for encryption."));
    connect(m_searchlineedit, SIGNAL(textChanged(QString)), iproxy, SLOT(setFilterFixedString(QString)));

    optionsbox = new KVBox();
    optionsbox->setFrameShape(QFrame::StyledPanel);
    setDetailsWidget(optionsbox);

    if (m_hideasciioption)
        m_cbarmor = 0;
    else
    {
        m_cbarmor = new QCheckBox(i18n("ASCII armored encryption"), optionsbox);
        m_cbarmor->setChecked(KGpgSettings::asciiArmor());
        m_cbarmor->setWhatsThis(i18n("<b>ASCII encryption</b>: makes it possible to open the encrypted file/message in a text editor"));
    }

    m_cbuntrusted = new QCheckBox(i18n("Allow encryption with untrusted keys"), optionsbox);
    m_cbuntrusted->setChecked(KGpgSettings::allowUntrustedKeys());
    m_cbuntrusted->setWhatsThis(i18n("<b>Allow encryption with untrusted keys</b>: when you import a public key, it is usually "
                    "marked as untrusted and you cannot use it unless you sign it in order to make it 'trusted'. Checking this "
                    "box enables you to use any key, even if it has not be signed."));

    m_cbhideid = new QCheckBox(i18n("Hide user id"), optionsbox);
    connect(m_cbuntrusted, SIGNAL(toggled(bool)), this, SLOT(slotUntrusted(bool)));
    m_cbhideid->setChecked(KGpgSettings::hideUserID());
    m_cbhideid->setWhatsThis(i18n("<b>Hide user ID</b>: Do not put the keyid into encrypted packets. This option hides the receiver "
                    "of the message and is a countermeasure against traffic analysis. It may slow down the decryption process because "
                    "all available secret keys are tried."));

    m_cbsymmetric = new QCheckBox(i18n("Symmetrical encryption"), optionsbox);
    m_cbsymmetric->setWhatsThis(i18n("<b>Symmetrical encryption</b>: encryption does not use keys. You just need to give a password "
                                     "to encrypt/decrypt the file"));

    QVBoxLayout *dialoglayout = new QVBoxLayout(page);
    dialoglayout->setSpacing(spacingHint());
    dialoglayout->setMargin(0);
    dialoglayout->addWidget(m_searchbar);
    dialoglayout->addWidget(m_keyslist);
    page->setLayout(dialoglayout);

    if (KGpgSettings::allowCustomEncryptionOptions())
    {
        KHBox *expertbox = new KHBox(page);
        (void) new QLabel(i18n("Custom option:"), expertbox);

        m_customoptions = new KLineEdit(expertbox);
        m_customoptions->setText(KGpgSettings::customEncryptionOptions());
        m_customoptions->setWhatsThis(i18n("<b>Custom option</b>: for experienced users only, allows you to enter a gpg command line option, like: '--armor'"));

        dialoglayout->addWidget(expertbox);
    }

    KActionCollection *actcol = new KActionCollection(this);
    KAction *action = actcol->addAction(QLatin1String( "go_default_key" ));
    action->setText(i18n("&Go to Default Key"));
    action->setShortcut(goDefaultKey);

    connect(action, SIGNAL(triggered(bool)), SLOT(slotGotoDefaultKey()));
    connect(m_cbsymmetric, SIGNAL(toggled(bool)), this, SLOT(slotSymmetric(bool)));
    connect(m_keyslist->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(slotSelectionChanged()));
    connect(m_keyslist, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotOk()));

    setMinimumSize(550, 200);
    updateGeometry();

    setMainWidget(page);
    slotSelectionChanged();

    if (fmode)
        slotGotoDefaultKey();
}

QStringList KgpgSelectPublicKeyDlg::selectedKeys() const
{
	if (getSymmetric())
		return QStringList();

	QStringList selectedKeys;

	foreach (const QModelIndex &idx, m_keyslist->selectionModel()->selectedIndexes()) {
		if (idx.column() != 0)
			continue;
		KGpgNode *nd = iproxy->nodeForIndex(idx);
		if (nd->getType() == ITYPE_GROUP)
			selectedKeys << nd->getName();
		else
			selectedKeys << nd->getId();
	}

	return selectedKeys;
}

bool KgpgSelectPublicKeyDlg::getSymmetric() const
{
    return m_cbsymmetric->isChecked();
}

QString KgpgSelectPublicKeyDlg::getCustomOptions() const
{
    if (m_customoptions == 0)
        return QString();
    return m_customoptions->text().simplified();
}

bool KgpgSelectPublicKeyDlg::getUntrusted() const
{
    return m_cbuntrusted->isChecked();
}

bool KgpgSelectPublicKeyDlg::getArmor() const
{
    return m_hideasciioption || m_cbarmor->isChecked();
}

const KUrl::List &KgpgSelectPublicKeyDlg::getFiles() const
{
	return m_files;
}

bool KgpgSelectPublicKeyDlg::getHideId() const
{
    return m_cbhideid->isChecked();
}

void KgpgSelectPublicKeyDlg::slotOk()
{
    if (getSymmetric() || m_keyslist->selectionModel()->hasSelection())
        slotButtonClicked(Ok);
}

void KgpgSelectPublicKeyDlg::slotSelectionChanged()
{
    enableButtonOk(getSymmetric() || m_keyslist->selectionModel()->hasSelection());
}

void KgpgSelectPublicKeyDlg::slotSymmetric(const bool state)
{
    m_keyslist->setDisabled(state);
    m_cbuntrusted->setDisabled(state);
    m_cbhideid->setDisabled(state);
    m_searchbar->setDisabled(state);
    slotSelectionChanged();
}

void KgpgSelectPublicKeyDlg::slotUntrusted(const bool state)
{
	iproxy->setShowUntrusted(state);
}

void KgpgSelectPublicKeyDlg::slotGotoDefaultKey()
{
	KGpgNode *nd = imodel->getRootNode()->findKey(KGpgSettings::defaultKey());
	if (nd == NULL)
		return;
	QModelIndex sidx = imodel->nodeIndex(nd);
	QModelIndex pidx = iproxy->mapFromSource(sidx);
	m_keyslist->selectionModel()->setCurrentIndex(pidx, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
}

#include "selectpublickeydialog.moc"
