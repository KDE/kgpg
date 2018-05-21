#include "kgpgchangetrust.h"
#include "../transactions/kgpgchangetrust.h"
#include "../kgpginterface.h"
#include "common.h"

#include <QLatin1String>
#include <QSignalSpy>

void KGpgChangeTrustTest::init()
{
	QVERIFY(resetGpgConf());
}

void KGpgChangeTrustTest::testChangeTrust()
{
	QFETCH(gpgme_validity_t, target_trust);
	QLatin1String keyID("BA7695F3C550DF14");
	addGpgKey(QLatin1String("keys/kgpgtest_BA7695F3C550DF14.asc"));
	KGpgChangeTrust *transaction = new KGpgChangeTrust(this, keyID, target_trust);
	QSignalSpy spy(transaction, &KGpgChangeTrust::done);
	QObject::connect(transaction, &KGpgChangeTrust::done,
			 [](int result) { QCOMPARE(result, KGpgTransaction::TS_OK); });
	transaction->start();
	QVERIFY(spy.wait());
	KgpgCore::KgpgKeyList keyList = KgpgInterface::readPublicKeys(QStringList(keyID));
	QCOMPARE(keyList.first().ownerTrust(), target_trust);
}

void KGpgChangeTrustTest::testChangeTrust_data()
{
	QTest::addColumn<gpgme_validity_t>("target_trust");
	QTest::newRow("ultimate") << GPGME_VALIDITY_ULTIMATE;
	QTest::newRow("full") << GPGME_VALIDITY_FULL;
	QTest::newRow("marginal") << GPGME_VALIDITY_MARGINAL;
	QTest::newRow("never") << GPGME_VALIDITY_NEVER;
}

// #include "kgpgchangetrust.moc"
QTEST_MAIN(KGpgChangeTrustTest)
