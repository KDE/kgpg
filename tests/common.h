#ifndef COMMON_TEST_H
#define COMMON_TEST_H

#include <QString>

class KGpgTransaction;
class QTemporaryDir;

bool resetGpgConf(QTemporaryDir &basedir);
QString readFile(const QString& filename);
void addGpgKey(QTemporaryDir &dir, const QString &file, const QString &password = QString());
void addPasswordArguments(KGpgTransaction *transaction, const QString &passphrase);
bool hasPhoto(QTemporaryDir &dir, const QString &id);

#endif
