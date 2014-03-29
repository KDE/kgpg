/*
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

#ifndef KGPGCHANGEDISABLE_H
#define KGPGCHANGEDISABLE_H

#include <QObject>

#include "kgpgeditkeytransaction.h"

/**
 * @brief enable of disable a key
 */
class KGpgChangeDisable: public KGpgEditKeyTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgChangeDisable)
public:
	KGpgChangeDisable(QObject *parent, const QString &keyid, const bool disable);
	virtual ~KGpgChangeDisable();

	void setDisable(bool disable);

protected:
	virtual bool preStart();
	virtual bool nextLine(const QString &line);
};

#endif // KGPGCHANGEDISABLE_H
