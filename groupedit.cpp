/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2012,2013 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "groupedit.h"

#include "kgpgsettings.h"
#include "core/kgpgkey.h"
#include "model/groupeditproxymodel.h"
#include "model/kgpgitemmodel.h"

#include <KAction>
#include <KActionCollection>
#include <QHeaderView>
#include <QSortFilterProxyModel>

groupEdit::groupEdit(QWidget *parent, QList<KGpgNode *> *ids, KGpgItemModel *md)
	: QWidget(parent),
	m_outFilter(new QSortFilterProxyModel(this)),
	members(ids)
{
	Q_ASSERT(ids != NULL);
	Q_ASSERT(md != NULL);

	setupUi( this );
	KgpgCore::KgpgKeyTrust mintrust;
	if (KGpgSettings::allowUntrustedGroupMembers()) {
		mintrust = KgpgCore::TRUST_UNDEFINED;
		textLabelAvailable->setText(i18n("Available Keys"));
	} else {
		mintrust = KgpgCore::TRUST_FULL;
		textLabelAvailable->setText(i18n("Available Trusted Keys"));
	}

	m_in = new GroupEditProxyModel(this, false, members, KgpgCore::TRUST_MINIMUM);
	m_in->setKeyModel(md);
	m_out = new GroupEditProxyModel(this, true, members, mintrust);
	m_out->setKeyModel(md);

	m_outFilter->setSourceModel(m_out);
	m_outFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
	m_outFilter->setFilterKeyColumn(-1);

	connect(filterEdit, SIGNAL(textChanged(QString)), m_outFilter, SLOT(setFilterFixedString(QString)));

	availableKeys->setModel(m_outFilter);
	groupKeys->setModel(m_in);
	buttonAdd->setIcon(KIcon( QLatin1String( "go-down" )));
	buttonRemove->setIcon(KIcon( QLatin1String( "go-up" )));

	availableKeys->setColumnWidth(0, 200);
	availableKeys->setColumnWidth(1, 200);
	availableKeys->setColumnWidth(2, 100);
	availableKeys->verticalHeader()->hide();

	groupKeys->setColumnWidth(0, 200);
	groupKeys->setColumnWidth(1, 200);
	groupKeys->setColumnWidth(2, 100);
	groupKeys->verticalHeader()->hide();

	setMinimumSize(sizeHint());

	connect(buttonAdd, SIGNAL(clicked()), this, SLOT(groupAdd()));
	connect(buttonRemove, SIGNAL(clicked()), this, SLOT(groupRemove()));
	connect(availableKeys, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(groupAdd(QModelIndex)));
	connect(groupKeys, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(groupRemove(QModelIndex)));
}

groupEdit::~groupEdit()
{
	delete m_in;
	delete m_out;
}

void
groupEdit::groupAdd()
{
	QModelIndexList sel = availableKeys->selectionModel()->selectedIndexes();
	for (int i = 0; i < sel.count(); i++) {
		if (sel.at(i).column() != 0)
			continue;
		KGpgNode *nd = m_out->nodeForIndex(m_outFilter->mapToSource(sel.at(i)));
		members->append(nd);
	}
	m_in->invalidate();
	m_out->invalidate();
}

void
groupEdit::groupRemove()
{
	Q_ASSERT(!members->isEmpty());
	QModelIndexList sel = groupKeys->selectionModel()->selectedIndexes();
	for (int i = 0; i < sel.count(); i++) {
		if (sel.at(i).column() != 0)
			continue;
		KGpgNode *nd = m_in->nodeForIndex(sel.at(i));
		for (int j = 0; j < members->count(); j++)
			if (nd->toKeyNode()->compareId(members->at(j)->getId())) {
				members->removeAt(j);
				break;
			}
	}
	m_in->invalidate();
	m_out->invalidate();
}

void
groupEdit::groupAdd(const QModelIndex &index)
{
	KGpgNode *nd = m_out->nodeForIndex(m_outFilter->mapToSource(index));
	members->append(nd);
	m_in->invalidate();
	m_out->invalidate();
}

void
groupEdit::groupRemove(const QModelIndex &index)
{
	Q_ASSERT(!members->isEmpty());
	KGpgKeyNode *nd = m_in->nodeForIndex(index)->toKeyNode();
	for (int i = 0; i < members->count(); i++)
		if (nd->compareId(members->at(i)->getId())) {
			members->removeAt(i);
			break;
		}
	m_in->invalidate();
	m_out->invalidate();
}

#include "groupedit.moc"
