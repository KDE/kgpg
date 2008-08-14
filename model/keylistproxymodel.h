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
#ifndef KEYLISTPROXYMODEL_H
#define KEYLISTPROXYMODEL_H

#include <QSortFilterProxyModel>
#include "core/kgpgkey.h"

class KGpgNode;
class KGpgExpandableNode;
class KGpgItemModel;

class KeyListProxyModel: public QSortFilterProxyModel
{
	Q_PROPERTY(int idLength READ idLength WRITE setIdLength)

public:
	explicit KeyListProxyModel(QObject * parent = 0);

	virtual bool hasChildren(const QModelIndex &idx) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	void setKeyModel(KGpgItemModel *);
	void setOnlySecret(const bool &b);
	/**
	 * \brief set the minimum trust level to be shown
	 * @param t trust level
	 *
	 * This enables filtering by key trust. All keys that have a lower trust than
	 * the given value will be hidden.
	 */
	void setTrustFilter(const KgpgCore::KgpgKeyTrustFlag &t);

	KGpgNode *nodeForIndex(const QModelIndex &index) const;
	QModelIndex nodeIndex(KGpgNode *node);
	void setPreviewSize(const int &pixel);
	inline KGpgItemModel *getModel() const
		{ return m_model; }
	inline int idLength() const
		 { return m_idLength; }
	void setIdLength(const int &length);

protected:
	virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
	virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
	bool lessThan(const KGpgNode *left, const KGpgNode *right, const int &column) const;
	bool nodeLessThan(const KGpgNode *left, const KGpgNode *right, const int &column) const;
	KGpgItemModel *m_model;
	bool m_onlysecret;
	KgpgCore::KgpgKeyTrustFlag m_mintrust;
	bool m_showexpired;
	int m_previewsize;
	int m_idLength;
};

#endif
