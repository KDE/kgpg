/* Copyright 2008,2009,2010,2011,2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
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

#include "kgpgsettings.h"
#include "core/convert.h"
#include "model/kgpgitemnode.h"

#include <KGlobal>
#include <KLocale>
#include <QMetaObject>

KGpgItemModel::KGpgItemModel(QObject *parent)
	: QAbstractItemModel(parent),
	m_root(new KGpgRootNode(this)),
	m_default(KGpgSettings::defaultKey())
{
	QMetaObject::invokeMethod(this, "refreshGroups", Qt::QueuedConnection);
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
		case Qt::EditRole:
			return node->getName();
		case Qt::DecorationRole:
			if (node->getType() == ITYPE_UAT) {
				return node->toUatNode()->getPixmap();
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
			return KGlobal::locale()->formatDate(node->getExpiration().date(), KLocale::ShortDate);
		break;
	case KEYCOLUMN_SIZE:
		switch (role) {
		case Qt::DisplayRole:
			return node->getSize();
		case Qt::ToolTipRole:
			switch (node->getType()) {
			case ITYPE_PAIR:
			case ITYPE_PUBLIC:
				return node->toKeyNode()->getSignCount();
			case ITYPE_UAT:
				return node->toUatNode()->getSignCount();
			case ITYPE_UID:
				return node->toUidNode()->getSignCount();
			case ITYPE_SUB:
				return node->toSubkeyNode()->getSignCount();
			}
		}
		break;
	case KEYCOLUMN_CREAT:
		if (role == Qt::DisplayRole)
			return KGlobal::locale()->formatDate(node->getCreation().date(), KLocale::ShortDate);
		break;
	case KEYCOLUMN_ID:
		switch (role) {
		case Qt::DisplayRole:
			return node->getId();
		case Qt::ToolTipRole:
			switch (node->getType()) {
			case ITYPE_PAIR:
			case ITYPE_PUBLIC:
				return node->toKeyNode()->getFingerprint();
			case ITYPE_SECRET:
				return node->toOrphanNode()->getFingerprint();
			default:
				return QVariant();
			}
		default:
			return QVariant();
		}
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

KGpgKeyNode *
KGpgItemModel::findKeyNode(const QString& id) const
{
	return m_root->findKey(id);
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
	const int groups = m_root->groupChildren();
	const int keys = m_root->getChildCount() - groups;

	return statusCountMessageString(keys, groups);
}

QString
KGpgItemModel::statusCountMessageString(const unsigned int keys, const unsigned int groups)
{
	// Most people will not have groups. Handle this case
	// special so the string isn't displayed in this case at all
	if (groups == 0) {
		return i18np("1 Key", "%1 Keys", keys);
	}
	
	const QString keyString = i18np("1 Key", "%1 Keys", keys);
	const QString groupString = i18np("1 Group", "%1 Groups", groups);
	
	return i18nc("%1 = something like 7 keys, %2 = something like 2 groups", "%1, %2", keyString, groupString);
	
}

KGpgGroupNode *
KGpgItemModel::addGroup(const QString &name, const KGpgKeyNode::List &keys)
{
	KGpgGroupNode *nd;
	const int cIndex = m_root->getChildCount();	// row of the new node

	beginInsertRows(QModelIndex(), cIndex, cIndex);
	nd = new KGpgGroupNode(m_root, name, keys);
	endInsertRows();

	nd->saveMembers();

	Q_ASSERT(m_root->getChildIndex(nd) == cIndex);

	return nd;
}

void
KGpgItemModel::delNode(KGpgNode *node)
{
	beginResetModel();
	delete node;
	endResetModel();
}

void
KGpgItemModel::changeGroup(KGpgGroupNode *node, const QList<KGpgNode *> &keys)
{
	const QModelIndex gIndex = nodeIndex(node);
	for (int i = node->getChildCount() - 1; i >= 0; i--) {
		bool found = false;

		for (int j = 0; j < keys.count(); j++) {
			found = (node->getChild(i)->getId() == keys.at(j)->getId());
			if (found)
				break;
		}
		if (found)
			continue;

		beginRemoveRows(gIndex, i, i);
		delete node->getChild(i);
		endRemoveRows();
	}

	int cnt = node->getChildCount();

	for (int i = 0; i < keys.count(); i++) {
		bool found = false;
		for (int j = 0; j < cnt; j++) {
			found = (node->getChild(j)->getId() == keys.at(i)->getId());
			if (found)
				break;
		}
		if (found)
			continue;

		beginInsertRows(gIndex, cnt, cnt);
		new KGpgGroupMemberNode(node, keys.at(i)->toKeyNode());
		endInsertRows();
		cnt++;
	}

	node->saveMembers();
}

void
KGpgItemModel::deleteFromGroup(KGpgGroupNode *group, KGpgGroupMemberNode *member)
{
	Q_ASSERT(group == member->getParentKeyNode());

	const int childRow = group->getChildIndex(member);
	const QModelIndex pIndex = nodeIndex(group);

	beginRemoveRows(pIndex, childRow, childRow);
	delete member;
	endRemoveRows();

	group->saveMembers();
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
	case KEYCOLUMN_EMAIL:	return QString(i18nc("@title:column Title of a column of emails", "Email"));
	case KEYCOLUMN_TRUST:	return QString(i18n("Trust"));
	case KEYCOLUMN_SIZE:	return QString(i18n("Size"));
	case KEYCOLUMN_EXPIR:	return QString(i18n("Expiration"));
	case KEYCOLUMN_CREAT:	return QString(i18n("Creation"));
	case KEYCOLUMN_ID:	return QString(i18n("ID"));
	default:	return QVariant();
	}
}

void
KGpgItemModel::setDefaultKey(KGpgKeyNode *def)
{
	int defrow = m_root->findKeyRow(def);
	int odefrow = m_root->findKeyRow(m_default);
	if (defrow == odefrow)
		return;

	int lastcol = columnCount(QModelIndex()) - 1;
	if (odefrow >= 0) {
		KGpgNode *nd = m_root->getChild(odefrow);
		emit dataChanged(createIndex(odefrow, 0, nd), createIndex(odefrow, lastcol, nd));
	}

	if (def) {
		m_default = def->getId();
		emit dataChanged(createIndex(defrow, 0, def), createIndex(defrow, lastcol, def));
	} else {
		m_default.clear();
	}
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
	KGpgKeyNode::List nodes;

	nodes.append(nd);

	refreshKeyIds(nodes);
}

void
KGpgItemModel::refreshKey(const QString &id)
{
	refreshKeyIds(QStringList(id));
}

void
KGpgItemModel::refreshKeys(KGpgKeyNode::List keys)
{
	refreshKeyIds(keys);
}

void
KGpgItemModel::refreshKeys(const QStringList &ids)
{
	refreshKeyIds(ids);
}

void
KGpgItemModel::refreshKeyIds(const QStringList &ids)
{
	beginResetModel();
	if (ids.isEmpty()) {
		for (int i = m_root->getChildCount() - 1; i >= 0; i--) {
			KGpgNode *nd = m_root->getChild(i);
			if (nd->getType() == ITYPE_GROUP)
				continue;
			delete nd;
		}
		m_root->addKeys();
	} else {
		QStringList::ConstIterator it = ids.constBegin();
		const QStringList::ConstIterator itEnd = ids.constEnd();

		KGpgKeyNode::List refreshNodes;
		QStringList addIds;

		for (; it != itEnd; ++it) {
			KGpgKeyNode *nd = m_root->findKey(*it);
			if (nd)
				refreshNodes << nd;
			else
				addIds << *it;
		}

		if (!refreshNodes.isEmpty())
			m_root->refreshKeys(refreshNodes);
		if (!addIds.isEmpty())
			m_root->addKeys(addIds);
	}

	endResetModel();
}

void
KGpgItemModel::refreshKeyIds(KGpgKeyNode::List &nodes)
{
	beginResetModel();
	m_root->refreshKeys(nodes);
	endResetModel();
}

void
KGpgItemModel::refreshGroups()
{
	for (int i = m_root->getChildCount() - 1; i >= 0; i--) {
		KGpgNode *nd = m_root->getChild(i);
		if (nd->getType() != ITYPE_GROUP)
			continue;

		beginRemoveRows(QModelIndex(), i, i);
		delete nd;
		endRemoveRows();
	}

	const QStringList groups = KGpgGroupNode::readGroups();

	if (groups.isEmpty())
		return;

	const int oldCount = m_root->getChildCount();
	beginInsertRows(QModelIndex(), oldCount, oldCount + groups.count());
	m_root->addGroups(groups);
	endInsertRows();
}

bool
KGpgItemModel::isDefaultKey(const KGpgNode *node) const
{
	return !m_default.isEmpty() && (m_default == node->getId().right(m_default.length()));
}

void
KGpgItemModel::invalidateIndexes(KGpgNode *nd)
{
	foreach (const QModelIndex &idx, persistentIndexList()) {
		KGpgNode *n = nodeForIndex(idx);

		if (n != nd)
			continue;

		changePersistentIndex(idx, QModelIndex());
	}
}

void
KGpgItemModel::refreshTrust(const KgpgCore::KgpgKeyTrust trust, const QColor& color)
{
	updateNodeTrustColor(m_root, trust, color);
}

void
KGpgItemModel::updateNodeTrustColor(KGpgExpandableNode *node, const KgpgCore::KgpgKeyTrust trust, const QColor &color)
{
	for (int i = 0; i < node->getChildCount(); i++) {
		KGpgNode *child = node->getChild(i);

		if (child->getTrust() == trust)
			emit dataChanged(createIndex(i, KEYCOLUMN_TRUST, child), createIndex(i, KEYCOLUMN_TRUST, child));

		if (!child->hasChildren())
			continue;

		KGpgExpandableNode *echild = child->toExpandableNode();
		if (echild->wasExpanded())
			updateNodeTrustColor(echild, trust, color);
	}
}

#include "kgpgitemmodel.moc"
