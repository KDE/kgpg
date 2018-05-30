#ifndef KGPGDELKEY_TEST_H
#define KGPGDELKEY_TEST_H

#include <QObject>

class KGpgDelKeyTest : public QObject {
	Q_OBJECT
private slots:
	void init();
	void testDeleteKey();
	void testDeleteKey_data();
};

#endif
