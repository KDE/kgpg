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

#ifndef KGPGADDUID_H
#define KGPGADDUID_H

#include <QObject>

#include "kgpgeditkeytransaction.h"

class QString;

/**
 * @brief add a new user id to a key pair
 */
class KGpgAddUid: public KGpgEditKeyTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgAddUid)
public:
	KGpgAddUid(QObject *parent, const QString &keyid, const QString &name, const QString &email = QString(), const QString &comment = QString());
	virtual ~KGpgAddUid();

	void setName(const QString &name);
	void setEmail(const QString &email);
	void setComment(const QString &comment);

protected:
	virtual bool preStart();
	virtual bool nextLine(const QString &line);

private:
	QString m_name;
	QString m_email;
	QString m_comment;
};

#endif // KGPGADDUID_H
