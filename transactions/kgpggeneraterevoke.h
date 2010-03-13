/*
 * Copyright (C) 2010 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGGENERATEREVOKE_H
#define KGPGGENERATEREVOKE_H

#include <QObject>
#include <KUrl>

#include "kgpgtransaction.h"
#include "kgpgkey.h"

class QString;

/**
 * @brief generate a revokation certificate
 */
class KGpgGenerateRevoke: public KGpgTransaction {
	Q_OBJECT

public:
	/**
	 * @brief KGpgGenerateRevoke's constructor
	 * @param parent parent object
	 * @param id key fingerprint to generate revokation certificate for
	 * @param revokeUrl place to store the certificate (may be empty)
	 * @param reason revokation reason
	 * @param description text description for revokation
	 */
	KGpgGenerateRevoke(QObject *parent, const QString &keyID, const KUrl &revokeUrl, const int reason, const QString &description);
	virtual ~KGpgGenerateRevoke();

	/**
	 * @brief returns the revokation certificate
	 */
	const QString &getOutput() const;

signals:
	void revokeCertificate(const QString &cert);

protected:
	virtual bool preStart();
	virtual bool nextLine(const QString &line);
	virtual void finish();

private:
	QString m_keyid;
	KUrl m_revUrl;
	int m_reason;
	QString m_description;
	QString m_output;
};

#endif // KGPGGENERATEREVOKE_H
