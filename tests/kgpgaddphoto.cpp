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
	addGpgKey(QLatin1String("keys/kgpgtest_BA7695F3C550DF14.asc"),
		  readFile(QLatin1String("keys/kgpgtest_BA7695F3C550DF14.pass")));
	QString keyID = QLatin1String("BA7695F3C550DF14");
	QString imagepath = QLatin1String("keys/image.jpg");
	KGpgAddPhoto *transaction = new KGpgAddPhoto(this, keyID, imagepath);
	QSignalSpy spy(transaction, &KGpgAddPhoto::done);
	transaction->start();
	QVERIFY(spy.wait());
	QVERIFY(hasPhoto(keyID));
}

QTEST_MAIN(KGpgAddPhotoTest)
