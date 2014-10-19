/* Copyright 2008,2014  Rolf Eike Beer <kde@opensource.sf-tec.de>
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
#ifndef GROUPEDITPROXYMODEL_H
#define GROUPEDITPROXYMODEL_H

#include "core/kgpgkey.h"

#include <QSortFilterProxyModel>

class KGpgNode;
class KGpgItemModel;

class GroupEditProxyModel: public QSortFilterProxyModel
{
public:
	explicit GroupEditProxyModel(QObject * parent, const bool invert, QList<KGpgNode *> *ids, const KgpgCore::KgpgKeyTrust mintrust = KgpgCore::TRUST_FULL);

	void setKeyModel(KGpgItemModel *);

	KGpgNode *nodeForIndex(const QModelIndex &index) const;

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual bool hasChildren(const QModelIndex &parent) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

protected:
	virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
	virtual int columnCount(const QModelIndex &) const;

private:
	KGpgItemModel *m_model;
	bool m_invert;
	QList<KGpgNode *> *m_ids;
	KgpgCore::KgpgKeyTrust m_mintrust;
};

#endif
