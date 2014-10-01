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

#include "gpgservermodel.h"

#include <KLocale>
#include <QStringList>

GpgServerModel::GpgServerModel(QObject *parent)
	: QStringListModel(parent),
	m_defaultRow(-1)
{
	connect(this, SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(slotRowsRemoved(QModelIndex,int,int)));
}

GpgServerModel::~GpgServerModel()
{
}

void
GpgServerModel::setDefault(const int row)
{
	Q_ASSERT(row < stringList().count());

	if (m_defaultRow == row)
		return;

	const int oldRow = m_defaultRow;
	m_defaultRow = row;
	if (oldRow >= 0)
		emit dataChanged(index(oldRow, 0), index(oldRow, 0));
	if (row >= 0)
		emit dataChanged(index(row, 0), index(row, 0));
}

void
GpgServerModel::setDefault(const QString &server)
{
	if (server.isEmpty()) {
		setDefault(-1);
	} else {
		const int row = stringList().indexOf(server);
		Q_ASSERT(row >= 0);
		setDefault(row);
	}
}

int
GpgServerModel::defaultRow() const
{
	// only in case there is not set any default yet promote the first entry of the list
	if ((m_defaultRow >= 0) || stringList().empty())
		return m_defaultRow;
	else
		return 0;
}

QString
GpgServerModel::defaultServer() const
{
	if (stringList().isEmpty())
		return QString();
	if (m_defaultRow < 0)
		return stringList().first();
	else
		return stringList().at(m_defaultRow);
}

QVariant
GpgServerModel::data(const QModelIndex &index, int role) const
{
	QVariant res = QStringListModel::data(index, role);

	if ((role == Qt::DisplayRole) && (index.row() == m_defaultRow))
		res = i18nc("Mark default keyserver in GUI", "%1 (Default)", res.toString());

	return res;
}

void
GpgServerModel::slotRowsRemoved(const QModelIndex &, int start, int end)
{
	if (end < m_defaultRow) {
		// removed before default, i.e. default is moved up
		setDefault(m_defaultRow - (end - start) - 1);
	} else if ((start <= m_defaultRow) && (end >= m_defaultRow)) {
		// the default was deleted
		if (m_defaultRow >= rowCount())
			m_defaultRow = -1; // avoid sending dataChanged() for the already deleted row
		if (rowCount() > 0)
			setDefault(0);
	}
}
