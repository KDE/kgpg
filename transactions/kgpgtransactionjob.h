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

#ifndef KGPGTRANSACTIONJOB_H
#define KGPGTRANSACTIONJOB_H

#include <KJob>

class KGpgTransaction;

/**
* @brief Wrap a GnuPG transaction in a job
*
* This class allows to run any KGpgTransaction as KJob.
*
* @author Rolf Eike Beer
*/
class KGpgTransactionJob : public KJob {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgTransactionJob)
	KGpgTransactionJob() Q_DECL_EQ_DELETE;

public:
	/**
	 * @brief create a new KJob for this transaction
	 * @param transaction operation to do
	 *
	 * The job will take ownership of the transaction, i.e.
	 * will delete the transaction object when the job is done.
	 */
	explicit KGpgTransactionJob(KGpgTransaction *transaction);
	/**
	 * @brief KGpgTransactionJob destructor
	 */
	virtual ~KGpgTransactionJob();

	/**
	 * @brief starts the transaction
	 */
	virtual void start() Q_DECL_OVERRIDE;

	/**
	 * @brief get the transaction this job is handling
	 */
	const KGpgTransaction *getTransaction() const;

	/**
	 * @brief get the result of the transaction
	 */
	int getResultCode() const;

protected:
	virtual bool doKill() Q_DECL_OVERRIDE;

private slots:
	void slotTransactionDone(int result);
	void slotStatusMessage(const QString &plain);
	void slotInfoProgress(qulonglong processedAmount, qulonglong totalAmount);

private:
	KGpgTransaction * const m_transaction;
	int m_result;
	bool m_wasKilled;
};

#endif
