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

#ifndef KGPGCHANGETRUST_H
#define KGPGCHANGETRUST_H

#include <QObject>

#include "kgpgtransaction.h"
#include "kgpgkey.h"

/**
 * @brief change the owner trust level of a public key
 */
class KGpgChangeTrust: public KGpgTransaction {
	Q_OBJECT

public:
	KGpgChangeTrust(QObject *parent, const QString &keyid, const KgpgCore::KgpgKeyOwnerTrust &trust);
	virtual ~KGpgChangeTrust();

	void setTrust(const KgpgCore::KgpgKeyOwnerTrust &trust);

protected:
	virtual bool nextLine(const QString &line);
	virtual bool preStart();

private:
	KgpgCore::KgpgKeyOwnerTrust m_trust;
};

#endif // KGPGCHANGETRUST_H
