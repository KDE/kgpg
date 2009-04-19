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

#ifndef KGPGGENERATEKEY_H
#define KGPGGENERATEKEY_H

#include <QObject>

#include "kgpgtransaction.h"
#include "kgpgkey.h"

class QString;

/**
 * \brief generate a new key pair
 */
class KGpgGenerateKey: public KGpgTransaction {
	Q_OBJECT

public:
	enum ts_generatekey {
		TS_INVALID_NAME = TS_COMMON_END + 1	///< the owners name is not accepted by GnuPG
	};
	/**
	 * \brief KGpgGenerateKey's constructor
	 * @param name the name of the key, it is also the user's name.
	 * @param email email MUST be a valid email address or an empty string.
	 * @param comment is a comment, it can be an empty string
	 * @param algorithm this is the type of the key, RSA or DSA & ELGAMAL (see Kgpg::KeyAlgo).
	 * @param size this is the length of the key (1024, 2048, ...)
	 * @param expire defines the key expiry time together with \em expireunit, 0 for unlimited key lifetime
	 * @param expireunit is the unit of the number given as \em expire. See setExpire(const unsigned int &expire, const unsigned int &expireunit)
	 */
	KGpgGenerateKey(QObject *parent, const QString &name, const QString &email, const QString &comment,
			 const KgpgCore::KgpgKeyAlgo &algorithm, const uint &size, const unsigned int &expire = 0,
			 const unsigned int &expireunit = 1);
	virtual ~KGpgGenerateKey();

	void setName(const QString &name);
	QString getName() const;
	void setEmail(const QString &email);
	QString getEmail() const;
	void setComment(const QString &comment);
	void setAlgorithm(const KgpgCore::KgpgKeyAlgo &algorithm);
	void setSize(const unsigned int &size);
	/**
	 * \brief set expire date for key
	 * @param expire defines the key expiry time together with \em expireunit, 0 for unlimited key lifetime
	 * @param expireunit is the unit of the number given as \em expire.
	 * - 1 = day
	 * - 2 = week
	 * - 3 = month
	 * - 4 = year
	 */
	void setExpire(const unsigned int &expire, const unsigned int &expireunit);

	QString getFingerprint() const;

protected:
	virtual bool preStart();
	virtual bool nextLine(const QString &line);

private:
	QString m_name;
	QString m_email;
	QString m_comment;
	KgpgCore::KgpgKeyAlgo m_algorithm;
	unsigned int m_size;
	unsigned int m_expire;
	unsigned int m_expireunit;
	QString m_fingerprint;
	bool m_started;
	bool m_namesent;

Q_SIGNALS:
	void generateKeyStarted();
};

#endif // KGPGGENERATEKEY_H
