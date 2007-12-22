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
