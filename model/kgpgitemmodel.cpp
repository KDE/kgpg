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
#include "kgpgitemmodel.h"
#include "kgpgitemnode.h"
#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "convert.h"
#include <KLocale>

KGpgItemModel::KGpgItemModel(QObject *parent)
	: QAbstractItemModel(parent)
{
	m_root = new KGpgRootNode(this);
	m_default = KGpgSettings::defaultKey();
}

KGpgItemModel::~KGpgItemModel()
{
	delete m_root;
}

QModelIndex
KGpgItemModel::index(int row, int column, const QModelIndex &parent) const
{
	if (hasIndex(row, column, parent)) {
		KGpgNode *parentNode = nodeForIndex(parent);
		KGpgNode *childNode = parentNode->getChild(row);
		return createIndex(row, column, childNode);
	}
	return QModelIndex();
}

QModelIndex
KGpgItemModel::parent(const QModelIndex &child) const
{
	if (!child.isValid())
		return QModelIndex();
	KGpgNode *childNode = nodeForIndex(child);
	KGpgNode *parentNode = childNode->m_parent;

	if (parentNode == m_root)
		return QModelIndex();

	Q_ASSERT(parentNode != NULL);
	int row = rowForNode(parentNode);
	int column = 0;
	
	return createIndex(row, column, parentNode);
}

int
KGpgItemModel::rowCount(const QModelIndex &parent) const
{
	if (parent.column() > 0)
		return 0;

	KGpgNode *parentNode = nodeForIndex(parent);

	return parentNode->getChildCount();
}

bool
KGpgItemModel::hasChildren(const QModelIndex &parent) const
{
	if (parent.column() > 0)
		return false;

	KGpgNode *parentNode = nodeForIndex(parent);

	return parentNode->hasChildren();
}

QVariant
KGpgItemModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	KGpgNode *node = nodeForIndex(index);

	if (role == Qt::FontRole) {
		QFont f;
		f.setBold(isDefaultKey(node));
		return f;
	}

	switch (index.column()) {
	case KEYCOLUMN_NAME:
		switch (role) {
		case Qt::DisplayRole:
			return node->getName();
		case Qt::DecorationRole:
			if (node->getType() == ITYPE_UAT) {
				KGpgUatNode *nd = static_cast<KGpgUatNode *>(node);
				return nd->getPixmap();
			} else {
				return Convert::toPixmap(node->getType());
			}
		case Qt::ToolTipRole:
			return node->getComment();
		}
		break;
	case KEYCOLUMN_EMAIL:
		if (role == Qt::DisplayRole)
			return node->getEmail();
		break;
	case KEYCOLUMN_TRUST:
		{
		KgpgKeyTrust t = node->getTrust();

		switch (role) {
		case Qt::BackgroundColorRole:	return Convert::toColor(t);
		case Qt::AccessibleTextRole:	return Convert::toString(t);
		}
		break;
		}
	case KEYCOLUMN_EXPIR:
		if (role == Qt::DisplayRole)
			return node->getExpiration();
		break;
	case KEYCOLUMN_SIZE:
		switch (role) {
		case Qt::DisplayRole:
			return node->getSize();
		case Qt::ToolTipRole:
			switch (node->getType()) {
			case ITYPE_PAIR:
			case ITYPE_PUBLIC:
				return static_cast<KGpgKeyNode *>(node)->getSignCount();
			case ITYPE_UAT:
				return static_cast<KGpgUatNode *>(node)->getSignCount();
			case ITYPE_UID:
				return static_cast<KGpgUidNode *>(node)->getSignCount();
			case ITYPE_SUB:
				return static_cast<KGpgSubkeyNode *>(node)->getSignCount();
			}
		}
		break;
	case KEYCOLUMN_CREAT:
		if (role == Qt::DisplayRole)
			return node->getCreation();
		break;
	case KEYCOLUMN_ID:
		if (role == Qt::DisplayRole)
			return node->getId();
		break;
	}

	Q_ASSERT(1);
	return QVariant();
}

