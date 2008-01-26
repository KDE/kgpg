#ifndef GROUPEDITPROXYMODEL_H
#define GROUPEDITPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QStringList>

class KGpgNode;
class KGpgGroupNode;
class KGpgItemModel;

class GroupEditProxyModel: public QSortFilterProxyModel
{
public:
	explicit GroupEditProxyModel(QObject * parent, const bool &invert, QList<KGpgNode *> *ids);

	void setKeyModel(KGpgItemModel *);

	KGpgNode *nodeForIndex(const QModelIndex &index) const;

	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

protected:
	virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
	virtual int columnCount(const QModelIndex &) const;

private:
	KGpgItemModel *m_model;
	bool m_invert;
	QList<KGpgNode *> *m_ids;
};

#endif
