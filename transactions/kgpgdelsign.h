/*
 * Copyright (C) 2010,2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGDELSIGN_H
#define KGPGDELSIGN_H

#include "kgpguidtransaction.h"

#include "core/KGpgSignNode.h"

#include <QObject>
#include <QString>

/**
 * @brief delete signatures from user ids
 */
class KGpgDelSign: public KGpgUidTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgDelSign)
	KGpgDelSign(); // = delete C++0x
public:
	/**
	 * @brief construct a new transaction to delete signatures
	 * @param parent object that own the transaction
	 * @param signids list of signature ids to remove
	 *
	 * All members of signids need to have the same parent node, i.e.
	 * they not even may be signatures to different uids of the same key.
	 */
	KGpgDelSign(QObject *parent, const KGpgSignNode::List &signids);
	/**
	 * @brief construct a new transaction to delete signatures
	 * @param parent object that own the transaction
	 * @param signid signature to delete
	 */
	KGpgDelSign(QObject *parent, KGpgSignNode *signid);

	virtual ~KGpgDelSign();

	/**
	 * @brief set the ids to delete
	 * @param signids fingerprints of the signatures to delete
	 *
	 * This will replace all previously set signature ids.
	 */
	void setSignIds(const KGpgSignNode::List &keyids);
	/**
	 * @brief set the id to delete
	 * @param keyid fingerprint of the signatures to delete
	 * @overload
	 *
	 * This will replace all previously set signature ids.
	 */
	void setSignId(KGpgSignNode *keyid);
	/**
	 * @brief return the signature ids to delete
	 */
	KGpgSignNode::List getSignIds(void) const;

protected:
	virtual bool nextLine(const QString &line);
	virtual ts_boolanswer boolQuestion(const QString &line);

private:
	KGpgSignNode::List m_signids;	///< the list of ids to delete
	QString m_cachedid;	///< the next id GnuPG will ask to delete
};

#endif // KGPGDELSIGN_H
