#ifndef KEYLISTPROXYMODEL_H
#define KEYLISTPROXYMODEL_H

#include <QSortFilterProxyModel>

class KGpgNode;
class KGpgItemModel;

class KeyListProxyModel: public QSortFilterProxyModel
{
public:
	explicit KeyListProxyModel(QObject * parent = 0);

	virtual bool hasChildren(const QModelIndex &idx) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	void setKeyModel(KGpgItemModel *);
	void setOnlySecret(const bool &b);
	void setShowExpired(const bool &b);

	KGpgNode *nodeForIndex(const QModelIndex &index) const;
	QModelIndex nodeIndex(KGpgNode *node);
	void setPreviewSize(const int &pixel);

protected:
	virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
	virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
	bool lessThan(const KGpgNode *left, const KGpgNode *right, const int &column) const;
	bool nodeLessThan(const KGpgNode *left, const KGpgNode *right, const int &column) const;
	KGpgItemModel *m_model;
	bool m_onlysecret;
	bool m_showexpired;
	int m_previewsize;
};

#endif
