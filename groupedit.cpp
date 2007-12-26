#include "groupedit.h"
#include "groupeditproxymodel.h"
#include "kgpgitemmodel.h"

#include <QHeaderView>

groupEdit::groupEdit(QWidget *parent, QList<KGpgNode *> *ids)
	: QWidget( parent ), members(ids)
{
	setupUi( this );
	m_in = new GroupEditProxyModel(this, false, members);
	m_out = new GroupEditProxyModel(this, true, members);
	availableKeys->setModel(m_out);
	groupKeys->setModel(m_in);
	buttonAdd->setIcon(KIcon("go-down"));
	buttonRemove->setIcon(KIcon("go-up"));

	availableKeys->setColumnWidth(0, 200);
	availableKeys->setColumnWidth(1, 200);
	availableKeys->setColumnWidth(2, 100);
	availableKeys->verticalHeader()->hide();

	groupKeys->setColumnWidth(0, 200);
	groupKeys->setColumnWidth(1, 200);
	groupKeys->setColumnWidth(2, 100);
	groupKeys->verticalHeader()->hide();

	setMinimumSize(sizeHint());

	connect(buttonAdd, SIGNAL(clicked()), this, SLOT(groupAdd()));
	connect(buttonRemove, SIGNAL(clicked()), this, SLOT(groupRemove()));
	connect(availableKeys, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(groupAdd(const QModelIndex &)));
	connect(groupKeys, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(groupRemove(const QModelIndex &)));
}

groupEdit::~groupEdit()
{
	delete m_in;
	delete m_out;
}

void
groupEdit::setModel(KGpgItemModel *md)
{
	m_model = md;
	m_in->setKeyModel(md);
	m_out->setKeyModel(md);
}

void
groupEdit::groupAdd()
{
	QModelIndexList sel = availableKeys->selectionModel()->selectedIndexes();
	for (int i = 0; i < sel.count(); i++) {
		if (sel.at(i).column() != 0)
			continue;
		KGpgNode *nd = m_out->nodeForIndex(sel.at(i));
		members->append(nd);
	}
	m_in->invalidate();
	m_out->invalidate();
}

void
groupEdit::groupRemove()
{
	Q_ASSERT(!members->isEmpty());
	QModelIndexList sel = groupKeys->selectionModel()->selectedIndexes();
	for (int i = 0; i < sel.count(); i++) {
		if (sel.at(i).column() != 0)
			continue;
		KGpgNode *nd = m_in->nodeForIndex(sel.at(i));
		for (int j = 0; j < members->count(); j++)
			if (members->at(j)->getId() == nd->getId()) {
				members->removeAt(j);
				break;
			}
	}
	m_in->invalidate();
	m_out->invalidate();
}

void
groupEdit::groupAdd(const QModelIndex &index)
{
	KGpgNode *nd = m_out->nodeForIndex(index);
	members->append(nd);
	m_in->invalidate();
	m_out->invalidate();
}

void
groupEdit::groupRemove(const QModelIndex &index)
{
	Q_ASSERT(!members->isEmpty());
	KGpgNode *nd = m_in->nodeForIndex(index);
	for (int i = 0; i < members->count(); i++)
		if (members->at(i)->getId() == nd->getId()) {
			members->removeAt(i);
			break;
		}
	m_in->invalidate();
	m_out->invalidate();
}
