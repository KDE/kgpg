/*
 * Copyright (C) 2009 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGSIGNKEY_H
#define KGPGSIGNKEY_H

#include <QList>
#include <QObject>

#include "kgpgeditkeytransaction.h"
#include "kgpgsigntransactionhelper.h"

class KGpgKeyNode;

/**
 * @brief transaction class to sign all user ids of a key
 */
class KGpgSignKey: public KGpgEditKeyTransaction, public KGpgSignTransactionHelper {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgSignKey)
	KGpgSignKey(); // = delete C++0x

public:
	/**
	 * @brief constructor
	 * @param parent parent object
	 * @param signer id of the key to sign with
	 * @param key node to sign
	 * @param local if signature should be local (not exportable)
	 * @param checking how carefully the identity of the key owner was checked
	 *
	 * See setUid() for description of uid.
	 */
	KGpgSignKey(QObject *parent, const QString &signer, KGpgKeyNode *key, const bool local, const carefulCheck checking);
	/**
	 * @brief destructor
	 */
	virtual ~KGpgSignKey();

protected:
	virtual bool nextLine(const QString &line);
	virtual ts_boolanswer boolQuestion(const QString &line);
	virtual bool passphraseReceived();

	virtual KGpgTransaction *asTransaction();
	virtual void replaceCmd(const QString &cmd);
};

#endif // KGPGSIGNKEY_H