KGpgNode *
KGpgItemModel::nodeForIndex(const QModelIndex &index) const
{
	if (index.isValid())
		return static_cast<KGpgNode*>(index.internalPointer());
	return m_root;
}

int
KGpgItemModel::rowForNode(KGpgNode *node) const
{
	return node->m_parent->getChildIndex(node);
}

KGpgRootNode *
KGpgItemModel::getRootNode() const
{
	return m_root;
}

QString
KGpgItemModel::statusCountMessage() const
{
	int groupNb = m_root->groupChildren();
	QString kmsg = i18np("1 Key", "%1 Keys", m_root->getChildCount() - groupNb);

	if (groupNb == 0) {
		return kmsg;
	} else {
		QString gmsg = i18np("1 Group", "%1 Groups", groupNb);

		return kmsg + ", " + gmsg;
	}
}

KGpgGroupNode *
KGpgItemModel::addGroup(const QString &name, const KGpgKeyNodeList &keys)
{
	KGpgGroupNode *nd;

	emit layoutAboutToBeChanged();
	nd = new KGpgGroupNode(m_root, name, keys);
	fixPersistentIndexes();
	emit layoutChanged();

	return nd;
}

void
KGpgItemModel::delNode(const KGpgNode *node)
{
	emit layoutAboutToBeChanged();
	delete node;
	fixPersistentIndexes();
	emit layoutChanged();
}

void
KGpgItemModel::changeGroup(KGpgGroupNode *node, const QList<KGpgNode *> &keys)
{
	emit layoutAboutToBeChanged();
	for (int i = node->getChildCount() - 1; i >= 0; i--) {
		bool found = false;

		for (int j = 0; j < keys.count(); j++) {
			found = (node->getChild(i)->getId() == keys.at(j)->getId());
			if (found)
				break;
		}
		if (found)
			continue;
		delete node->getChild(i);
	}

	for (int i = 0; i < keys.count(); i++) {
		bool found = false;
		for (int j = 0; j < node->getChildCount(); j++) {
			found = (node->getChild(j)->getId() == keys.at(i)->getId());
			if (found)
				break;
		}
		if (found)
			continue;
		Q_ASSERT(!(keys.at(i)->getType() & ITYPE_GROUP));
		new KGpgGroupMemberNode(node, static_cast<KGpgKeyNode *>(keys.at(i)));
	}
	fixPersistentIndexes();
	emit layoutChanged();
}

QVariant
KGpgItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation != Qt::Horizontal)
		return QVariant();

	switch (section) {
	case KEYCOLUMN_NAME:	return QString(i18n("Name"));
	case KEYCOLUMN_EMAIL:	return QString(i18n("Email"));
	case KEYCOLUMN_TRUST:	return QString(i18n("Trust"));
	case KEYCOLUMN_SIZE:	return QString(i18n("Size"));
	case KEYCOLUMN_EXPIR:	return QString(i18n("Expiration"));
	case KEYCOLUMN_CREAT:	return QString(i18n("Creation"));
	case KEYCOLUMN_ID:	return QString(i18n("ID"));
	default:	return QVariant();
	}
}

void
KGpgItemModel::setDefaultKey(const QString &def)
{
	int defrow = m_root->findKeyRow(def);
	int odefrow = m_root->findKeyRow(m_default);
	if (defrow == odefrow)
		return;

	KGpgNode *n_def = m_root->getChild(defrow);

	int lastcol = columnCount(QModelIndex()) - 1;
	if (odefrow >= 0) {
		KGpgNode *nd = m_root->getChild(odefrow);
		emit dataChanged(createIndex(odefrow, 0, nd), createIndex(odefrow, lastcol, nd));
	}

	m_default = def;
	emit dataChanged(createIndex(defrow, 0, n_def), createIndex(defrow, lastcol, n_def));
}

QModelIndex
KGpgItemModel::nodeIndex(KGpgNode *node)
{
	KGpgNode *p = node->getParentKeyNode();

	for (int i = 0; i < p->getChildCount(); i++)
		if (p->getChild(i) == node)
			return createIndex(i, 0, node);

	Q_ASSERT(1);
	return QModelIndex();
}

