#ifndef KGPGVERIFYTEST_H
#define KGPGVERIFYTEST_H

#include <QObject>

class KGpgVerifyTest: public QObject
{
	Q_OBJECT
private slots:
	void init();
	void testVerifySignedText();
	void testVerifySignedFile();
	void testVerifyReturnMissingKey();
	void testVerifyMissingId();
	void testVerifyReturnBadSignature();
};

#endif // KGPGVERIFYTEST_H
