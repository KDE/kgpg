/*
 * Copyright (C) 2011,2012,2013 Rolf Eike Beer <kde@opensource.sf-tec.de>
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

#include <QUrl>

#include "kgpgtextorfiletransaction.h"

/**
 * @brief encrypt the given text or files
 */
class KGpgEncrypt: public KGpgTextOrFileTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgEncrypt)
	KGpgEncrypt() Q_DECL_EQ_DELETE;
public:
	enum EncryptOption {
		DefaultEncryption = 0,		///< use whatever GnuPGs defaults are
		AsciiArmored = 0x1,		///< output the data as printable ASCII as opposed to binary data
		AllowUntrustedEncryption = 0x2,	///< allow encryption with untrusted keys, ignored for symmetric encryption
		HideKeyId = 0x4			///< remove anything that shows which key ids this data is encrypted to, ignored for symmetric encryption
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
	KGpgEncrypt(QObject *parent, const QStringList &userIds, const QList<QUrl> &files, const EncryptOptions &options = DefaultEncryption, const QStringList &extraOptions = QStringList());

	/**
	 * @brief destructor
	 */
	virtual ~KGpgEncrypt();

	/**
	 * @brief get decryption result
	 * @return decrypted text
	 */
	QStringList encryptedText() const;

	/**
	 * @brief return the preferred extension for encrypted files
	 * @param ascii if the file is encrypted with ASCII armor
	 * @return the file extension with leading dot
	 */
	static QString encryptExtension(const bool ascii);

protected:
	QStringList command() const Q_DECL_OVERRIDE;
	bool nextLine(const QString &line) Q_DECL_OVERRIDE;
	ts_boolanswer confirmOverwrite (QUrl &currentFile) Q_DECL_OVERRIDE;

private:
	int m_fileIndex;
	const EncryptOptions m_options;
	const QStringList m_userIds;
	QStringList m_extraOptions;
	QString m_currentFile;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KGpgEncrypt::EncryptOptions);

#endif // KGPGENCRYPT_H
