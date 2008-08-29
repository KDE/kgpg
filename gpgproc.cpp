/**
 * Copyright (C) 2007 Rolf Eike Beer <kde@opensource.sf-tec.de>
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

#include <ctype.h>

#include <QTextCodec>

#include "kgpgsettings.h"

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
	QStringList args;
	args << "--no-secmem-warning" << "--no-tty";
	if (binary.isEmpty())
		setProgram(KGpgSettings::gpgBinaryPath(), args);
	else
		setProgram(binary, args);
	setOutputChannelMode(OnlyStdoutChannel);

	disconnect(SIGNAL(finished(int, QProcess::ExitStatus)));
	disconnect(SIGNAL(lineReadyStandardOutput()));
}

void GPGProc::start()
{
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finished()));
    connect(this, SIGNAL(lineReadyStandardOutput()), this, SLOT(received()));
    KProcess::start();
}

void GPGProc::received()
{
	emit readReady(this);
}

void GPGProc::finished()
{
    emit processExited(this);
}

int GPGProc::readln(QString &line, const bool &colons)
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

    l = s.split(':');

    for (int i = 0; i < l.count(); ++i)
    {
        int j = 0;
        while ((j = l[i].indexOf("\\x3a", j, Qt::CaseInsensitive)) >= 0)
        {
            l[i].replace(j, 4, ':');
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

		char c1, c2;
		c1 = a[pos + 2];
		c2 = a[pos + 3];

		// ':' must be skipped, it is used as colon delimiter
		// since it is pure ascii it can be replaced in QString.
		if (!colons && (c1 == '3') && ((c2 == 'a') || (c2 == 'A'))) {
			pos += 3;
			continue;
		}

		if (!isxdigit(c1) || !isxdigit(c2))
			continue;

		char n[2] = { 0, 0 };

		if (isdigit(c1))
			n[0] = c1 - '0';
		else
			n[0] = tolower(c1) - 'a' + 10;
		n[0] *= 16;

		if (isdigit(c2))
			n[0] += c2 - '0';
		else
			n[0] += tolower(c2) - 'a' + 10;

		// it is likely to find the same byte sequence more than once
		int npos = pos;
		QByteArray pattern = a.mid(pos, 4);
		do {
			a.replace(npos, 4, n);
		} while ((npos = a.indexOf(pattern, npos)) >= 0);
	}

	return QTextCodec::codecForName("utf8")->toUnicode(a);
}

#include "gpgproc.moc"
