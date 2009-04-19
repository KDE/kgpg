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
#ifndef KGPGITEMMODEL_H
#define KGPGITEMMODEL_H

#include <QAbstractItemModel>

#define KEYCOLUMN_NAME	0
#define KEYCOLUMN_EMAIL	1
#define KEYCOLUMN_TRUST	2
#define KEYCOLUMN_EXPIR	3
#define KEYCOLUMN_SIZE	4
#define KEYCOLUMN_CREAT	5
#define KEYCOLUMN_ID	6

#include "kgpgitemnode.h"
#include "kgpgkey.h"

class QColor;

class KGpgItemModel : public QAbstractItemModel
{
	Q_OBJECT

private:
	KGpgRootNode *m_root;
	QString m_default;

public:

	explicit KGpgItemModel(QObject *parent = 0);
	virtual ~KGpgItemModel();

	virtual QModelIndex index(int row, int column,
				const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex &child) const;

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex & /*parent = QModelIndex()*/ ) const
			{ return 7; }

	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual bool hasChildren(const QModelIndex &parent) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	KGpgNode *nodeForIndex(const QModelIndex &index) const;

	KGpgRootNode *getRootNode() const;
	QString statusCountMessage() const;

public Q_SLOTS:
	KGpgGroupNode *addGroup(const QString &name, const KGpgKeyNodeList &keys);
	void delNode(KGpgNode *node);
	void changeGroup(KGpgGroupNode *node, const QList<KGpgNode *> &keys);
	void deleteFromGroup(KGpgGroupNode *group, KGpgGroupMemberNode *member);
	void setDefaultKey(const QString &def);
	QModelIndex nodeIndex(KGpgNode *node);
	void refreshKey(const QString &id);
	void refreshKey(KGpgKeyNode *nd);
	void refreshKeys(const QStringList &ids = QStringList());
	void refreshKeys(KGpgKeyNodeList keys);
	void refreshGroups();
	void invalidateIndexes(KGpgNode *nd);
	void refreshTrust(const KgpgCore::KgpgKeyTrust &trust, const QColor &color);

protected:
	int rowForNode(KGpgNode *node) const;
	void refreshKeyIds(const QStringList &id);
	void refreshKeyIds(KGpgKeyNodeList &nodes);
	bool isDefaultKey(const KGpgNode *node) const;
	void fixPersistentIndexes();
	void updateNodeTrustColor(KGpgExpandableNode *node, const KgpgCore::KgpgKeyTrust &trust, const QColor &color);
};

#endif
