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

#ifndef KGPGDELKEY_H
#define KGPGDELKEY_H

#include <QObject>

#include "kgpgtransaction.h"

class QString;
class QStringList;

/**
 * \brief delete a public key
 */
class KGpgDelKey: public KGpgTransaction {
	Q_OBJECT

public:
	KGpgDelKey(QObject *parent, const QString &keyid);
	KGpgDelKey(QObject *parent, const QStringList &keyids);
	virtual ~KGpgDelKey();

	void setDelKey(const QString &keyid);
	void setDelKeys(const QStringList &keyids);

protected:
	virtual bool nextLine(const QString &line);
	virtual bool preStart();
	virtual void finish();
	
private:
	QStringList m_keyids;
	int m_argscount;

	void setCmdLine();
};

#endif // KGPGDELKEY_H
