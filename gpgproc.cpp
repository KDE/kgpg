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

#include <kdebug.h>
#include <QtCore/QTextCodec>
#include <QFile>

#include "kgpgsettings.h"

class GPGProcPrivate
{
public:
	GPGProcPrivate()
		: codec( QTextCodec::codecForName("utf8") )
	{
	}

	QTextCodec *codec;
	QByteArray recvbuffer;
};

GPGProc::GPGProc(QObject *parent)
	: KProcess(parent), d( new GPGProcPrivate() )
{
	QStringList args;
	args << "--no-secmem-warning" << "--no-tty" << "--with-colons";
	setProgram(KGpgSettings::gpgBinaryPath(), args);
	setOutputChannelMode(OnlyStdoutChannel);
}

GPGProc::~GPGProc()
{
	delete d;
}

void GPGProc::start()
{
	connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
		this, SLOT(finished()));

	connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(received()));

	KProcess::start();
}

void GPGProc::received()
{
	d->recvbuffer.append(readAllStandardOutput());

	if (d->recvbuffer.indexOf('\n') >= 0)
		emit readReady(this);
}

void GPGProc::finished()
{
	emit processExited(this);
}

int GPGProc::readln(QString &line)
{
	int len = d->recvbuffer.indexOf('\n');
	if (len < 0)
		return -1;

	// don't copy '\n'
	QByteArray a = d->recvbuffer.mid(0, len);
	d->recvbuffer.remove(0, len + 1);

	int pos = 0;

	while ( (pos = a.indexOf("\\x", pos)) >= 0) {
		if (pos > a.length() - 4)
			break;

		char c1, c2;
		c1 = a[pos + 2];
		c2 = a[pos + 3];

		// ':' must be skipped, it is used as colon delimiter
		// since it is pure ascii it can be replaced in QString.
		if ((c1 == '3') && ((c2 == 'a') || (c2 == 'A'))) {
			pos += 3;
			continue;
		}

		if (!isxdigit(c1) || !isxdigit(c2))
			continue;

		char n[2] = { 0, 0 };

		if ((c1 >= '0') && (c1 <= '9'))
			n[0] = c1 - '0';
		else
			n[0] = tolower(c1) - 'a' + 10;
		n[0] *= 16;

		if ((c2 >= '0') && (c2 <= '9'))
			n[0] += c2 - '0';
		else
			n[0] += tolower(c2) - 'a' + 10;

		// it is likely to find the same byte sequence more than once
		int npos = pos;
		QByteArray pattern = a.mid(pos, 4);
		do {
			a.replace(npos, 4, n);
		} while ( (npos = a.indexOf(pattern, npos)) >= 0 );
	}

	line = d->codec->toUnicode(a);

	return line.length();
}

int GPGProc::readln(QStringList &l)
{
	QString s;

	int len = readln(s);
	if (len < 0)
		return len;

	l = s.split(':');

	for (int i = 0; i < l.count(); i++) {
		int j = 0;

		while ( (j = l[i].indexOf("\\x3a", j, Qt::CaseInsensitive)) >= 0 ) {
			l[i].replace(j, 4, ':');
			j++;
		}
	}

	return l.count();
}

#include "gpgproc.moc"
