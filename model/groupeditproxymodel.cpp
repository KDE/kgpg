/* Copyright 2008,2010,2012,2013,2014  Rolf Eike Beer <kde@opensource.sf-tec.de>
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
#include "model/kgpgitemnode.h"
#include "kgpgitemmodel.h"
#include "core/kgpgkey.h"

#include <KDebug>
#include <KLocale>
#include <QIcon>

using namespace KgpgCore;

GroupEditProxyModel::GroupEditProxyModel(QObject *parent, const bool invert, QList<KGpgNode *> *ids, const KgpgCore::KgpgKeyTrust mintrust)
	: QSortFilterProxyModel(parent),
	m_model(NULL),
	m_invert(invert),
	m_ids(ids),
	m_mintrust(mintrust)
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

	if (l->getType() & ~ITYPE_PAIR)
		return false;

	if (l->getTrust() < m_mintrust)
		return false;

	const KGpgKeyNode * const lk = l->toKeyNode();
	for (int i = 0; i < m_ids->count(); i++)
		if (lk->compareId(m_ids->at(i)->getId()))
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
	if (!index.isValid() || (index.column() >= 3))
		return QVariant();

	KGpgNode *nd = m_model->nodeForIndex(mapToSource(index));

	switch (role) {
	case Qt::ToolTipRole:
	case Qt::DisplayRole:
		switch (index.column()) {
		case 0:
			if (role == Qt::ToolTipRole)
				return nd->getNameComment();
			else
				return nd->getName();
		case 1:
			if (role == Qt::ToolTipRole) {
				if (nd->toKeyNode()->getExpiration().isValid() && (nd->toKeyNode()->getExpiration() <= QDateTime::currentDateTime()))
					return i18nc("Expired key", "Expired");
				break;
			} else {
				return nd->getEmail();
			}
		case 2:
			if (role == Qt::ToolTipRole)
				return nd->toKeyNode()->getBeautifiedFingerprint();
			else
				return nd->getId().right(8);
		default:
			break;
		}
	case Qt::DecorationRole:
		if (index.column() != 1)
			break;

		if (nd->toKeyNode()->getExpiration().isValid() && (nd->toKeyNode()->getExpiration() <= QDateTime::currentDateTime()))
			return QIcon::fromTheme(QLatin1String("dialog-warning"));
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
	case 0:
		return m_model->headerData(KEYCOLUMN_NAME, orientation, role);
	case 1:
		return m_model->headerData(KEYCOLUMN_EMAIL, orientation, role);
	case 2:
		return m_model->headerData(KEYCOLUMN_ID, orientation, role);
	default:
		return QVariant();
	}
}
