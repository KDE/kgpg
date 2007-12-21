#include <QSortFilterProxyModel>

class KGpgNode;
class KGpgItemModel;

class KeyListProxyModel: public QSortFilterProxyModel
{
public:
	KeyListProxyModel(QObject * parent = 0);

	virtual bool hasChildren(const QModelIndex &idx) const;
	void setKeyModel(KGpgItemModel *);

protected:
	virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
	virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
	bool lessThan(const KGpgNode *left, const KGpgNode *right) const;
	KGpgItemModel *m_model;
};
