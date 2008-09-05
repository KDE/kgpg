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

#ifndef KGPGDELUID_H
#define KGPGDELUID_H

#include <QObject>

#include "kgpguidtransaction.h"

class QString;

class KGpgDelUid: public KGpgUidTransaction {
	Q_OBJECT

public:
	KGpgDelUid(QObject *parent, const QString &keyid, const QString &uid);
	virtual ~KGpgDelUid();

protected:
	virtual bool nextLine(const QString &line);
};

#endif // KGPGDELUID_H
