/*
 * Copyright (C) 2009 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgsignkey.h"

#include "kgpgitemnode.h"

KGpgSignKey::KGpgSignKey(QObject *parent, const QString &signer, KGpgKeyNode *key, const bool local, const carefulCheck checking)
	: KGpgEditKeyTransaction(parent, key->getId(), QString(), false, false),
	KGpgSignTransactionHelper(signer, !local, checking)
{
	insertArgument(1, "-u");
	insertArgument(2, signer);
	m_signerPos = 2;
	addArgumentRef(&m_signerPos);

	setKey(key);

	setLocal(local);
}

KGpgSignKey::~KGpgSignKey()
{
}

bool
KGpgSignKey::nextLine(const QString &line)
{
	switch (KGpgSignTransactionHelper::nextLine(line)) {
	case KGpgSignTransactionHelper::handledFalse:
		return false;
	case KGpgSignTransactionHelper::handledTrue:
		return true;
	default:
		// just to keep the compiler happy
		Q_ASSERT(0);
	case KGpgSignTransactionHelper::notHandled:
		return KGpgEditKeyTransaction::nextLine(line);
	}
}

KGpgTransaction::ts_boolanswer
KGpgSignKey::boolQuestion(const QString& line)
{
	if (line.contains(QLatin1String("sign_all.okay")))
		return BA_YES;

	ts_boolanswer ret = KGpgSignTransactionHelper::boolQuestion(line);

	if (ret == BA_UNKNOWN)
		ret = KGpgTransaction::boolQuestion(line);

	return ret;
}

KGpgTransaction *
KGpgSignKey::asTransaction()
{
	return this;
}

void
KGpgSignKey::replaceCmd(const QString &cmd)
{
	replaceCommand(cmd);
}
