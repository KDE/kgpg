/* Copyright 2008,2013  Rolf Eike Beer <kde@opensource.sf-tec.de>
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
class KeyListProxyModelPrivate;

class KeyListProxyModel: public QSortFilterProxyModel
{
	Q_OBJECT

	Q_PROPERTY(int idLength READ idLength WRITE setIdLength)
	Q_DECLARE_PRIVATE(KeyListProxyModel)

	KeyListProxyModelPrivate * const d_ptr;

public:
	enum DisplayMode {
		MultiColumn = 0,
		SingleColumnIdFirst = 1,
		SingleColumnIdLast = 2
	};

	explicit KeyListProxyModel(QObject * parent = 0, const DisplayMode mode = MultiColumn);
        virtual ~KeyListProxyModel();

	virtual bool hasChildren(const QModelIndex &idx) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	void setKeyModel(KGpgItemModel *);
	/**
	 * @brief set the minimum trust level to be shown
	 * @param t trust level
	 *
	 * This enables filtering by key trust. All keys that have a lower trust than
	 * the given value will be hidden.
	 */
	void setTrustFilter(const KgpgCore::KgpgKeyTrustFlag t);

	/**
	 * @brief show only keys capable of encryption
	 */
	void setEncryptionKeyFilter(bool b);

	KGpgNode *nodeForIndex(const QModelIndex &index) const;
	QModelIndex nodeIndex(KGpgNode *node);
	void setPreviewSize(const int pixel);
	KGpgItemModel *getModel() const;
	int idLength() const;
	void setIdLength(const int length);

public slots:
	/**
	 * @brief set if only secret keys should be shown
	 * @param b new value
	 */
	void setOnlySecret(const bool b);

	/**
	 * @brief call this when the settings have changed
	 */
	void settingsChanged();

protected:
	virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
	virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};

#endif
