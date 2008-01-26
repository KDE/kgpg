#ifndef GROUPEDIT_H
#define GROUPEDIT_H

#include <QList>
#include "ui_groupedit.h"

class GroupEditProxyModel;
class KGpgNode;
class KGpgItemModel;

class groupEdit : public QWidget, public Ui::groupEdit
{
	Q_OBJECT

private:
	GroupEditProxyModel *m_in;
	GroupEditProxyModel *m_out;
	KGpgItemModel *m_model;

public:
	QList<KGpgNode *> *members;

	explicit groupEdit(QWidget *parent, QList<KGpgNode *> *ids);
	~groupEdit();

	void setModel(KGpgItemModel *md);

private Q_SLOTS:
	void groupAdd();
	void groupRemove();
	void groupAdd(const QModelIndex &index);
	void groupRemove(const QModelIndex &index);
};

#endif
