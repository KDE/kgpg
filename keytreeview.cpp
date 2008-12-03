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
#include "keytreeview.h"

#include <QDragMoveEvent>
#include <QDropEvent>
#include <QHeaderView>

#include <KMessageBox>
#include <KLocale>
#include <KConfigGroup>

#include "kgpgitemmodel.h"
#include "keylistproxymodel.h"
#include "kgpgitemnode.h"
#include "kgpginterface.h"

KeyTreeView::KeyTreeView(QWidget *parent, KeyListProxyModel *model)
	: QTreeView(parent), m_proxy(model)
{
	setModel(model);
	setDragEnabled(true);
	setDragDropMode(DragDrop);
	setAcceptDrops(true);
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
	if (nd == NULL)
		return;

	QModelIndex idx = m_proxy->nodeIndex(nd);

	selectionModel()->select(idx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
	selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
}

void
KeyTreeView::restoreLayout(KConfigGroup &cg)
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
        cg.writeEntry( "SortColumn", header ()->sortIndicatorSection () );
        cg.writeEntry( "SortAscending", ( header()->sortIndicatorOrder () == Qt::AscendingOrder ) );
}

void
KeyTreeView::contentsDragMoveEvent(QDragMoveEvent *e)
{
	e->setAccepted(KUrl::List::canDecode(e->mimeData()));
}

void
KeyTreeView::contentsDropEvent(QDropEvent *o)
{
	KUrl::List uriList = KUrl::List::fromMimeData(o->mimeData());
	if (!uriList.isEmpty()) {
		if (KMessageBox::questionYesNo(this, i18n("<p>Do you want to import file <b>%1</b> into your key ring?</p>",
					uriList.first().path()), QString(), KGuiItem(i18n("Import")),
					KGuiItem(i18n("Do Not Import"))) != KMessageBox::Yes)
			return;

		KgpgInterface *interface = new KgpgInterface();
		connect(interface, SIGNAL(importKeyFinished(QStringList)), m_proxy->getModel(), SLOT(refreshKeys(QStringList)));
		interface->importKey(uriList.first());
	}
}

void
KeyTreeView::startDrag(Qt::DropActions supportedActions)
{
	QList<KGpgNode *> nodes = selectedNodes();

	if (nodes.isEmpty())
		return;

	KGpgNode *nd = nodes.first();
	QString keyid = nd->getId();

	if (!(nd->getType() & ITYPE_PUBLIC))
		return;

	KgpgInterface *interface = new KgpgInterface();
	QString keytxt = interface->getKeys(NULL, QStringList(keyid));
	delete interface;

	QMimeData *m = new QMimeData();
	m->setText(keytxt);
	QDrag *d = new QDrag(this);
	d->setMimeData(m);
	d->exec(supportedActions, Qt::IgnoreAction);
	// do NOT delete d.
}

void
KeyTreeView::resizeColumnsToContents()
{
	for (int i = m_proxy->columnCount() - 1; i >= 0; i--)
		resizeColumnToContents(i);
}
