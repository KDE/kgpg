/*
 * Copyright (C) 2010,2012,2013 Rolf Eike Beer <kde@opensource.sf-tec.de>
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

#include "kgpgtransaction.h"

#include "core/kgpgkey.h"

#include <KUrl>
#include <QObject>

class QString;

/**
 * @brief generate a revokation certificate
 */
class KGpgGenerateRevoke: public KGpgTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgGenerateRevoke)
	KGpgGenerateRevoke(); // = delete C++0x
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
	virtual ts_boolanswer boolQuestion(const QString &line);
	virtual void finish();
	virtual bool passphraseReceived();
	virtual ts_boolanswer confirmOverwrite (KUrl &currentFile);

private:
	QString m_keyid;
	KUrl m_revUrl;
	int m_reason;
	QString m_description;
	QString m_output;
};

#endif // KGPGGENERATEREVOKE_H
