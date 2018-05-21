#include <QtTest/QtTest>

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
