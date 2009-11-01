/**
 * Copyright (C) 2008,2009 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGDELUID_H
#define KGPGDELUID_H

#include <QList>
#include <QObject>

#include "kgpguidtransaction.h"

class KGpgSignableNode;
class KGpgKeyNode;
class KGpgUidNode;

class KGpgDelUid: public KGpgUidTransaction {
	Q_OBJECT

public:
	KGpgDelUid(QObject *parent, const KGpgSignableNode *uid);
	/**
	 * @brief constructor
	 * @param parent parent object
	 * @param uids user ids to delete
	 *
	 * All entries in @uids must be children of the same keynode. The keynode itself
	 * may be part of the list, representing the primary user id. The nodes must be
	 * either the keynode itself, user id nodes, or user attribute nodes.
	 */
	KGpgDelUid(QObject *parent, const QList<const KGpgSignableNode *> &uids);
	/**
	 * @brief constructor
	 * @param parent parent object
	 * @param keynode key to edit
	 * @param uid uid to delete, negative to delete all others
	 */
	KGpgDelUid(QObject *parent, const KGpgKeyNode *keynode, const int uid);
	/**
	 * @brief destructor
	 */
	virtual ~KGpgDelUid();

	void setUid(const KGpgSignableNode *uid);
	void setUids(const QList<const KGpgSignableNode *> &uids);
	void setUids(const KGpgKeyNode *keynode, const int uid);
	
protected:
	virtual bool nextLine(const QString &line);
	virtual void finish();

private:
	int m_fixargs;
	QList<const KGpgSignableNode *> m_uids;
};

#endif // KGPGDELUID_H
