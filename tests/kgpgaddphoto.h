#ifndef KGPGADDPHOTO_TEST_H
#define KGPGADDPHOTO_TEST_H

#include <QObject>

class KGpgAddPhotoTest : public QObject {
	Q_OBJECT
private slots:
	void init();
	void testAddPhoto();
};

#endif
