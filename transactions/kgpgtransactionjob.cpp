#include "kgpgtransactionjob.h"

#include "kgpgtransaction.h"

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
	m_transaction->start();
	emit description(this, m_transaction->getDescription());
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

#include "kgpgtransactionjob.moc"
