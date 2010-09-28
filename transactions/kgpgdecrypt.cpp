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

#include "kgpgdecrypt.h"

#include "kgpgsettings.h"

#include <KLocale>

KGpgDecrypt::KGpgDecrypt(QObject *parent, const QString &text)
	: KGpgTextOrFileTransaction(parent, text),
	m_fileIndex(-1)
{
}

KGpgDecrypt::KGpgDecrypt(QObject *parent, const KUrl::List &files)
	: KGpgTextOrFileTransaction(parent, files),
	m_fileIndex(0)
{
}

KGpgDecrypt::~KGpgDecrypt()
{
}

QStringList
KGpgDecrypt::command() const
{
	QStringList ret;

	ret << QLatin1String( "--decrypt" );

	ret << KGpgSettings::customDecrypt().simplified().split(QLatin1Char( ' ' ), QString::SkipEmptyParts);

	return ret;
}

QStringList
KGpgDecrypt::decryptedText() const
{
	QStringList result;

	foreach (const QString &line, getMessages())
		if (!line.startsWith(QLatin1String("[GNUPG:] ")))
			result.append(line);

	return result;
}

bool
KGpgDecrypt::isEncryptedText(const QString &text, int *startPos, int *endPos)
{
	int posStart = text.indexOf(QLatin1String("-----BEGIN PGP MESSAGE-----"));
	if (posStart == -1)
		return false;

	int posEnd = text.indexOf(QLatin1String("-----END PGP MESSAGE-----"), posStart);
	if (posEnd == -1)
		return false;

	if (startPos != NULL)
		*startPos = posStart;
	if (endPos != NULL)
		*endPos = posEnd;

	return true;
}

bool
KGpgDecrypt::nextLine(const QString& line)
{
	const KUrl::List &inputFiles = getInputFiles();

	if (!inputFiles.isEmpty()) {
		if (line == QLatin1String("[GNUPG:] BEGIN_DECRYPTION")) {
			emit statusMessage(i18nc("Status message 'Decrypting <filename>' (operation starts)", "Decrypting %1", inputFiles.at(m_fileIndex).fileName()));
			emit infoProgress(2 * m_fileIndex + 1, inputFiles.count() * 2);
		} else if (line == QLatin1String("[GNUPG:] END_DECRYPTION")) {
			emit statusMessage(i18nc("Status message 'Decrypted <filename>' (operation was completed)", "Decrypted %1", inputFiles.at(m_fileIndex).fileName()));
			m_fileIndex++;
			emit infoProgress(2 * m_fileIndex, inputFiles.count() * 2);
		}
	}

	return KGpgTextOrFileTransaction::nextLine(line);
}
