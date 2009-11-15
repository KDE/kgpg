/**
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

#ifndef KGPGPRIMARYUID_H
#define KGPGPRIMARYUID_H

#include <QList>
#include <QObject>

#include "kgpguidtransaction.h"

class KGpgUidNode;

/**
 * @brief transaction class to change the primary user id of a key
 */
class KGpgPrimaryUid: public KGpgUidTransaction {
	Q_OBJECT

public:
	/**
	 * @brief constructor
	 * @param parent parent object
	 * @param uids user ids to become new primary
	 */
	KGpgPrimaryUid(QObject *parent, KGpgUidNode *uid);
	/**
	 * @brief destructor
	 */
	virtual ~KGpgPrimaryUid();

protected:
	virtual bool nextLine(const QString &line);

private:
	int m_fixargs;
};

#endif // KGPGPRIMARYUID_H
