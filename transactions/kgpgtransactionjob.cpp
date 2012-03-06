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

#include "kgpgtransactionjob.h"

#include "kgpgtransaction.h"

#include <KLocale>

KGpgTransactionJob::KGpgTransactionJob(KGpgTransaction *transaction)
	: KJob(transaction->parent()),
	m_transaction(transaction),
	m_result(-1)
{
}

KGpgTransactionJob::~KGpgTransactionJob()
{
	delete m_transaction;
}

void
KGpgTransactionJob::start()
{
	connect(m_transaction, SIGNAL(done(int)), SLOT(slotTransactionDone(int)));
	connect(m_transaction, SIGNAL(statusMessage(QString)), SLOT(slotStatusMessage(QString)));
	connect(m_transaction, SIGNAL(infoProgress(qulonglong,qulonglong)), SLOT(slotInfoProgress(qulonglong,qulonglong)));

	slotStatusMessage(i18nc("Job is started up", "Startup"));
	m_transaction->start();
}

const KGpgTransaction *
KGpgTransactionJob::getTransaction() const
{
	return m_transaction;
}

int
KGpgTransactionJob::getResultCode() const
{
	return m_result;
}

void
KGpgTransactionJob::slotTransactionDone(int result)
{
	m_result = result;
	emitResult();
}

void
KGpgTransactionJob::slotStatusMessage(const QString &msg)
{
	emit description(this, m_transaction->getDescription(), qMakePair(i18nc("State of operation as in status", "State"), msg));
}

void
KGpgTransactionJob::slotInfoProgress(qulonglong processedAmount, qulonglong totalAmount)
{
	emitPercent(processedAmount, totalAmount);
}

bool
KGpgTransactionJob::doKill()
{
	m_transaction->kill();

	return true;
}


#include "kgpgtransactionjob.moc"
