#include "keylistproxymodel.h"
#include "kgpgitemnode.h"
#include "kgpgitemmodel.h"

#include <KDebug>

KeyListProxyModel::KeyListProxyModel(QObject *parent)
	: QSortFilterProxyModel(parent)
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

	return lessThan(l, r);
}

bool
KeyListProxyModel::lessThan(const KGpgNode *left, const KGpgNode *right) const
{
	KGpgRootNode *r = m_model->getRootNode();

	if (r == left->getParentKeyNode()) {
		if (r == right->getParentKeyNode()) {
			if (left->getType() == ITYPE_GROUP) {
				if (right->getType() == ITYPE_GROUP)
					return left->getName() < right->getName();
				else
					return true;
			}

			bool test1 = (left->getType() & ITYPE_PUBLIC) && !(left->getType() & ITYPE_SECRET); // only a public key
			bool test2 = (right->getType() & ITYPE_PUBLIC) && !(right->getType() & ITYPE_SECRET); // only a public key

			// key-pair goes before simple public key
			if (left->getType() == ITYPE_PAIR && test2) return true;
			if (right->getType() == ITYPE_PAIR && test1) return false;

			return (left->getName() < right->getName());
		} else {
			lessThan(left, right->getParentKeyNode());
		}
	} else {
		if (r == right->getParentKeyNode()) {
			return lessThan(left->getParentKeyNode(), right);
		} else if (left->getParentKeyNode() == right->getParentKeyNode()) {
			if (left->getType() != right->getType())
				return (left->getType() < right->getType());

			if (left->getName().startsWith('[') && !right->getName().startsWith('['))
				return false;
			else if (!left->getName().startsWith('[') && right->getName().startsWith('['))
				return true;
			else if (left->getName().startsWith('[') && right->getName().startsWith('['))
				return (left->getId() < right->getId());
			else
				return (left->getName() < right->getName());
		} else {
			return lessThan(left->getParentKeyNode(), right->getParentKeyNode());
		}
	}
	return false;
}
