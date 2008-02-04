#ifndef SELECTKEYPROXYMODEL_H
#define SELECTKEYPROXYMODEL_H

#include <QSortFilterProxyModel>

class KGpgNode;
class KGpgItemModel;

class SelectKeyProxyModel: public QSortFilterProxyModel
{
	Q_PROPERTY(bool showUntrusted read showUntrusted write setShowUntrusted)

public:
	explicit SelectKeyProxyModel(QObject * parent);

	void setKeyModel(KGpgItemModel *);

	KGpgNode *nodeForIndex(const QModelIndex &index) const;

	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	inline bool showUntrusted() const
		{ return m_showUntrusted; }
	void setShowUntrusted(const bool &b);

protected:
	virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
	virtual int columnCount(const QModelIndex &) const;

private:
	KGpgItemModel *m_model;
	bool m_showUntrusted;
};

#endif
