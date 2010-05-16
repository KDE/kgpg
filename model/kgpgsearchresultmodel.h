/* Copyright 2009,2010 Rolf Eike Beer <kde@opensource.sf-tec.de>
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
#ifndef KGPGSEARCHRESULTMODEL_H
#define KGPGSEARCHRESULTMODEL_H

#include <QAbstractItemModel>

class QString;
class QStringList;

class KGpgSearchResultModelPrivate;

/**
 * @brief Model of the results of a keyserver search
 *
 * This model parses and stores the results of a search on a keyserver.
 *
 * @author Rolf Eike Beer
 */
class KGpgSearchResultModel : public QAbstractItemModel {
	Q_OBJECT
public:
	explicit KGpgSearchResultModel(QObject *parent = NULL);
	~KGpgSearchResultModel();

	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex &index) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	/**
	 * @brief get the key fingerprint for the given index
	 * @param index valid index of any item in the model
	 * @return fingerprint of the corresponding key
	 */
	const QString &idForIndex(const QModelIndex &index) const;

public slots:
	void slotAddKey(QStringList lines);

private:
	KGpgSearchResultModelPrivate * const d;
};

#endif
