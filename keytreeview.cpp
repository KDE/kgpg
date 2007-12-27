#include "keytreeview.h"
#include "kgpgitemmodel.h"
#include "keylistproxymodel.h"
#include "kgpgitemnode.h"

KeyTreeView::KeyTreeView(QWidget *parent, KeyListProxyModel *model)
	: QTreeView(parent), m_proxy(model)
{
	setModel(model);
}

QList<KGpgNode *>
KeyTreeView::selectedNodes(bool *psame, KgpgCore::KgpgItemType *pt) const
{
	QModelIndexList selidx = selectedIndexes();
	QList<KGpgNode *> ndlist;
	KgpgItemType tp = 0;
	bool sametype = true;

	if (selidx.count() == 0) {
		if (pt != NULL)
			*pt = tp;
		if (psame != NULL)
			*psame = sametype;
		return ndlist;
	}

	tp = m_proxy->nodeForIndex(selidx[0])->getType();

	for (int i = 0; i < selidx.count(); i++) {
		if (selidx[i].column() != 0)
			continue;
		KGpgNode *nd = m_proxy->nodeForIndex(selidx[i]);

		if (nd->getType() != tp) {
			tp |= nd->getType();
			sametype = false;
		}

		ndlist << nd;
	}

	if (pt != NULL)
		*pt = tp;
	if (psame != NULL)
		*psame = sametype;
	return ndlist;
}

KGpgNode *
KeyTreeView::selectedNode() const
{
	QModelIndexList selidx = selectedIndexes();

	if (selidx.isEmpty())
		return NULL;

	return m_proxy->nodeForIndex(selidx[0]);
}

void
KeyTreeView::selectNode(KGpgNode *nd)
{
	QModelIndex idx = m_proxy->nodeIndex(nd);

	selectionModel()->select(idx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
	selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
}

void
KeyTreeView::restoreLayout(KConfigGroup & cg)
{
	QStringList cols = cg.readEntry("ColumnWidths", QStringList());
	int i = 0;

	QStringList::ConstIterator it = cols.constBegin();
	const QStringList::ConstIterator itEnd = cols.constEnd();
	for (; it != itEnd; ++it)
		setColumnWidth(i++, (*it).toInt());

	if (cg.hasKey("SortColumn")) {
		Qt::SortOrder order = cg.readEntry("SortAscending", true) ? Qt::AscendingOrder : Qt::DescendingOrder;
		sortByColumn(cg.readEntry("SortColumn", 0), order);
	}
}

void
KeyTreeView::saveLayout(KConfigGroup &cg) const
{
	QStringList widths;

	const int colCount = model()->columnCount();

	for (int i = 0; i < colCount; ++i) {
		widths << QString::number(columnWidth(i));
	}
	cg.writeEntry("ColumnWidths", widths);
#warning port me
/*	cg.writeEntry("SortColumn", d->sortColumn);
	cg.writeEntry("SortAscending", d->sortAscending);*/
}
