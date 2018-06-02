#ifndef KGPGEXPORT_TEST_H
#define KGPGEXPORT_TEST_H

#include <QObject>

class KGpgExportTest : public QObject {
	Q_OBJECT
private slots:
	void init();
	void testExportPublicKeyToFile();
	void testExportSecretKeyToFile();
	void testExportPublicKeyToStdOutput();
};

#endif
