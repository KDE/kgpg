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

#ifndef KGPGCHANGEEXPIRE_H
#define KGPGCHANGEEXPIRE_H

#include <QObject>
#include <QDate>

#include "kgpgeditkeytransaction.h"

/**
 * @brief change the key lifetime
 */
class KGpgChangeExpire: public KGpgEditKeyTransaction {
	Q_OBJECT

public:
	KGpgChangeExpire(QObject *parent, const QString &keyid, const QDate &date);
	virtual ~KGpgChangeExpire();

	void setDate(const QDate &date);

protected:
	virtual bool nextLine(const QString &line);
	virtual void finish();

private:
	QDate m_date;
};

#endif // KGPGCHANGEEXPIRE_H
