#include "keylistproxymodel.h"
#include "kgpgitemnode.h"
#include "kgpgitemmodel.h"
#include "core/kgpgkey.h"

#include <KDebug>

using namespace KgpgCore;

KeyListProxyModel::KeyListProxyModel(QObject *parent)
	: QSortFilterProxyModel(parent), m_onlysecret(false), m_showexpired(false)
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
	m_model = md;
	setSourceModel(md);
}

bool
KeyListProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	KGpgNode *l = m_model->nodeForIndex(left);
	KGpgNode *r = m_model->nodeForIndex(right);

	return lessThan(l, r, left.column());
}

bool
KeyListProxyModel::lessThan(const KGpgNode *left, const KGpgNode *right, const int &column) const
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
			if (left->getType() == ITYPE_PAIR && test2) return true;
			if (right->getType() == ITYPE_PAIR && test1) return false;

			return nodeLessThan(left, right, column);
		} else {
			lessThan(left, right->getParentKeyNode(), column);
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
KeyListProxyModel::nodeLessThan(const KGpgNode *left, const KGpgNode *right, const int &column) const
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
		return (left->getName() < right->getName());
	case KEYCOLUMN_EMAIL:
		return (left->getEmail() < right->getEmail());
	case KEYCOLUMN_TRUST:
		return (left->getTrust() < right->getTrust());
	case KEYCOLUMN_EXPIR:
		return (left->getExpiration() < right->getExpiration());
	case KEYCOLUMN_SIZE:
		return (left->getSize() < right->getSize());
	case KEYCOLUMN_CREAT:
		return (left->getCreation() < right->getCreation());
	default:
		Q_ASSERT(column == KEYCOLUMN_ID);
		return (left->getId() < right->getId());
	}
}

bool
KeyListProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	QModelIndex idx = m_model->index(source_row, 0, source_parent);
	KGpgNode *l = m_model->nodeForIndex(idx);

	if (m_onlysecret) {
		switch (l->getType()) {
		case ITYPE_PAIR:
		case ITYPE_SECRET:
			break;
		default:
			return false;
		}
	}

	if (!m_showexpired) {
		if ((l->getTrust() <= TRUST_EXPIRED) && (l->getTrust() != TRUST_UNKNOWN))
			return false;
	}

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
	m_onlysecret = b;
	invalidateFilter();
}

void
KeyListProxyModel::setShowExpired(const bool &b)
{
	m_showexpired = b;
	invalidateFilter();
}

KGpgNode *
KeyListProxyModel::nodeForIndex(const QModelIndex &index) const
{
	return m_model->nodeForIndex(mapToSource(index));
}
