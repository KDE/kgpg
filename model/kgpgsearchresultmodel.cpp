/*
 * Copyright (C) 2009,2010,2013 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgsearchresultmodel.h"

#include <KDateTime>
#include <KDebug>
#include <KLocale>
#include <QString>
#include <QStringList>
#include <QTextCodec>

#include "core/convert.h"
#include "core/kgpgkey.h"

class SearchResult {
private:
	QStringList m_emails;
	QStringList m_names;

public:
	SearchResult(const QString &line);

	bool m_validPub;	// true when the "pub" line passed to constructor was valid

	void addUid(const QString &id);
	const QString &getName(const int index) const;
	const QString &getEmail(const int index) const;
	int getUidCount() const;

	QString m_fingerprint;
	unsigned int m_uatCount;

	QVariant summary() const;
private:
	KDateTime m_expiry;
	KDateTime m_creation;
	bool m_revoked;
	unsigned int m_bits;
	KgpgCore::KgpgKeyAlgo m_algo;
};

class KGpgSearchResultModelPrivate {
public:
	explicit KGpgSearchResultModelPrivate();
	~KGpgSearchResultModelPrivate();

	QList<SearchResult *> m_items;

	QString urlDecode(const QString &line);
};

SearchResult::SearchResult(const QString &line)
	: m_validPub(false),
	m_uatCount(0),
	m_revoked(false),
	m_bits(0)
{
	const QStringList parts(line.split(QLatin1Char( ':' )));

	if (parts.count() < 6)
		return;

	if (parts.at(1).isEmpty())
		return;

	m_fingerprint = parts.at(1);
	m_algo = KgpgCore::Convert::toAlgo(parts.at(2));
	m_bits = parts.at(3).toUInt();
	m_creation.setTime_t(parts.at(4).toULongLong());
	m_revoked = (parts.at(6) == QLatin1String( "r" ));

	m_validPub = true;
}

void
SearchResult::addUid(const QString &id)
{
	Q_ASSERT(m_emails.count() == m_names.count());
	QRegExp hasmail( QLatin1String( "(.*) <(.*)>" ));

	if (hasmail.exactMatch(id)) {
		m_names.append(hasmail.capturedTexts().at(1));
		m_emails.append(hasmail.capturedTexts().at(2));
	} else {
		m_names.append(id);
		m_emails.append(QString());
	}
}

const QString &
SearchResult::getName(const int index) const
{
	return m_names.at(index);
}

const QString &
SearchResult::getEmail(const int index) const
{
	return m_emails.at(index);
}

int
SearchResult::getUidCount() const
{
	Q_ASSERT(m_emails.count() == m_names.count());

	return m_emails.count();
}

QVariant
SearchResult::summary() const
{
	if (m_revoked) {
		return i18nc("example: ID abc123xy, 1024-bit RSA key, created Jan 12 2009, revoked",
				"ID %1, %2-bit %3 key, created %4, revoked", m_fingerprint,
				m_bits, KgpgCore::Convert::toString(m_algo),
				m_creation.toString(KDateTime::LocalDate));
	} else {
		return i18nc("example: ID abc123xy, 1024-bit RSA key, created Jan 12 2009",
				"ID %1, %2-bit %3 key, created %4", m_fingerprint,
				m_bits, KgpgCore::Convert::toString(m_algo),
				m_creation.toString(KDateTime::LocalDate));
	}
}

KGpgSearchResultModelPrivate::KGpgSearchResultModelPrivate()
{
}

KGpgSearchResultModelPrivate::~KGpgSearchResultModelPrivate()
{
	foreach (SearchResult *item, m_items)
		delete item;
}

QString
KGpgSearchResultModelPrivate::urlDecode(const QString &line)
{
	if (!line.contains(QLatin1Char( '%' )))
		return line;

	QByteArray tmp(line.toAscii());
	const QRegExp hex( QLatin1String( "[A-F0-9]{2}" ));	// URL-encoding uses only uppercase

	int pos = -1;	// avoid error if '%' is URL-encoded
	while ((pos = tmp.indexOf("%", pos + 1)) >= 0) {
		const QByteArray hexnum(tmp.mid(pos + 1, 2));

		// the input is not properly URL-encoded, so assume it does not need to be decoded at all
		if (!hex.exactMatch(QLatin1String( hexnum )))
			return line;

		char n[2];
		// this must work as we checked the regexp before
		n[0] = hexnum.toUShort(NULL, 16);
		n[1] = '\0';	// to use n as a 0-terminated string

		tmp.replace(pos, 3, n);
	}

	return QTextCodec::codecForName("utf8")->toUnicode(tmp);
}

KGpgSearchResultModel::KGpgSearchResultModel(QObject *parent)
	: QAbstractItemModel(parent), d(new KGpgSearchResultModelPrivate())
{
}

KGpgSearchResultModel::~KGpgSearchResultModel()
{
	delete d;
}

QVariant
KGpgSearchResultModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	if (index.row() < 0)
		return QVariant();

	SearchResult *tmp = static_cast<SearchResult *>(index.internalPointer());
	int row;

	if (tmp == NULL) {
		// this is a "top" item, show the first uid
		if (index.row() >= d->m_items.count())
			return QVariant();

		tmp = d->m_items.at(index.row());
		row = 0;
	} else {
		row = index.row() + 1;
		int summaryRow = tmp->getUidCount();
		int uatRow;
		if (tmp->m_uatCount != 0) {
			uatRow = summaryRow;
			summaryRow++;
		} else {
			uatRow = -1;
		}

		if (row == uatRow) {
			if (index.column() == 0)
				return i18np("One Photo ID", "%1 Photo IDs", tmp->m_uatCount);
			else
				return QVariant();
		} else if (row == summaryRow) {
			if (index.column() == 0)
				return tmp->summary();
			else
				return QVariant();
		}
		Q_ASSERT(row < tmp->getUidCount());
	}

	switch (index.column()) {
	case 0:
		return tmp->getName(row);
	case 1:
		return tmp->getEmail(row);
	default:
		return QVariant();
	}
}

int
KGpgSearchResultModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		if (parent.column() != 0)
			return 0;

		SearchResult *tmp = static_cast<SearchResult *>(parent.internalPointer());

		if (tmp == NULL)
			return 2;
		else
			return 0;
	} else {
		return 2;
	}
}

QModelIndex
KGpgSearchResultModel::index(int row, int column, const QModelIndex &parent) const
{
	// there are three hierarchy levels:
	// root: this is simply QModelIndex()
	// key items: parent is invalid, internalPointer is NULL
	// uid entries: parent is key item, internalPointer is set to SearchResult*

	if (parent.isValid()) {
		if (parent.internalPointer() != NULL) {
			return QModelIndex();
		} else {
			if (parent.row() >= d->m_items.count())
				return QModelIndex();
			SearchResult *tmp = d->m_items.at(parent.row());
			int maxRow = tmp->getUidCount();
			if (tmp->m_uatCount != 0)
				maxRow++;
			if ((row >= maxRow) || (column > 1))
				return QModelIndex();
			return createIndex(row, column, tmp);
		}
	} else {
		if ((row >= d->m_items.count()) || (column > 1) || (row < 0) || (column < 0))
			return QModelIndex();
		return createIndex(row, column);
	}
}

QModelIndex
KGpgSearchResultModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	SearchResult *tmp = static_cast<SearchResult *>(index.internalPointer());

	if (tmp == NULL)
		return QModelIndex();

	return createIndex(d->m_items.indexOf(tmp), 0);
}

int
KGpgSearchResultModel::rowCount(const QModelIndex &parent) const
{
	if (!parent.isValid()) {
		return d->m_items.count();
	} else if (parent.column() == 0) {
		if (parent.internalPointer() != NULL)
			return 0;

		SearchResult *item = d->m_items.at(parent.row());
		int cnt = item->getUidCount();
		if (item->m_uatCount != 0)
			cnt++;

		return cnt;
	} else {
		return 0;
	}
}

QVariant
KGpgSearchResultModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation != Qt::Horizontal)
		return QVariant();

	switch (section) {
	case 0:
		return i18n("Name");
	case 1:
		return QString(i18nc("@title:column Title of a column of emails", "Email"));
	default:
		return QVariant();
	}
}

const QString &
KGpgSearchResultModel::idForIndex(const QModelIndex &index) const
{
	Q_ASSERT(index.isValid());

	SearchResult *tmp = static_cast<SearchResult *>(index.internalPointer());
	if (tmp == NULL)
		tmp = d->m_items.at(index.row());

	return tmp->m_fingerprint;
}

void
KGpgSearchResultModel::slotAddKey(QStringList lines)
{
	Q_ASSERT(!lines.isEmpty());
	Q_ASSERT(lines.first().startsWith(QLatin1String("pub:")));

	if (lines.count() == 1)
		return;

	SearchResult *nkey = new SearchResult(lines.takeFirst());
	if (!nkey->m_validPub) {
		delete nkey;
		return;
	}

	foreach (const QString &line, lines) {
		if (line.startsWith(QLatin1String("uid:"))) {
			QString kid = d->urlDecode(line.section(QLatin1Char( ':' ), 1, 1));

			nkey->addUid(kid);
		} else if (line.startsWith(QLatin1String("uat:"))) {
			nkey->m_uatCount++;
		} else {
			kDebug(2100) << "ignored search result line" << line;
		}
	}

	if (nkey->getUidCount() > 0) {
		beginInsertRows(QModelIndex(), d->m_items.count(), d->m_items.count());
		d->m_items.append(nkey);
		endInsertRows();
	} else {
		// key server sent back a crappy key
		delete nkey;
	}
}

#include "kgpgsearchresultmodel.moc"
