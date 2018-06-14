#include "kgpgadduid.h"
#include "../transactions/kgpgadduid.h"
#include "../kgpginterface.h"
#include "../kgpgsettings.h"
#include "common.h"

#include <QLatin1String>
#include <QSignalSpy>
#include <QString>
#include <QtTest>

void KGpgAddUidTest::init()
{
	QVERIFY(resetGpgConf());
}

void KGpgAddUidTest::testAddUid()
{
	QString passphrase = readFile(QLatin1String("keys/kgpgtest_BA7695F3C550DF14.pass"));
	addGpgKey(QLatin1String("keys/kgpgtest_BA7695F3C550DF14_pub.asc"));
	addGpgKey(QLatin1String("keys/kgpgtest_BA7695F3C550DF14.asc"), passphrase);
	QString keyId = QLatin1String("BA7695F3C550DF14");
	QString name = QLatin1String("Test name");
	QString email = QLatin1String("test@kde.org");
	QString comment = QLatin1String("Test comment");
	KGpgAddUid *transaction = new KGpgAddUid(this, keyId, name, email, comment);
	addPasswordArguments(transaction, passphrase);
	QObject::connect(transaction, &KGpgAddUid::done,
			 [](int result) { QCOMPARE(result, static_cast<int>(KGpgTransaction::TS_OK)); });
	QSignalSpy spy(transaction, &KGpgAddUid::done);
	transaction->start();
	QVERIFY(spy.wait());
	KgpgCore::KgpgKeyList keyList = KgpgInterface::readPublicKeys();
	QCOMPARE(keyList.size(), 1);
	KgpgCore::KgpgKey key = keyList.first();
	QCOMPARE(key.name(), name);
	QCOMPARE(key.email(), email);
	QCOMPARE(key.comment(), comment);
	QCOMPARE(key.fullId(), keyId);
}

QTEST_MAIN(KGpgAddUidTest)
