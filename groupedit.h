/**
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
