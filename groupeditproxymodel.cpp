#include "groupeditproxymodel.h"
#include "kgpgitemnode.h"
#include "kgpgitemmodel.h"
#include "core/kgpgkey.h"

#include <KDebug>

using namespace KgpgCore;

GroupEditProxyModel::GroupEditProxyModel(QObject *parent, const bool &invert, QList<KGpgNode *> *ids)
	: QSortFilterProxyModel(parent), m_invert(invert), m_ids(ids)
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

QVariant
GroupEditProxyModel::data(const QModelIndex &index, int role) const
{
	if ((index.column() >= 3) || (role != Qt::DisplayRole))
		return QVariant();

	KGpgNode *nd = m_model->nodeForIndex(mapToSource(index));
	switch (index.column()) {
		case 0:	return nd->getName();
		case 1:	return nd->getEmail();
		case 2:	return nd->getId().right(8);
	}

	return QVariant();
}

void
GroupEditProxyModel::beginChange()
{
	emit layoutAboutToBeChanged();
}

void
GroupEditProxyModel::endChange()
{
	emit layoutChanged();
}
