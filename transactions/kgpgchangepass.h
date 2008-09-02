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

#ifndef KGPGCHANGEPASS_H
#define KGPGCHANGEPASS_H

#include <QObject>

#include "kgpgtransaction.h"

class KGpgChangePass: public KGpgTransaction {
	Q_OBJECT

public:
	KGpgChangePass(QObject *parent, const QString &keyid);
	virtual ~KGpgChangePass();

protected:
	virtual bool nextLine(const QString &line);
	virtual void preStart();
};

#endif // KGPGCHANGEPASS_H
