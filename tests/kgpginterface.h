#ifndef KGPG_INTERFACE_TEST_H
#define KGPG_INTERFACE_TEST_H
#include <QtTest/QtTest>

class KGpgInterfaceTest: public QObject
{
	Q_OBJECT
private slots:
	void init();
	void testReadPublicKeys();
	void testReadSecretKeys();
	void testReadEmptyKeyring();
};


#endif
