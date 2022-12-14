#ifndef COMMON_TEST_H
#define COMMON_TEST_H

#include <QString>

static const unsigned int encryptionTestTimeout = 10000;
static const unsigned int decryptionTestTimeout = 25000;

class KGpgTransaction;
class QTemporaryDir;

bool resetGpgConf(QTemporaryDir &basedir);
QString readFile(const QString& filename);
void addGpgKey(const QTemporaryDir &dir, const QString &file, const QString &password = QString());
void addPasswordArguments(KGpgTransaction *transaction, const QString &passphrase);
bool hasPhoto(const QTemporaryDir &dir, const QString &id);

#endif
