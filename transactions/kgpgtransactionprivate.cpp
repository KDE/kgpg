/*
 * Copyright (C) 2009,2010,2012,2014 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgtransaction.h"
#include "kgpgtransactionprivate.h"

KGpgTransactionPrivate::KGpgTransactionPrivate(KGpgTransaction *parent, bool allowChaining)
	: m_parent(parent),
	m_process(new GPGProc()),
	m_inputTransaction(Q_NULLPTR),
	m_newPasswordDialog(Q_NULLPTR),
	m_passwordDialog(Q_NULLPTR),
	m_success(KGpgTransaction::TS_OK),
	m_tries(3),
	m_chainingAllowed(allowChaining),
	m_inputProcessDone(false),
	m_inputProcessResult(KGpgTransaction::TS_OK),
	m_ownProcessFinished(false),
	m_quitTries(0)
{
}

KGpgTransactionPrivate::~KGpgTransactionPrivate()
{
	if (m_newPasswordDialog) {
		m_newPasswordDialog->close();
		m_newPasswordDialog->deleteLater();
	}
	if (m_process->state() == QProcess::Running) {
		m_process->closeWriteChannel();
		m_process->terminate();
	}
	delete m_inputTransaction;
	delete m_process;
}
