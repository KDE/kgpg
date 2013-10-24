/*
 * Copyright (C) 2008,2009,2012,2013 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGGENERATEKEY_H
#define KGPGGENERATEKEY_H

#include "kgpgtransaction.h"

#include "core/kgpgkey.h"

#include <QObject>

class QString;

/**
 * @brief generate a new key pair
 */
class KGpgGenerateKey: public KGpgTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgGenerateKey)
	KGpgGenerateKey(); // = delete C++0x
public:
	enum ts_generatekey {
		TS_INVALID_NAME = TS_COMMON_END + 1	///< the owners name is not accepted by GnuPG
	};

	/**
	 * @brief KGpgGenerateKey's constructor
	 * @param parent parent object
	 * @param name the name of the key, it is also the user's name.
	 * @param email email MUST be a valid email address or an empty string.
	 * @param comment is a comment, it can be an empty string
	 * @param algorithm this is the type of the key, RSA or DSA & ELGAMAL (\see Kgpg::KeyAlgo ).
	 * @param size this is the length of the key (1024, 2048, ...)
	 * @param expire defines the key expiry time together with \em expireunit, 0 for unlimited key lifetime
	 * @param expireunit is the unit of the number given as \em expire. Valid units are 'd', 'w', 'm' and 'y'. The unit is ignored if expire is 0.
	 * @param capabilities capabilities for the primary key
	 */
	KGpgGenerateKey(QObject *parent, const QString &name, const QString &email, const QString &comment,
			 const KgpgCore::KgpgKeyAlgo &algorithm, const uint size, const unsigned int expire = 0,
			 const char expireunit = 'd', const KgpgCore::KgpgSubKeyType capabilities = 0);
	virtual ~KGpgGenerateKey();

	QString getName() const;
	QString getEmail() const;

	/**
	 * @brief return the fingerprint of the generated key
	 */
	QString getFingerprint() const;

	/**
	 * @brief get error output of GnuPG
	 * @return the messages GnuPG printed to standard error
	 *
	 * This will only return data after the done() signal has been emitted.
	 */
	QString gpgErrorMessage() const;

protected:
	virtual bool preStart();
	virtual void postStart();
	virtual bool nextLine(const QString &line);
	virtual void finish();
	virtual void newPassphraseEntered();

private:
	const QString m_name;
	const QString m_email;
	const QString m_comment;
	const KgpgCore::KgpgKeyAlgo m_algorithm;
	const KgpgCore::KgpgSubKeyType m_capabilities;
	const unsigned int m_size;
	const unsigned int m_expire;
	const unsigned int m_expireunit;
	QString m_fingerprint;
	QString m_errorOutput;
};

#endif // KGPGGENERATEKEY_H
