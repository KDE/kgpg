#include "kgpgaddphoto.h"
#include "../transactions/kgpgaddphoto.h"
#include "../kgpginterface.h"
#include "common.h"

#include <QSignalSpy>
#include <QtTest>

void KGpgAddPhotoTest::init()
{
	QVERIFY(resetGpgConf());
}

void KGpgAddPhotoTest::testAddPhoto()
{
	const QString passphrase = readFile(QLatin1String("keys/kgpgtest_BA7695F3C550DF14.pass"));
	addGpgKey(QLatin1String("keys/kgpgtest_BA7695F3C550DF14_pub.asc"));
	addGpgKey(QLatin1String("keys/kgpgtest_BA7695F3C550DF14.asc"), passphrase);
	QString keyID = QLatin1String("BA7695F3C550DF14");
	QString imagepath = QLatin1String("keys/image_small.jpg");
	KGpgAddPhoto *transaction = new KGpgAddPhoto(this, keyID, imagepath);
	addPasswordArguments(transaction, passphrase);
	QSignalSpy spy(transaction, &KGpgAddPhoto::done);
	transaction->start();
	QVERIFY(spy.wait());
	QVERIFY(hasPhoto(keyID));
}

QTEST_MAIN(KGpgAddPhotoTest)
