/**
 * Copyright (C) 2008 Rolf Eike Beer <kde@opensource.sf-tec.de>
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

class KGpgUidTransaction: public KGpgTransaction {
	Q_OBJECT

public:
	KGpgUidTransaction(QObject *parent, const QString &keyid, const QString &uid);
	virtual ~KGpgUidTransaction();

	void setUid(const QString &uid);

protected:
	virtual void preStart();

	bool standardCommands(const QString &line);

private:
	QString m_uid;
	int m_uidpos;
};

#endif // KGPGUIDTRANSACTION_H
