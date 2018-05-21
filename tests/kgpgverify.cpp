#include "kgpgverify.h"
#include "../transactions/kgpgverify.h"
#include "common.h"

#include <QList>
#include <QLatin1String>
#include <QSignalSpy>

void KGpgVerifyTest::init()
{
	QVERIFY(resetGpgConf());
}

void KGpgVerifyTest::testVerifySignedText()
{
	addGpgKey(QLatin1String("keys/kgpgtest_BA7695F3C550DF14.asc"));
	QString text = readFile(QLatin1String("keys/signed_text"));
	KGpgVerify *transaction = new KGpgVerify(this, text);
	QSignalSpy spy(transaction, &KGpgVerify::done);
	transaction->start();
	QObject::connect(transaction, &KGpgVerify::done,
			 [](int result) { QCOMPARE(result, KGpgTransaction::TS_OK); });
	QVERIFY(spy.wait());
}

void KGpgVerifyTest::testVerifySignedFile()
{
	addGpgKey(QLatin1String("keys/kgpgtest_BA7695F3C550DF14.asc"));
	QList<QUrl> list;
	list.append(QUrl::fromLocalFile(QLatin1String("keys/signed_text")));
	KGpgVerify *transaction = new KGpgVerify(this, list);
	QSignalSpy spy(transaction, &KGpgVerify::done);
	transaction->start();
	QObject::connect(transaction, &KGpgVerify::done,
			 [](int result) { QCOMPARE(result, KGpgTransaction::TS_OK); });
	QVERIFY(spy.wait());
}

void KGpgVerifyTest::testVerifyReturnMissingKey()
{
	QList<QUrl> list;
	list.append(QUrl::fromLocalFile(QLatin1String("keys/signed_text")));
	KGpgVerify *transaction = new KGpgVerify(this, list);
	QSignalSpy spy(transaction, &KGpgVerify::done);
	transaction->start();
	QObject::connect(transaction, &KGpgVerify::done,
			 [](int result) { QCOMPARE(result, KGpgVerify::TS_MISSING_KEY); });
	QVERIFY(spy.wait());
}

void KGpgVerifyTest::testVerifyMissingId()
{
	QList<QUrl> list;
	list.append(QUrl::fromLocalFile(QLatin1String("keys/signed_text")));
	KGpgVerify *transaction = new KGpgVerify(this, list);
	QSignalSpy spy(transaction, &KGpgVerify::done);
	transaction->start();
	QObject::connect(transaction, &KGpgVerify::done, [transaction]() {
		QString keyID = QLatin1String("7882C615210F1022");
		QVERIFY(transaction->missingId().compare(keyID) == 0);
	});
	QVERIFY(spy.wait());
}

void KGpgVerifyTest::testVerifyReturnBadSignature()
{
	QList<QUrl> list;
	addGpgKey(QLatin1String("keys/kgpgtest_BA7695F3C550DF14.asc"));
	list.append(QUrl::fromLocalFile(QLatin1String("keys/signed_bad_sig")));
	KGpgVerify *transaction = new KGpgVerify(this, list);
	QSignalSpy spy(transaction, &KGpgVerify::done);
	transaction->start();
	QObject::connect(transaction, &KGpgVerify::done,
			 [](int result) { QCOMPARE(result, KGpgVerify::TS_BAD_SIGNATURE); });
	QVERIFY(spy.wait());
}

QTEST_MAIN(KGpgVerifyTest)
