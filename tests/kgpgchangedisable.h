#ifndef KGPGCHANGEDISABLE_TEST_H
#define KGPGCHANGEDISABLE_TEST_H

#include <QObject>

class KGpgChangeDisableTest : public QObject {
	Q_OBJECT
private slots:
	void init();
	void testDisableKey();
	void testEnableKey();
};

#endif
