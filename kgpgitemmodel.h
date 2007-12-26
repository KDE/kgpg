#ifndef KGPGITEMMODEL_H
#define KGPGITEMMODEL_H

#include <QAbstractItemModel>

#define KEYCOLUMN_NAME	0
#define KEYCOLUMN_EMAIL	1
#define KEYCOLUMN_TRUST	2
#define KEYCOLUMN_EXPIR	3
#define KEYCOLUMN_SIZE	4
#define KEYCOLUMN_CREAT	5
#define KEYCOLUMN_ID	6

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
			{ return 7; }

	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual bool hasChildren(const QModelIndex &parent) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	void setPreviewSize(const unsigned int &n)
		{ m_previewsize = n; }
	KGpgNode *nodeForIndex(const QModelIndex &index) const;

	KGpgRootNode *getRootNode() const;
	QString statusCountMessage() const;

public Q_SLOTS:
	void addGroup(const QString &name, const KGpgKeyNodeList &keys);
	void delGroup(const KGpgNode *node);
	void changeGroup(KGpgGroupNode *node, const QList<KGpgNode *> &keys);
	void setDefaultKey(const QString &def);
	QModelIndex nodeIndex(KGpgNode *node);

protected:
	int rowForNode(KGpgNode *node) const;
};

#endif
