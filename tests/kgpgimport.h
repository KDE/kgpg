#ifndef KGPGIMPORT_TEST_H
#define KGPGIMPORT_TEST_H

#include <QObject>

class KGpgImportTest: public QObject
{
	Q_OBJECT
private slots:
	void init();
	void testImportTextKey();
	void testImportIdsAll();
	void testImportIdsUnchanged();
	void testImportKeyFromFile();
	void testImportSameKeyTwice();
	void testLogMessage();
	void testImportSecretKey();
};

#endif