void
KGpgItemModel::refreshKey(KGpgKeyNode *nd)
{
	QList<KGpgNode *> nodes;

	nodes.append(nd);

	refreshKeyIds(QStringList(nd->getId()), nodes);
}

void
KGpgItemModel::refreshKey(const QString &id)
{
	QList<KGpgNode *> nodes;

	refreshKeyIds(QStringList(id), nodes);
}

void
KGpgItemModel::refreshKeys(const QStringList &ids)
{
	QList<KGpgNode *> nodes;

	refreshKeyIds(ids, nodes);
}

void
KGpgItemModel::refreshKeyIds(const QStringList &ids, QList<KGpgNode *> &nodes)
{
	emit layoutAboutToBeChanged();
	if (!nodes.isEmpty()) {
		for (int i = 0; i < nodes.count(); i++)
			delete nodes.at(i);
	} else if (ids.isEmpty()) {
		for (int i = m_root->getChildCount() - 1; i >= 0; i--) {
			KGpgNode *nd = m_root->getChild(i);
			if (nd->getType() == ITYPE_GROUP)
				continue;
			delete nd;
		}
	} else {
		QStringList::ConstIterator it = ids.constBegin();
		const QStringList::ConstIterator itEnd = ids.constEnd();
	
		for (; it != itEnd; ++it) {
			KGpgKeyNode *nd = m_root->findKey(*it);
			delete nd;
		}
	}

	m_root->addKeys(ids);
	fixPersistentIndexes();
	emit layoutChanged();
}

void
KGpgItemModel::refreshGroups()
{
	emit layoutAboutToBeChanged();
	for (int i = m_root->getChildCount() - 1; i >= 0; i--) {
		KGpgNode *nd = m_root->getChild(i);
		if (nd->getType() == ITYPE_GROUP)
			delete nd;
	}

	m_root->addGroups();
	fixPersistentIndexes();
	emit layoutChanged();
}

bool
KGpgItemModel::isDefaultKey(const KGpgNode *node) const
{
	return !m_default.isEmpty() && (m_default == node->getId().right(m_default.length()));
}

void
KGpgItemModel::fixPersistentIndexes()
{
	if (persistentIndexList().isEmpty())
		return;

	QModelIndexList newidx;

	for (int i = 0; i < persistentIndexList().count(); i++) {
		QModelIndex idx = persistentIndexList().at(i);

		if (!idx.isValid())
			continue;

		KGpgNode *nd = nodeForIndex(idx);
		int j = rowForNode(nd);
		if (j == idx.row())
			continue;

// find out if this code is ever hit
kDebug(3125) << i << nd << idx.column() << "row" << idx.row() << "new row" << j;

		if (j >= 0)
			changePersistentIndex(idx, createIndex(j, idx.column(), nd));
		else
			changePersistentIndex(idx, QModelIndex());
	}
}

void
KGpgItemModel::invalidateIndexes(KGpgNode *nd)
{
	for (int i = 0; i < persistentIndexList().count(); i++) {
		QModelIndex idx = persistentIndexList().at(i);

		KGpgNode *n = nodeForIndex(idx);

		if (n != nd)
			continue;

		changePersistentIndex(idx, QModelIndex());
	}
}

void
KGpgItemModel::refreshTrust(const KgpgCore::KgpgKeyTrust &trust, const QColor &color)
{
	updateNodeTrustColor(m_root, trust, color);
}

void
KGpgItemModel::updateNodeTrustColor(KGpgExpandableNode *node, const KgpgCore::KgpgKeyTrust &trust, const QColor &color)
{
	for (int i = 0; i < node->getChildCount(); i++) {
		KGpgNode *child = node->getChild(i);

		if (child->getTrust() == trust)
			emit dataChanged(createIndex(i, KEYCOLUMN_TRUST, child), createIndex(i, KEYCOLUMN_TRUST, child));

		if (!child->hasChildren())
			continue;

		KGpgExpandableNode *echild = static_cast<KGpgExpandableNode *>(child);
		if (echild->wasExpanded())
			updateNodeTrustColor(echild, trust, color);
	}
}
