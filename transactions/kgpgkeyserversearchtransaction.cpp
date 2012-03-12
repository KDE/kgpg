/*
 * Copyright (C) 2010,2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "kgpgkeyserversearchtransaction.h"

KGpgKeyserverSearchTransaction::KGpgKeyserverSearchTransaction(QObject *parent, const QString &keyserver, const QString &pattern, const bool withProgress, const QString &proxy)
	: KGpgKeyserverTransaction(parent, keyserver, withProgress, proxy),
	m_count(0)
{
	addArgument(QLatin1String( "--with-colons" ));
	addArgument(QLatin1String( "--search-keys" ));
	m_patternPos = addArgument(pattern);
}

KGpgKeyserverSearchTransaction::~KGpgKeyserverSearchTransaction()
{
}

bool
KGpgKeyserverSearchTransaction::preStart()
{
	setSuccess(TS_MSG_SEQUENCE);
	m_count = 0;
	m_keyLines.clear();

	return KGpgKeyserverTransaction::preStart();
}

bool
KGpgKeyserverSearchTransaction::nextLine(const QString &line)
{
	if (line.startsWith(QLatin1String("[GNUPG:] GET_LINE keysearch.prompt"))) {
		if (m_count < 4) {
			write("n");
		} else {
			return true;
		}
	} else if (line.startsWith(QLatin1String("[GNUPG:] GOT_IT"))) {
		m_count++;
	} else if (!line.isEmpty() && !line.startsWith(QLatin1String("[GNUPG:] "))) {
		if (line.startsWith(QLatin1String("pub:"))) {
			if (!m_keyLines.isEmpty()) {
				emit newKey(m_keyLines);
				m_keyLines.clear();
			}
			m_keyLines.append(line);
		} else if (!m_keyLines.isEmpty())
			m_keyLines.append(line);
	}

	return false;
}

void
KGpgKeyserverSearchTransaction::finish()
{
	if (!m_keyLines.isEmpty()) {
		emit newKey(m_keyLines);
		m_keyLines.clear();
	}
}

void
KGpgKeyserverSearchTransaction::setPattern(const QString &pattern)
{
	replaceArgument(m_patternPos, pattern);
}

#include "kgpgkeyserversearchtransaction.moc"
