#ifndef KGPGITEMMODEL_H
#define KGPGITEMMODEL_H

#include <QAbstractItemModel>

#include "kgpgitemnode.h"
#include "keyinfodialog.h"

class KGpgItemModel : public QAbstractItemModel
{
	Q_OBJECT

private:
	KGpgRootNode *m_root;
	KGpgKeyNode *m_default;
	unsigned int m_previewsize;

public:

	explicit KGpgItemModel(QObject *parent = 0);
	virtual ~KGpgItemModel();

	virtual QModelIndex index(int row, int column,
				const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex &child) const;

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex & /*parent = QModelIndex()*/ ) const
			{ return 8; }

	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual bool hasChildren(const QModelIndex &parent) const;

	void setPreviewSize(const unsigned int &n)
		{ m_previewsize = n; }
	KGpgNode *nodeForIndex(const QModelIndex &index) const;

	KGpgRootNode *getRootNode() const;
protected:
	int rowForNode(KGpgNode *node) const;
};

#endif
