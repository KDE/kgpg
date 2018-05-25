#ifndef KGPGDECRYPT_TEST_H
#define KGPGDECRYPT_TEST_H

#include <QObject>

class KGpgDecryptTest : public QObject {
	Q_OBJECT
private slots:
	void init();
	void testDecrypt();
	void testDecrypt_data();
};

#endif
