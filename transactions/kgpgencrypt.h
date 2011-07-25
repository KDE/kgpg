/*
 * Copyright (C) 2011 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGENCRYPT_H
#define KGPGENCRYPT_H

#include <QObject>
#include <QString>
#include <QStringList>

#include <KUrl>

#include "kgpgtextorfiletransaction.h"

class QProcess;

/**
 * @brief encrypt the given text or files
 */
class KGpgEncrypt: public KGpgTextOrFileTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgEncrypt)
	KGpgEncrypt(); // = delete C++0x
public:
	enum EncryptOption {
		DefaultEncryption = 0,
		AsciiArmored = 0x1,
		AllowUntrustedEncryption = 0x2,
		HideKeyId = 0x4
	};
	Q_DECLARE_FLAGS(EncryptOptions, EncryptOption);

	/**
	 * @brief encrypt given text
	 * @param parent parent object
	 * @param userIds ids to encrypt to or empty list to use symmetric encryption with passphrase
	 * @param text text to encrypt
	 * @param options encryption options
	 */
	explicit KGpgEncrypt(QObject *parent, const QStringList &userIds = QStringList(), const QString &text = QString(), const EncryptOptions &options = DefaultEncryption, const QStringList &extraOptions = QStringList());

	/**
	 * @brief encrypt file(s)
	 * @param parent parent object
	 * @param userIds ids to encrypt to or empty list to use symmetric encryption with passphrase
	 * @param files list of file locations to encrypt
	 * @param options encryption options
	 */
	KGpgEncrypt(QObject *parent, const QStringList &userIds, const KUrl::List &files, const EncryptOptions &options = DefaultEncryption, const QStringList &extraOptions = QStringList());

	/**
	 * @brief destructor
	 */
	virtual ~KGpgEncrypt();

	/**
	 * @brief get decryption result
	 * @return decrypted text
	 */
	QStringList encryptedText() const;

protected:
	virtual QStringList command() const;
	virtual bool nextLine(const QString &line);

private:
	int m_fileIndex;
	const EncryptOptions m_options;
	const QStringList m_userIds;
	QStringList m_extraOptions;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KGpgEncrypt::EncryptOptions);

#endif // KGPGENCRYPT_H
