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

KGpgDecrypt::KGpgDecrypt(QObject *parent, const QString &text)
	: KGpgTextOrFileTransaction(parent, text)
{
}

KGpgDecrypt::KGpgDecrypt(QObject* parent, const KUrl::List &files)
	: KGpgTextOrFileTransaction(parent, files)
{
}

KGpgDecrypt::~KGpgDecrypt()
{
}

QStringList
KGpgDecrypt::command() const
{
	QStringList ret;

	ret << "--decrypt";

	ret << KGpgSettings::customDecrypt().simplified().split(' ', QString::SkipEmptyParts);

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
