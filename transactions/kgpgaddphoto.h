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

#ifndef KGPGADDPHOTO_H
#define KGPGADDPHOTO_H

#include <QObject>

#include "kgpgeditkeytransaction.h"

class QString;

class KGpgAddPhoto: public KGpgEditKeyTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgAddPhoto)
public:
	KGpgAddPhoto(QObject *parent, const QString &keyid, const QString &imagepath);
	virtual ~KGpgAddPhoto();

	void setImagePath(const QString &imagepath);

protected:
	virtual bool nextLine(const QString &line);

private:
	QString m_photourl;
};

#endif // KGPGADDPHOTO_H
