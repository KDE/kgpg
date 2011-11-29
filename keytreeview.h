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
#ifndef KEYTREEVIEW_H
#define KEYTREEVIEW_H

#include <QTreeView>

#include <KUrl>

#include "core/kgpgkey.h"

class QDragMoveEvent;
class QDropEvent;

class KGpgNode;
class KeyListProxyModel;
class KConfigGroup;

class KeyTreeView: public QTreeView
{
	Q_OBJECT
private:
	KeyListProxyModel *m_proxy;

public:
	explicit KeyTreeView(QWidget *parent = 0, KeyListProxyModel *model = 0);

	QList<KGpgNode *> selectedNodes(bool *psame = NULL, KgpgCore::KgpgItemType *pt = NULL) const;
	KGpgNode *selectedNode() const;

	void restoreLayout(KConfigGroup &cg);
	void saveLayout(KConfigGroup &cg) const;

	bool isEditing() const;

signals:
	void importDrop(const KUrl::List &urls);
	void returnPressed();

public slots:
	void selectNode(KGpgNode *nd);
	void resizeColumnsToContents();

protected:
	virtual void contentsDragMoveEvent(QDragMoveEvent *e);
	virtual void contentsDropEvent(QDropEvent *e);
	virtual void startDrag(Qt::DropActions);
	virtual void keyPressEvent(QKeyEvent *event);
};

#endif
