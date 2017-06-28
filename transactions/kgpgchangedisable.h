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
	KGpgChangeDisable() Q_DECL_EQ_DELETE;

public:
	KGpgChangeDisable(QObject *parent, const QString &keyid, const bool disable);
	virtual ~KGpgChangeDisable();

	void setDisable(bool disable);

protected:
	bool preStart() Q_DECL_OVERRIDE;
	bool nextLine(const QString &line) Q_DECL_OVERRIDE;
};

#endif // KGPGCHANGEDISABLE_H
