#ifndef KGPGADDUID_TEST_H
#define KGPGADDUID_TEST_H

#include <QObject>

class KGpgAddUidTest : public QObject {
	Q_OBJECT
private slots:
	void init();
	void testAddUid();
	void testAddUidInvalid();
};

#endif
