#ifndef COMMON_TEST_H
#define COMMON_TEST_H
#include "../transactions/kgpgtransaction.h"

#include <gpgme.h>
#include <QString>

bool resetGpgConf();
QString readFile(const QString& filename);
void addGpgKey(const QString& file, const QString& password = QString());
void addPasswordArguments(KGpgTransaction *transaction, QString passphrase);
#endif
