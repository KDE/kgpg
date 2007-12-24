#include "kgpgitemmodel.h"
#include "kgpgitemnode.h"
#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "convert.h"
#include <KLocale>

KGpgItemModel::KGpgItemModel(QObject *parent)
	: QAbstractItemModel(parent), m_root(new KGpgRootNode()), m_default(NULL), m_previewsize(0)
{
	m_root->addKeys();

	QString defaultkey = KGpgSettings::defaultKey();
	m_default = m_root->findKey(defaultkey);
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
	KGpgNode *childNode = nodeForIndex(child);
	KGpgNode *parentNode = childNode->m_parent;

	if (parentNode == m_root)
		return QModelIndex();

	int row = rowForNode(parentNode);
	int column = 0;
	
	return createIndex(row, column, parentNode);
}

int
KGpgItemModel::rowCount(const QModelIndex &parent) const
{
	KGpgNode *parentNode = nodeForIndex(parent);

	return parentNode->getChildCount();
}

bool
KGpgItemModel::hasChildren(const QModelIndex &parent) const
{
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
		f.setBold(node == m_default);
		return f;
	}

	switch (index.column()) {
	case KEYCOLUMN_NAME:
		switch (role) {
		case Qt::DisplayRole:
			return node->getName();
		case Qt::DecorationRole:
			if ((node->getType() == ITYPE_UAT) && (m_previewsize > 0)) {
				KGpgUatNode *nd = static_cast<KGpgUatNode *>(node);
				return nd->getPixmap().scaled(m_previewsize + 5, m_previewsize, Qt::KeepAspectRatio);
			}
			return Convert::toPixmap(node->getType());
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
		if (role == Qt::DisplayRole)
			return node->getSize();
		break;
	case KEYCOLUMN_CREAT:
		if (role == Qt::DisplayRole)
			return node->getCreation();
		break;
	case KEYCOLUMN_ID:
		if (role == Qt::DisplayRole)
			return node->getId().right(8);
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

void
KGpgItemModel::addGroup(const QString &name, const KGpgKeyNodeList &keys)
{
	emit layoutAboutToBeChanged();
	new KGpgGroupNode(m_root, name);
	emit layoutChanged();
}

void
KGpgItemModel::delGroup(const KGpgNode *node)
{
	emit layoutAboutToBeChanged();
	delete node;
	emit layoutChanged();
}
