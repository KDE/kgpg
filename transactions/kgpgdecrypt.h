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

#ifndef KGPGDECRYPT_H
#define KGPGDECRYPT_H

#include <QObject>

#include <KUrl>

#include "kgpgtextorfiletransaction.h"

class QStringList;

/**
 * @brief import one or more keys into the keyring
 */
class KGpgDecrypt: public KGpgTextOrFileTransaction {
	Q_OBJECT

public:
	/**
	 * @brief decrypt given text
	 * @param parent parent object
	 * @param text text to decrypt
	 */
	KGpgDecrypt(QObject *parent, const QString &text = QString());

	/**
	 * @brief decrypt file(s)
	 * @param parent parent object
	 * @param files list of file locations to decrypt
	 */
	KGpgDecrypt(QObject *parent, const KUrl::List &files);

	/**
	 * @brief destructor
	 */
	virtual ~KGpgDecrypt();

	/**
	 * @brief get decryption result
	 * @return decrypted text
	 */
	QStringList decryptedText() const;

	/**
	 * @brief check if the given text contains an encoded message
	 * @param text text to check
	 * @param startPos if not NULL start offset of encoded text will be returned here
	 * @param endPos if not NULL end offset of encoded text will be returned here
	 */
	static bool isEncryptedText(const QString &text, int *startPos = NULL, int *endPos = NULL);

protected:
	virtual QStringList command() const;
};

#endif // KGPGDECRYPT_H
