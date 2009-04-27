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
#include "keylistproxymodel.h"
#include "kgpgitemnode.h"
#include "kgpgitemmodel.h"
#include "core/kgpgkey.h"
#include "core/convert.h"

#include <KLocale>

using namespace KgpgCore;

class KeyListProxyModelPrivate {
	KeyListProxyModel * const q_ptr;

	Q_DECLARE_PUBLIC(KeyListProxyModel)
public:
	KeyListProxyModelPrivate(KeyListProxyModel *parent, const KeyListProxyModel::DisplayMode mode);

	bool lessThan(const KGpgNode *left, const KGpgNode *right, const int &column) const;
	bool nodeLessThan(const KGpgNode *left, const KGpgNode *right, const int &column) const;
	KGpgItemModel *m_model;
	bool m_onlysecret;
	KgpgCore::KgpgKeyTrustFlag m_mintrust;
	bool m_showexpired;
	int m_previewsize;
	int m_idLength;
	KeyListProxyModel::DisplayMode m_displaymode;

	QVariant dataSingleColumn(const QModelIndex &index, int role, const KGpgNode *node) const;
	QVariant dataMultiColumn(const QModelIndex &index, int role, const KGpgNode *node) const;
};

KeyListProxyModelPrivate::KeyListProxyModelPrivate(KeyListProxyModel *parent, const KeyListProxyModel::DisplayMode mode)
	: q_ptr(parent),
	m_onlysecret(false),
	m_mintrust(TRUST_UNKNOWN),
	m_previewsize(22),
	m_idLength(8),
	m_displaymode(mode)
{
}

QVariant
KeyListProxyModelPrivate::dataSingleColumn(const QModelIndex &index, int role, const KGpgNode *node) const
{
	Q_Q(const KeyListProxyModel);

	if (index.column() != 0)
		return QVariant();

	switch (role) {
	case Qt::DecorationRole:
		if (node->getType() == ITYPE_UAT) {
			if (m_previewsize > 0) {
				const KGpgUatNode *nd = node->toUatNode();
				return nd->getPixmap().scaled(m_previewsize + 5, m_previewsize, Qt::KeepAspectRatio);
			} else {
				return Convert::toPixmap(ITYPE_UAT);
			}
		} else {
			return m_model->data(q->mapToSource(index), Qt::DecorationRole);
		}
	case Qt::DisplayRole: {
		const QModelIndex srcidx(q->mapToSource(index));
		const int srcrow = srcidx.row();

		const QModelIndex ididx(srcidx.sibling(srcrow, KEYCOLUMN_ID));
		const QString id(m_model->data(ididx, Qt::DisplayRole).toString().right(m_idLength));

		const QModelIndex mailidx(srcidx.sibling(srcrow, KEYCOLUMN_EMAIL));
		const QString mail(m_model->data(mailidx, Qt::DisplayRole).toString());

		const QModelIndex nameidx(srcidx.sibling(srcrow, KEYCOLUMN_NAME));
		const QString name(m_model->data(nameidx, Qt::DisplayRole).toString());

		if (m_displaymode == KeyListProxyModel::SingleColumnIdFirst) {
			if (mail.isEmpty())
				return i18nc("ID: Name", "%1: %2", id, name);
			else
				return i18nc("ID: Name <Email>", "%1: %2 &lt;%3&gt;", id, name, mail);
		} else {
			if (mail.isEmpty())
				return i18nc("Name: ID", "%1: %2", name, id);
			else
				return i18nc("Name <Email>: ID", "%1 &lt;%2&gt;: %3", name, mail, id);
		}
		}
	default:
		return QVariant();
	}
}

QVariant
KeyListProxyModelPrivate::dataMultiColumn(const QModelIndex &index, int role, const KGpgNode *node) const
{
	Q_Q(const KeyListProxyModel);

	if ((node->getType() == ITYPE_UAT) && (role == Qt::DecorationRole) && (index.column() == 0)) {
		if (m_previewsize > 0) {
			const KGpgUatNode *nd = node->toUatNode();
			return nd->getPixmap().scaled(m_previewsize + 5, m_previewsize, Qt::KeepAspectRatio);
		} else {
			return Convert::toPixmap(ITYPE_UAT);
		}
	} else if ((role == Qt::DisplayRole) && (index.column() == KEYCOLUMN_ID)) {
		QString id = m_model->data(q->mapToSource(index), Qt::DisplayRole).toString();
		return id.right(m_idLength);
	} else if ((role == Qt::ToolTipRole) && (index.column() == KEYCOLUMN_ID)) {
		QString id = m_model->data(q->mapToSource(index), Qt::DisplayRole).toString();
		return id;
	}
	return m_model->data(q->mapToSource(index), role);
}

