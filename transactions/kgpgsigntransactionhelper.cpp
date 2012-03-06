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

#include "kgpgsigntransactionhelper.h"

#include "kgpgtransaction.h"
#include "model/kgpgitemnode.h"

KGpgSignTransactionHelper::KGpgSignTransactionHelper(const QString &signer, const bool local, const carefulCheck checking)
	: m_signer(signer),
	m_local(local),
	m_checking(checking),
	m_signerPos(-1)
{
}

KGpgSignTransactionHelper::~KGpgSignTransactionHelper()
{
}

void
KGpgSignTransactionHelper::setKey(const KGpgKeyNode *node)
{
	m_node = node;
}

const KGpgKeyNode *
KGpgSignTransactionHelper::getKey(void) const
{
	return m_node;
}

KGpgSignTransactionHelper::lineParseResults
KGpgSignTransactionHelper::nextLine(const QString &line)
{
	if (!line.startsWith(QLatin1String("[GNUPG:]"))) {
		return notHandled;
	}

	if (line.contains(QLatin1String( "ALREADY_SIGNED" ))) {
		asTransaction()->setSuccess(TS_ALREADY_SIGNED);
		return handledFalse;
	} else  if (line.contains(QLatin1String( "GOOD_PASSPHRASE" ))) {
		asTransaction()->setSuccess(KGpgTransaction::TS_MSG_SEQUENCE);
		return handledFalse;
	} else if (line.contains(QLatin1String( "sign_uid.expire" ))) {
		asTransaction()->write("Never");
		return handledFalse;
	} else if (line.contains(QLatin1String( "sign_uid.class" ))) {
		asTransaction()->write(m_checking);
		return handledFalse;
	} else if (line.startsWith(QLatin1String("[GNUPG:] KEYEXPIRED ")) ||
				line.startsWith(QLatin1String("[GNUPG:] SIGEXPIRED"))) {
		// I have no idea why GnuPG does this when I want to sign, but it sometimes does
		return handledFalse;
	} else {
		return notHandled;
	}
}

KGpgTransaction::ts_boolanswer
KGpgSignTransactionHelper::boolQuestion(const QString& line)
{
	if (line == QLatin1String("sign_uid.okay")) {
		return KGpgTransaction::BA_YES;
	} else if (line == QLatin1String("keyedit.save.okay")) {
		KGpgTransaction *ta = asTransaction();

		switch (ta->getSuccess()) {
		case TS_ALREADY_SIGNED:
		case KGpgTransaction::TS_USER_ABORTED:
			break;
		default:
			asTransaction()->setSuccess(KGpgTransaction::TS_OK);
		}

		return KGpgTransaction::BA_YES;
	} else {
		return KGpgTransaction::BA_UNKNOWN;
	}
}

void
KGpgSignTransactionHelper::setLocal(const bool local)
{
	if (local == m_local)
		return;

	m_local = local;
	if (local)
		replaceCmd(QLatin1String( "lsign" ));
	else
		replaceCmd(QLatin1String( "sign" ));
}

bool
KGpgSignTransactionHelper::getLocal(void) const
{
	return m_local;
}

void
KGpgSignTransactionHelper::setChecking(const carefulCheck level)
{
	m_checking = level;
}

KGpgSignTransactionHelper::carefulCheck
KGpgSignTransactionHelper::getChecking(void) const
{
	return m_checking;
}

void
KGpgSignTransactionHelper::setSigner(const QString &signer)
{
	m_signer = signer;
	asTransaction()->replaceArgument(m_signerPos, signer);
}


QString
KGpgSignTransactionHelper::getSigner(void) const
{
	return m_signer;
}

void
KGpgSignTransactionHelper::setSecringFile(const QString &filename)
{
	QStringList secringargs(QLatin1String( "--secret-keyring" ));
	secringargs << filename;

	asTransaction()->insertArguments(1, secringargs);
}

#include "kgpgsigntransactionhelper.moc"
