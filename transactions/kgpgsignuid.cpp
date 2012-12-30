/*
 * Copyright (C) 2009,2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgsignuid.h"

#include "model/kgpgitemnode.h"

KGpgSignUid::KGpgSignUid(QObject *parent, const QString &signer, const KGpgSignableNode *uid, const bool local, const carefulCheck checking)
	: KGpgUidTransaction(parent),
	KGpgSignTransactionHelper(signer, !local, checking)
{
	insertArgument(1, QLatin1String( "-u" ));
	insertArgument(2, signer);
	m_signerPos = 2;
	addArgumentRef(&m_signerPos);

	m_cmdPos = addArgument(QString());
	addArgumentRef(&m_cmdPos);

	addArgument(QLatin1String("save"));

	setUid(uid);

	setLocal(local);
}

KGpgSignUid::~KGpgSignUid()
{
}

void
KGpgSignUid::setUid(const KGpgSignableNode *uid)
{
	switch (uid->getType()) {
	case ITYPE_PUBLIC:
	case ITYPE_PAIR:
		KGpgUidTransaction::setUid(1);
		setKey(uid->toKeyNode());
		break;
	case ITYPE_UAT:
	case ITYPE_UID:
		KGpgUidTransaction::setUid(uid->getId());
		setKey(uid->getParentKeyNode()->toKeyNode());
		break;
	default:
		Q_ASSERT(0);
	}

	setKeyId(getKey()->getId());
}

bool
KGpgSignUid::nextLine(const QString &line)
{
	switch (KGpgSignTransactionHelper::nextLine(line)) {
	case KGpgSignTransactionHelper::handledFalse:
		return false;
	case KGpgSignTransactionHelper::handledTrue:
		return true;
	default:
		Q_ASSERT(0);
	case KGpgSignTransactionHelper::notHandled:
		return standardCommands(line);
	}
}

KGpgTransaction::ts_boolanswer
KGpgSignUid::boolQuestion(const QString& line)
{
	ts_boolanswer ret = KGpgSignTransactionHelper::boolQuestion(line);

	if (ret == BA_UNKNOWN)
		ret = KGpgTransaction::boolQuestion(line);

	return ret;
}

bool
KGpgSignUid::passphraseReceived()
{
	setSuccess(KGpgTransaction::TS_OK);
	return true;
}

KGpgTransaction *
KGpgSignUid::asTransaction()
{
	return this;
}

void
KGpgSignUid::replaceCmd(const QString &cmd)
{
	replaceArgument(m_cmdPos, cmd);
}

#include "kgpgsignuid.moc"
