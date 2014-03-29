/*
 * Copyright (C) 2008,2009,2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGCHANGETRUST_H
#define KGPGCHANGETRUST_H

#include "kgpgeditkeytransaction.h"

#include "core/kgpgkey.h"

#include <gpgme.h>
#include <QObject>

/**
 * @brief change the owner trust level of a public key
 */
class KGpgChangeTrust: public KGpgEditKeyTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgChangeTrust)
public:
	KGpgChangeTrust(QObject *parent, const QString &keyid, const gpgme_validity_t trust);
	virtual ~KGpgChangeTrust();

	void setTrust(const gpgme_validity_t trust);

protected:
	virtual bool nextLine(const QString &line);
	virtual ts_boolanswer boolQuestion(const QString &line);
	virtual bool preStart();

private:
	gpgme_validity_t m_trust;
};

#endif // KGPGCHANGETRUST_H
