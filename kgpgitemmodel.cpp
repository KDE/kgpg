#include "kgpgitemmodel.h"
#include "kgpgitemnode.h"
#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "convert.h"

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

QVariant
KGpgItemModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	KGpgNode *node = nodeForIndex(index);

	if (index.column() == 2) {
		KgpgKeyTrust t = node->getTrust();

		switch (role) {
		case Qt::BackgroundColorRole:	return Convert::toColor(t);
		case Qt::AccessibleTextRole:	return Convert::toString(t);
		default:	return QVariant();
		}
	}

	switch (role) {
	case Qt::FontRole: {
			QFont f;
			f.setBold(node == m_default);
			return f;
	}
	case Qt::DecorationRole:
			if (index.column() == 0) {
				if ((node->getType() == ITYPE_UAT) && (m_previewsize > 0)) {
					KGpgUatNode *nd = static_cast<KGpgUatNode *>(node);
					return nd->getPixmap().scaled(m_previewsize + 5, m_previewsize, Qt::KeepAspectRatio);
				}
				return Convert::toPixmap(node->getType());
			} else {
				return QVariant();
			}
	case Qt::DisplayRole:
			return node->getData(index.column());
	default:	return QVariant();
	}
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
