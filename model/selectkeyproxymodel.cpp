/* Copyright 2008,2010,2012,2013  Rolf Eike Beer <kde@opensource.sf-tec.de>
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
#include "selectkeyproxymodel.h"
#include "model/kgpgitemnode.h"
#include "kgpgitemmodel.h"
#include "core/kgpgkey.h"

#include <KGlobal>
#include <KLocale>

using namespace KgpgCore;

SelectKeyProxyModel::SelectKeyProxyModel(QObject *parent)
	: QSortFilterProxyModel(parent),
	m_model(NULL),
	m_showUntrusted(false)
{
}

void
SelectKeyProxyModel::setKeyModel(KGpgItemModel *md)
{
	m_model = md;
	setSourceModel(md);
}

bool
SelectKeyProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	QModelIndex idx = m_model->index(source_row, 0, source_parent);
	KGpgNode *l = m_model->nodeForIndex(idx);

	switch (l->getType()) {
	case ITYPE_GROUP:
		break;
	case ITYPE_PAIR:
	case ITYPE_PUBLIC:
		if (!l->toKeyNode()->canEncrypt())
			return false;
		break;
	default:
		return false;
	}

	if (!m_showUntrusted && ((l->getTrust() != TRUST_FULL) && (l->getTrust() != TRUST_ULTIMATE)))
		return false;

	// there is probably a better place to do this
	QRegExp rx = filterRegExp();
	rx.setCaseSensitivity(Qt::CaseInsensitive);

	if (l->getName().contains(rx))
		return true;

	if (l->getEmail().contains(rx))
		return true;

	if (l->getId().contains(rx))
		return true;

	return false;
}

KGpgNode *
SelectKeyProxyModel::nodeForIndex(const QModelIndex &index) const
{
	return m_model->nodeForIndex(mapToSource(index));
}

int
SelectKeyProxyModel::columnCount(const QModelIndex &) const
{
	return 3;
}

int
SelectKeyProxyModel::rowCount(const QModelIndex &parent) const
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
SelectKeyProxyModel::data(const QModelIndex &index, int role) const
{
	if (index.column() >= 3)
		return QVariant();

	QModelIndex sidx = mapToSource(index);
	KGpgNode *nd = m_model->nodeForIndex(sidx);

	if ((index.column() == 2) && (role == Qt::ToolTipRole))
		return nd->getId();

	if ((role != Qt::DisplayRole) && (index.column() <= 1))
		return m_model->data(sidx, role);

	if (role != Qt::DisplayRole)
		return QVariant();

	switch (index.column()) {
		case 0:	return nd->getName();
		case 1:	return nd->getEmail();
		case 2:	return nd->getId().right(8);
	}

	return QVariant();
}

bool
SelectKeyProxyModel::hasChildren(const QModelIndex &parent) const
{
	if (m_model == NULL)
		return false;
	return !parent.isValid();
}

QVariant
SelectKeyProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
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

bool
SelectKeyProxyModel::showUntrusted() const
{
	return m_showUntrusted;
}

void
SelectKeyProxyModel::setShowUntrusted(const bool b)
{
	m_showUntrusted = b;
	invalidate();
}

SelectSecretKeyProxyModel::SelectSecretKeyProxyModel(QObject *parent)
	: SelectKeyProxyModel(parent)
{
}

bool
SelectSecretKeyProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	QModelIndex idx = m_model->index(source_row, 0, source_parent);
	KGpgNode *l = m_model->nodeForIndex(idx);

	return ((l->getType() == ITYPE_PAIR) && !(l->getTrust() == TRUST_EXPIRED) && !(l->getTrust() == TRUST_DISABLED));
}

int
SelectSecretKeyProxyModel::columnCount(const QModelIndex &) const
{
	return 4;
}

int
SelectSecretKeyProxyModel::rowCount(const QModelIndex &parent) const
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
SelectSecretKeyProxyModel::data(const QModelIndex &index, int role) const
{
	if (index.column() >= 4)
		return QVariant();

	QModelIndex sidx = mapToSource(index);
	KGpgNode *nd = m_model->nodeForIndex(sidx);

	if ((index.column() == 3) && (role == Qt::ToolTipRole))
		return nd->getId();

	if ((role != Qt::DisplayRole) && (index.column() <= 1))
		return m_model->data(sidx, role);

	if (role != Qt::DisplayRole)
		return QVariant();

	switch (index.column()) {
	case 0:
		return nd->getName();
	case 1:
		return nd->getEmail();
	case 2:
		return KGlobal::locale()->formatDate(nd->getExpiration().date(), KLocale::ShortDate);
	case 3:
		return nd->getId().right(8);
	}

	return QVariant();
}

bool
SelectSecretKeyProxyModel::hasChildren(const QModelIndex &parent) const
{
	if (m_model == NULL)
		return false;
	return !parent.isValid();
}

QVariant
SelectSecretKeyProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
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
		return m_model->headerData(KEYCOLUMN_EXPIR, orientation, role);
	case 3:
		return m_model->headerData(KEYCOLUMN_ID, orientation, role);
	default:
		return QVariant();
	}
}
