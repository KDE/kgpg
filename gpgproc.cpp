/*
 * Copyright (C) 2007,2010 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "gpgproc.h"

#include "kgpgsettings.h"
#include "kgpginterface.h"

#include <KProcess>
#include <KDebug>
#include <QTextCodec>

class GnupgBinary {
public:
	GnupgBinary();

	const QString &binary() const;
	void setBinary(const QString &executable);
	const QStringList &standardArguments() const;
	unsigned int version() const;
	bool supportsDebugLevel() const;

private:
	QString m_binary;
	QStringList m_standardArguments;
	unsigned int m_version;
	bool m_useDebugLevel;
};

GnupgBinary::GnupgBinary()
	: m_useDebugLevel(false)
{
}

const QString &GnupgBinary::binary() const
{
	return m_binary;
}

/**
 * @brief check if GnuPG returns an error for this arguments
 * @param executable the GnuPG executable to call
 * @param arguments the arguments to pass to executable
 *
 * The arguments will be used together with "--version", so they should not
 * be any commands.
 */
static bool checkGnupgArguments(const QString &executable, const QStringList &arguments)
{
	KProcess gpg;

	// We ignore the output anyway, just make sure it doesn't clutter the output of
	// the parent process. Simplify the handling by putting all trash in one can.
	gpg.setOutputChannelMode(KProcess::MergedChannels);

	QStringList allArguments = arguments;
	allArguments << QLatin1String("--version");
	gpg.setProgram(executable, allArguments);

	return (gpg.execute() == 0);
}

void GnupgBinary::setBinary(const QString &executable)
{
	kDebug(2100) << "checking version of GnuPG executable" << executable;
	// must be set first as KgpgInterface uses GPGProc to parse the output
	m_binary = executable;
	const QString verstr = KgpgInterface::gpgVersionString(executable);
	m_version = KgpgInterface::gpgVersion(verstr);
	kDebug(2100) << "version is" << verstr << m_version;

	m_useDebugLevel = (m_version > 0x20000);

	const QString gpgConfigFile = KGpgSettings::gpgConfigPath();

	m_standardArguments.clear();
	m_standardArguments << QLatin1String( "--no-secmem-warning" )
			<< QLatin1String( "--no-tty" )
			<< QLatin1String("--no-greeting");

	if (!gpgConfigFile.isEmpty())
		m_standardArguments << QLatin1String("--options")
				<< gpgConfigFile;

	QStringList debugLevelArguments(QLatin1String("--debug-level"));
	debugLevelArguments << QLatin1String("none");
	if (checkGnupgArguments(executable, debugLevelArguments))
		m_standardArguments << debugLevelArguments;
}

const QStringList& GnupgBinary::standardArguments() const
{
	return m_standardArguments;
}

unsigned int GnupgBinary::version() const
{
	return m_version;
}

bool GnupgBinary::supportsDebugLevel() const
{
	return m_useDebugLevel;
}

K_GLOBAL_STATIC(GnupgBinary, lastBinary)

GPGProc::GPGProc(QObject *parent, const QString &binary)
       : KLineBufferedProcess(parent)
{
	resetProcess(binary);
}

GPGProc::~GPGProc()
{
}

void
GPGProc::resetProcess(const QString &binary)
{
	GnupgBinary *bin = lastBinary;
	QString executable;
	if (binary.isEmpty())
		executable = KGpgSettings::gpgBinaryPath();
	else
		executable = binary;

	if (bin->binary() != executable)
		bin->setBinary(executable);

	setProgram(executable, bin->standardArguments());

	setOutputChannelMode(OnlyStdoutChannel);

	disconnect(SIGNAL(finished(int,QProcess::ExitStatus)));
	disconnect(SIGNAL(lineReadyStandardOutput()));
}

void GPGProc::start()
{
	// make sure there is exactly one connection from us to that signal
	connect(this, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(finished()), Qt::UniqueConnection);
	connect(this, SIGNAL(lineReadyStandardOutput()), this, SLOT(received()), Qt::UniqueConnection);
	KProcess::start();
}

void GPGProc::received()
{
	emit readReady();
}

void GPGProc::finished()
{
	emit processExited();
}

int GPGProc::readln(QString &line, const bool colons)
{
	QByteArray a;
	if (!readLineStandardOutput(&a))
		return -1;

	line = recode(a, colons);

	return line.length();
}

int GPGProc::readln(QStringList &l)
{
    QString s;

    int len = readln(s);
    if (len < 0)
        return len;

    l = s.split(QLatin1Char( ':' ));

    for (int i = 0; i < l.count(); ++i)
    {
        int j = 0;
        while ((j = l[i].indexOf(QLatin1String( "\\x3a" ), j, Qt::CaseInsensitive)) >= 0)
        {
            l[i].replace(j, 4, QLatin1Char( ':' ));
            j++;
        }
    }

    return l.count();
}

QString
GPGProc::recode(QByteArray a, const bool colons)
{
	int pos = 0;

	while ((pos = a.indexOf("\\x", pos)) >= 0) {
		if (pos > a.length() - 4)
			break;

		const QByteArray pattern(a.mid(pos, 4));
		const QByteArray hexnum(pattern.right(2));
		bool ok;
		char n[2];
		n[0] = hexnum.toUShort(&ok, 16);
		n[1] = '\0';	// to use n as a 0-terminated string
		if (!ok)
			continue;

		// QLatin1Char( ':' ) must be skipped, it is used as column delimiter
		// since it is pure ascii it can be replaced in QString.
		if (!colons && (n[0] == ':' )) {
			pos += 3;
			continue;
		}

		// it is likely to find the same byte sequence more than once
		int npos = pos;
		do {
			a.replace(npos, 4, n);
		} while ((npos = a.indexOf(pattern, npos)) >= 0);
	}

	return QTextCodec::codecForName("utf8")->toUnicode(a);
}

#include "gpgproc.moc"
