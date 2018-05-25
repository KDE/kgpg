#ifndef KGPGCHANGETRUST_TEST_H
#define KGPGCHANGETRUST_TEST_H

#include <gpgme.h>
#include <QMetaType>
#include <QObject>

class KGpgChangeTrustTest : public QObject {
	Q_OBJECT
private slots:
	void init();
	void testChangeTrust();
	void testChangeTrust_data();
};

Q_DECLARE_METATYPE(gpgme_validity_t)

#endif
