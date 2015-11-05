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

#ifndef KGPGSIGNUID_H
#define KGPGSIGNUID_H

#include <QObject>

#include "kgpguidtransaction.h"
#include "kgpgsigntransactionhelper.h"

class KGpgSignableNode;
class QString;

/**
 * @brief transaction class to sign a single user id of a key
 */
class KGpgSignUid: public KGpgUidTransaction, public KGpgSignTransactionHelper {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgSignUid)
	KGpgSignUid() Q_DECL_EQ_DELETE;

public:
	/**
	 * @brief constructor
	 * @param parent parent object
	 * @param signer id of the key to sign with
	 * @param uid node to sign
	 * @param local if signature should be local (not exportable)
	 * @param checking how carefully the identity of the key owner was checked
	 *
	 * See setUid() for description of uid.
	 */
	KGpgSignUid(QObject *parent, const QString &signer, const KGpgSignableNode *uid, const bool local, const carefulCheck checking);
	/**
	 * @brief destructor
	 */
	virtual ~KGpgSignUid();

	/**
	 * @brief set node to sign
	 * @param uid node to sign
	 *
	 * If uid is a KGpgKeyNode only the primary id of that key is
	 * signed. If you want to sign all user ids at once use
	 * KGpgSignKey instead. Legal values for uid are also KGpgUidNode
	 * and KGpgUatNode.
	 */
	void setUid(const KGpgSignableNode *uid);

protected:
	virtual bool preStart() Q_DECL_OVERRIDE;
	virtual bool nextLine(const QString &line) Q_DECL_OVERRIDE;
	virtual ts_boolanswer boolQuestion(const QString &line) Q_DECL_OVERRIDE;
	virtual bool passphraseReceived() Q_DECL_OVERRIDE;

	virtual KGpgTransaction *asTransaction() Q_DECL_OVERRIDE;
	virtual void replaceCmd(const QString &cmd) Q_DECL_OVERRIDE;

private:
	int m_cmdPos;		///< position of the command in GnuPG command line
};

#endif // KGPGSIGNUID_H
