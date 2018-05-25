#ifndef KGPGENCRYPT_TEST_H
#define KGPGENCRYPT_TEST_H

#include <QObject>

class KGpgEncryptTest : public QObject {
	Q_OBJECT
private slots:
	void init();
	void testAsciiArmoredEncryption();
	void testHideKeyIdEncryption();
	void testSymmetricEncryption();
};

#endif
