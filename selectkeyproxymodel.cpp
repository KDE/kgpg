#include "selectkeyproxymodel.h"
#include "kgpgitemnode.h"
#include "kgpgitemmodel.h"
#include "core/kgpgkey.h"

using namespace KgpgCore;

SelectKeyProxyModel::SelectKeyProxyModel(QObject *parent)
	: QSortFilterProxyModel(parent), m_model(NULL)
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
	case ITYPE_PAIR:
	case ITYPE_PUBLIC:
		break;
	default:
		return false;
	}

	// there is probably a better place to do this
	QRegExp rx = filterRegExp();
	rx.setCaseSensitivity(Qt::CaseInsensitive);
	if (!m_showUntrusted && !((l->getTrust() == TRUST_FULL) || (l->getTrust() == TRUST_ULTIMATE)))
		return false;

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
	case 0:	return m_model->headerData(KEYCOLUMN_NAME, orientation, role);
	case 1:	return m_model->headerData(KEYCOLUMN_EMAIL, orientation, role);
	case 2:	return m_model->headerData(KEYCOLUMN_ID, orientation, role);
	default:	return QVariant();
	}
}

void
SelectKeyProxyModel::setShowUntrusted(const bool &b)
{
	m_showUntrusted = b;
	invalidate();
}
