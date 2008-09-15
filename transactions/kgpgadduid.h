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

#ifndef KGPGADDUID_H
#define KGPGADDUID_H

#include <QObject>

#include "kgpgtransaction.h"

class QString;

/**
 * \brief add a new user id to a key pair
 */
class KGpgAddUid: public KGpgTransaction {
	Q_OBJECT

public:
	KGpgAddUid(QObject *parent, const QString &keyid, const QString &name, const QString &email = QString(), const QString &comment = QString());
	virtual ~KGpgAddUid();

	void setName(const QString &name);
	void setEmail(const QString &email);
	void setComment(const QString &comment);

	QString getKeyid() const;

protected:
	virtual bool preStart();
	virtual bool nextLine(const QString &line);

private:
	QString m_name;
	QString m_email;
	QString m_comment;
	QString m_keyid;
};

#endif // KGPGADDPHOTO_H
