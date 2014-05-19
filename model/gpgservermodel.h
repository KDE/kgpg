/* Copyright 2014  Rolf Eike Beer <kde@opensource.sf-tec.de>
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
#ifndef GPGSERVERMODEL_H
#define GPGSERVERMODEL_H

#include <QStringListModel>

/**
 * @brief model holding the configured GnuPG key servers
 */
class GpgServerModel: public QStringListModel {
	Q_OBJECT
public:
	GpgServerModel(QObject *parent = NULL);
	virtual ~GpgServerModel();

	void setDefault(const QString &server);
	void setDefault(const int index);

	/**
	 * @brief returns the row of the current default keyserver
	 */
	int defaultRow() const;

	/**
	 * @brief returns the URL of the default server
	 * @retval QString() if no default server is selected
	 */
	QString defaultServer() const;

	virtual QVariant data(const QModelIndex &index, int role) const;

private slots:
	void slotRowsRemoved(const QModelIndex &, int start, int end);

private:
	int m_defaultRow;
};

#endif
