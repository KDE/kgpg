#include <QSortFilterProxyModel>

class KGpgNode;
class KGpgItemModel;

class KeyListProxyModel: public QSortFilterProxyModel
{
public:
	KeyListProxyModel(QObject * parent = 0);

	virtual bool hasChildren(const QModelIndex &idx) const;
	void setKeyModel(KGpgItemModel *);
	void setOnlySecret(const bool &b);
	void setShowExpired(const bool &b);

protected:
	virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
	virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
	bool lessThan(const KGpgNode *left, const KGpgNode *right) const;
	KGpgItemModel *m_model;
	bool m_onlysecret;
	bool m_showexpired;
};
