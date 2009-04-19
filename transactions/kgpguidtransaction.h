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

#ifndef KGPGUIDTRANSACTION_H
#define KGPGUIDTRANSACTION_H

#include <QObject>
#include <QString>

#include "kgpgtransaction.h"

/**
 * \brief base class for transactions involving only one user id of a key
 */
class KGpgUidTransaction: public KGpgTransaction {
	Q_OBJECT

public:
	/**
	 * \brief construct a new transaction for the given key and uid
	 * @param parent object that own the transaction
	 * @param keyid key to work with
	 * @param uid uid to work with
	 *
	 * You should call this from the childrens constructor to set up
	 * everything properly
	 */
	KGpgUidTransaction(QObject *parent, const QString &keyid, const QString &uid);
	virtual ~KGpgUidTransaction();

	/**
	 * \brief set the uid number of the transaction to the given value
	 * @param uid the number of the user id to work with
	 */
	void setUid(const QString &uid);
	/**
	 * \brief set the uid number of the transaction to the given value
	 * @param uid the number of the user id to work with
	 *
	 * This is an overloaded member function, provided for convenience.
	 */
	void setUid(const unsigned int uid);

protected:
	virtual bool preStart();

	/**
	 * \brief handle common GnuPG messages for uid transactions
	 * @param line GnuPG message
	 * @return true if "quit" should be sent to process
	 *
	 * You should call these function for all messages in nextLine()
	 * you do not need to handle yourself.
	 */
	bool standardCommands(const QString &line);

private:
	/**
	 * \brief forbidden
	 */
	explicit KGpgUidTransaction(QObject *parent = 0);

	QString m_uid;
	int m_uidpos;
};

#endif // KGPGUIDTRANSACTION_H