KeyListProxyModel::KeyListProxyModel(QObject *parent, const DisplayMode mode)
	: QSortFilterProxyModel(parent),
	d_ptr(new KeyListProxyModelPrivate(this, mode))
{
	setFilterCaseSensitivity(Qt::CaseInsensitive);
	setFilterKeyColumn(-1);
}

bool
KeyListProxyModel::hasChildren(const QModelIndex &idx) const
{
	return sourceModel()->hasChildren(mapToSource(idx));
}

void
KeyListProxyModel::setKeyModel(KGpgItemModel *md)
{
	Q_D(KeyListProxyModel);

	d->m_model = md;
	setSourceModel(md);
}

bool
KeyListProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	Q_D(const KeyListProxyModel);

	KGpgNode *l = d->m_model->nodeForIndex(left);
	KGpgNode *r = d->m_model->nodeForIndex(right);

	return d->lessThan(l, r, left.column());
}

bool
KeyListProxyModelPrivate::lessThan(const KGpgNode *left, const KGpgNode *right, const int &column) const
{
	KGpgRootNode *r = m_model->getRootNode();

	if (r == left->getParentKeyNode()) {
		if (r == right->getParentKeyNode()) {
			if (left->getType() == ITYPE_GROUP) {
				if (right->getType() == ITYPE_GROUP)
					return left->getName() < right->getName();
				else
					return true;
			} else if (right->getType() == ITYPE_GROUP)
				return false;

			// we don't need to care about group members here because they will never have root as parent
			bool test1 = (left->getType() & ITYPE_PUBLIC) && !(left->getType() & ITYPE_SECRET); // only a public key
			bool test2 = (right->getType() & ITYPE_PUBLIC) && !(right->getType() & ITYPE_SECRET); // only a public key

			// key-pair goes before simple public key
			// extra check needed to get sorting by trust right
			if (left->getType() == ITYPE_PAIR && test2) return (column != KEYCOLUMN_TRUST);
			if (right->getType() == ITYPE_PAIR && test1) return (column == KEYCOLUMN_TRUST);

			return nodeLessThan(left, right, column);
		} else {
			return lessThan(left, right->getParentKeyNode(), column);
		}
	} else {
		if (r == right->getParentKeyNode()) {
			return lessThan(left->getParentKeyNode(), right, column);
		} else if (left->getParentKeyNode() == right->getParentKeyNode()) {
			if (left->getType() != right->getType())
				return (left->getType() < right->getType());

			return nodeLessThan(left, right, column);
		} else {
			return lessThan(left->getParentKeyNode(), right->getParentKeyNode(), column);
		}
	}
	return false;
}

bool
KeyListProxyModelPrivate::nodeLessThan(const KGpgNode *left, const KGpgNode *right, const int &column) const
{
	Q_ASSERT(left->getType() == right->getType());

	switch (column) {
	case KEYCOLUMN_NAME:
		if (left->getType() == ITYPE_SIGN) {
			if (left->getName().startsWith('[') && !right->getName().startsWith('['))
				return false;
			else if (!left->getName().startsWith('[') && right->getName().startsWith('['))
				return true;
			else if (left->getName().startsWith('[') && right->getName().startsWith('['))
				return (left->getId() < right->getId());
		}
		return (left->getName().compare(right->getName().toLower(), Qt::CaseInsensitive) < 0);
	case KEYCOLUMN_EMAIL:
		return (left->getEmail() < right->getEmail());
	case KEYCOLUMN_TRUST:
		return (left->getTrust() < right->getTrust());
	case KEYCOLUMN_EXPIR:
		return (left->getExpiration() < right->getExpiration());
	case KEYCOLUMN_SIZE:
		if ((left->getType() & ITYPE_PAIR) && (right->getType() & ITYPE_PAIR)) {
			unsigned int lsign, lenc, rsign, renc;

			if (left->getType() & ITYPE_GROUP) {
				const KGpgGroupMemberNode *g = static_cast<const KGpgGroupMemberNode *>(left);

				lsign = g->getSignKeySize();
				lenc = g->getEncryptionKeySize();
			} else {
				const KGpgKeyNode *g = static_cast<const KGpgKeyNode *>(left);

				lsign = g->getSignKeySize();
				lenc = g->getEncryptionKeySize();
			}

			if (right->getType() & ITYPE_GROUP) {
				const KGpgGroupMemberNode *g = static_cast<const KGpgGroupMemberNode *>(right);

				rsign = g->getSignKeySize();
				renc = g->getEncryptionKeySize();
			} else {
				const KGpgKeyNode *g = static_cast<const KGpgKeyNode *>(right);

				rsign = g->getSignKeySize();
				renc = g->getEncryptionKeySize();
			}

			if (lsign != rsign)
				return lsign < rsign;
			else
				return lenc < renc;
		} else {
			return (left->getSize() < right->getSize());
		}
	case KEYCOLUMN_CREAT:
		return (left->getCreation() < right->getCreation());
	default:
		Q_ASSERT(column == KEYCOLUMN_ID);
		return (left->getId().right(m_idLength) < right->getId().right(m_idLength));
	}
}

