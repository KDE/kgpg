/*
 * Copyright (C) 2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGSIGNTEXT_H
#define KGPGSIGNTEXT_H

#include <QObject>
#include <QString>
#include <QStringList>

#include <KUrl>

#include "kgpgtextorfiletransaction.h"

class QProcess;

/**
 * @brief sign the given text or files
 */
class KGpgSignText: public KGpgTextOrFileTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgSignText)
	KGpgSignText(); // = delete C++0x
public:
	enum SignOption {
		DefaultSignature = 0,		///< use whatever GnuPGs defaults are
		AsciiArmored = 0x1,		///< output the data as printable ASCII as opposed to binary data
		DetachedSignature = 0x2,	///< save the signature in a separate file
	};
	Q_DECLARE_FLAGS(SignOptions, SignOption);

	/**
	 * @brief encrypt given text
	 * @param parent parent object
	 * @param signId the key to use for signing
	 * @param text text to encrypt
	 * @param options encryption options
	 */
	KGpgSignText(QObject *parent, const QString &signId, const QString &text = QString(), const SignOptions &options = AsciiArmored, const QStringList &extraOptions = QStringList());

	/**
	 * @brief encrypt file(s)
	 * @param parent parent object
	 * @param signId the key to use for signing
	 * @param files list of file locations to encrypt (must only be 1 file)
	 * @param options encryption options
	 *
	 * @warning GnuPG can currently handle only one file per invocation for
	 * signing, so files may only contain one single file.
	 */
	KGpgSignText(QObject *parent, const QString &signId, const KUrl::List &files, const SignOptions &options = DefaultSignature, const QStringList &extraOptions = QStringList());

	/**
	 * @brief destructor
	 */
	virtual ~KGpgSignText();

	/**
	 * @brief get signing result
	 * @return signed text
	 */
	QStringList signedText() const;

protected:
	virtual QStringList command() const;

private:
	int m_fileIndex;
	const SignOptions m_options;
	const QString m_signId;
	QStringList m_extraOptions;
	QString m_currentFile;
};

#endif // KGPGSIGNTEXT_H
