/* Copyright 2008  Rolf Eike Beer <kde@opensource.sf-tec.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "groupeditproxymodel.h"
#include "kgpgitemnode.h"
#include "kgpgitemmodel.h"
#include "core/kgpgkey.h"

#include <KDebug>

using namespace KgpgCore;

GroupEditProxyModel::GroupEditProxyModel(QObject *parent, const bool &invert, QList<KGpgNode *> *ids)
	: QSortFilterProxyModel(parent), m_model(NULL), m_invert(invert), m_ids(ids)
{
}

void
GroupEditProxyModel::setKeyModel(KGpgItemModel *md)
{
	m_model = md;
	setSourceModel(md);
}

bool
GroupEditProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	QModelIndex idx = m_model->index(source_row, 0, source_parent);
	KGpgNode *l = m_model->nodeForIndex(idx);

	if (l->getType() & ~ITYPE_GPAIR)
		return false;

	if ((l->getTrust() != TRUST_FULL) && (l->getTrust() != TRUST_ULTIMATE))
		return false;

	for (int i = 0; i < m_ids->count(); i++)
		if (m_ids->at(i)->getId() == l->getId())
			return !m_invert;

	return m_invert;
}

KGpgNode *
GroupEditProxyModel::nodeForIndex(const QModelIndex &index) const
{
	return m_model->nodeForIndex(mapToSource(index));
}

int
GroupEditProxyModel::columnCount(const QModelIndex &) const
{
	return 3;
}

int
GroupEditProxyModel::rowCount(const QModelIndex &parent) const
{
	if (parent.column() > 0)
		return 0;
	if (parent.isValid())
		return 0;
	if (m_model == NULL)
		return 0;
	return QSortFilterProxyModel::rowCount(parent);
}

QVariant
GroupEditProxyModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || (index.column() >= 3) || (role != Qt::DisplayRole))
		return QVariant();

	KGpgNode *nd = m_model->nodeForIndex(mapToSource(index));
	switch (index.column()) {
		case 0:	return nd->getName();
		case 1:	return nd->getEmail();
		case 2:	return nd->getId().right(8);
	}

	return QVariant();
}

bool
GroupEditProxyModel::hasChildren(const QModelIndex &parent) const
{
	if (m_model == NULL)
		return false;
	if (parent.column() > 0)
		return false;
	return !parent.isValid();
}

QVariant
GroupEditProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation != Qt::Horizontal)
		return QVariant();

	if (m_model == NULL)
		return QVariant();

	switch (section) {
	case 0:	return m_model->headerData(KEYCOLUMN_NAME, orientation, role);
	case 1:	return m_model->headerData(KEYCOLUMN_EMAIL, orientation, role);
	case 2:	return m_model->headerData(KEYCOLUMN_ID, orientation, role);
	default:	return QVariant();
	}
}