bool
KeyListProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	Q_D(const KeyListProxyModel);
	QModelIndex idx = d->m_model->index(source_row, 0, source_parent);
	KGpgNode *l = d->m_model->nodeForIndex(idx);

	if (d->m_onlysecret) {
		switch (l->getType()) {
		case ITYPE_PUBLIC:
		case ITYPE_GPUBLIC:
		case ITYPE_GROUP:
			return false;
		default:
			break;
		}
	}

	switch (d->m_displaymode) {
	case SingleColumnIdFirst:
	case SingleColumnIdLast:
		if (l->getType() == ITYPE_GROUP)
			return false;
	default:
		break;
	}

	if (l->getTrust() < d->m_mintrust)
		return false;

	if (l->getParentKeyNode() != d->m_model->getRootNode())
		return true;

	if (l->getName().contains(filterRegExp()))
		return true;

	if (l->getEmail().contains(filterRegExp()))
		return true;

	if (l->getId().contains(filterRegExp()))
		return true;

	return false;
}

void
KeyListProxyModel::setOnlySecret(const bool &b)
{
	Q_D(KeyListProxyModel);

	d->m_onlysecret = b;
	invalidateFilter();
}

void
KeyListProxyModel::setTrustFilter(const KgpgCore::KgpgKeyTrustFlag &t)
{
	Q_D(KeyListProxyModel);

	d->m_mintrust = t;
	invalidateFilter();
}

KGpgNode *
KeyListProxyModel::nodeForIndex(const QModelIndex &index) const
{
	Q_D(const KeyListProxyModel);

	return d->m_model->nodeForIndex(mapToSource(index));
}

QModelIndex
KeyListProxyModel::nodeIndex(KGpgNode *node)
{
	Q_D(KeyListProxyModel);

	return mapFromSource(d->m_model->nodeIndex(node));
}

void
KeyListProxyModel::setPreviewSize(const int &pixel)
{
	Q_D(KeyListProxyModel);

	emit layoutAboutToBeChanged();
	d->m_previewsize = pixel;
	emit layoutChanged();
}

QVariant
KeyListProxyModel::data(const QModelIndex &index, int role) const
{
	Q_D(const KeyListProxyModel);

	if (!index.isValid())
		return QVariant();

	const KGpgNode *node = nodeForIndex(index);

	switch (d->m_displaymode) {
	case MultiColumn:
		return d->dataMultiColumn(index, role, node);
	case SingleColumnIdFirst:
	case SingleColumnIdLast:
		return d->dataSingleColumn(index, role, node);
	}

	Q_ASSERT(0);

	return QVariant();
}

KGpgItemModel *
KeyListProxyModel::getModel() const
{
	Q_D(const KeyListProxyModel);

	return d->m_model;
}

int
KeyListProxyModel::idLength() const
{
	Q_D(const KeyListProxyModel);

	return d->m_idLength;
}

void
KeyListProxyModel::setIdLength(const int &length)
{
	Q_D(KeyListProxyModel);

	if (length == d->m_idLength)
		return;

	d->m_idLength = length;
	invalidate();
}
