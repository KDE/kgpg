#ifndef COMMON_TEST_H
#define COMMON_TEST_H

#include <QString>

class KGpgTransaction;

bool resetGpgConf();
QString readFile(const QString& filename);
void addGpgKey(const QString& file, const QString& password = QString());
void addPasswordArguments(KGpgTransaction *transaction, const QString &passphrase);
bool hasPhoto(QString id);
#endif
