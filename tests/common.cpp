#include "common.h"
#include "../kgpginterface.h"
#include "../kgpgsettings.h"
#include "../transactions/kgpgtransaction.h"

#include <gpgme.h>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QProcess>
#include <QString>

bool resetGpgConf()
{
	const QString dot_gpg(QLatin1String(".gnupg"));
	QDir dir(dot_gpg);
	dir.removeRecursively();
	if (!QDir::current().mkdir(dot_gpg))
		return false;
	if (!QFile::setPermissions(QDir::currentPath() + QLatin1Char('/') + dot_gpg,
				   QFileDevice::ReadOwner | QFileDevice::WriteOwner |
					   QFileDevice::ExeOwner))
		return false;
	return QFile::copy(QLatin1String("gnupg/gpg.conf"), QLatin1String(".gnupg/gpg.conf"));
}

QString readFile(const QString &filename)
{
	QFile file(filename);
	if (file.open(QIODevice::ReadOnly))
		return QLatin1String(file.readAll());
	else
		return QString();
}

void addGpgKey(const QString &file, const QString &password)
{
	QString conf = QLatin1String(".gnupg/gpg.conf");
	QString gpgHome = QLatin1String(".gnupg");
	QString command = QLatin1String("gpg");
	QStringList args;
	args.push_back(QLatin1String("--no-secmem-warning"));
	args.push_back(QLatin1String("--no-tty"));
	args.push_back(QLatin1String("--batch"));
	if (!password.isEmpty()) {
		args.push_back(QLatin1String("--passphrase"));
		args.push_back(password);
	}
	args.push_back(QLatin1String("--options"));
	args.push_back(conf);
	args.push_back(QLatin1String("--homedir"));
	args.push_back(gpgHome);
	args.push_back(QLatin1String("--debug-level"));
	args.push_back(QLatin1String("none"));
	args.push_back(QLatin1String("--status-fd=1"));
	args.push_back(QLatin1String("--import"));
	args.push_back(QLatin1String("--allow-secret-key-import"));
	args.push_back(QLatin1String("--command-fd=0"));
	args.push_back(file);
	QProcess process;
	process.execute(command, args);
	qDebug() << "Added Gpg key: " << file;
}

void addPasswordArguments(KGpgTransaction *transaction, const QString &passphrase)
{
	QStringList args;
	args.push_back(QLatin1String("--batch"));
	args.push_back(QLatin1String("--passphrase"));
	args.push_back(passphrase);
	args.push_back(QLatin1String("--pinentry-mode"));
	args.push_back(QLatin1String("loopback"));
	transaction->insertArguments(1, args);
}

bool hasPhoto(QString id)
{
	QStringList args{ QLatin1String("--list-key"), id };
	QString command = QLatin1String("gpg");
	QProcess process;
	process.start(command, args);
	process.waitForFinished();
	QString output = QLatin1String(process.readAllStandardOutput());
	qDebug()<< output;
	return output.contains(QLatin1String("image"));
}
