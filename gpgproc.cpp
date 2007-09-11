/* This file was part of the KDE libraries
   Copyright (C) 1997 David Sweet <dsweet@kde.org>
   Copyright (C) 2007 Rolf Eike Beer <kde@opensource.sf-tec.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

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
        : codec( QTextCodec::codecForName("utf8") ),
          rbi( 0 ),
          readsignalon( true ),
          comm(K3Process::All)
    {
    }

    QByteArray recvbuffer;
    QTextCodec *codec;
    int rbi;
    bool needreadsignal;
    bool readsignalon;
    K3Process::Communication comm;
};

GPGProc::GPGProc()
  : d( new GPGProcPrivate() )
{
    arguments.append(QFile::encodeName(KGpgSettings::gpgBinaryPath()));
    arguments.append(QFile::encodeName("--no-secmem-warning"));
    arguments.append(QFile::encodeName("--no-tty"));
    arguments.append(QFile::encodeName("--with-colons"));
}

GPGProc::~GPGProc()
{
    delete d;
}

bool GPGProc::start(RunMode runmode, bool includeStderr)
{
  connect(this, SIGNAL(receivedStdout(K3Process *, char *, int)),
	   this, SLOT(received(K3Process *, char *, int)));

  if (includeStderr)
  {
     connect(this, SIGNAL(receivedStderr(K3Process *, char *, int)),
              this, SLOT(received(K3Process *, char *, int)));
  }

  return K3Process::start(runmode, d->comm);
}

void GPGProc::received(K3Process *, char *buffer, int buflen)
{
  d->recvbuffer += QByteArray(buffer, buflen);

  controlledEmission();
}

void GPGProc::ackRead()
{
    d->readsignalon = true;
    if ( d->needreadsignal || d->recvbuffer.length() != 0 ) {
        controlledEmission();
    }
}

void GPGProc::controlledEmission()
{
    if ( d->readsignalon ) {
        d->needreadsignal = false;
        d->readsignalon = false; //will stay off until read is acknowledged
        emit readReady(this);
    } else {
        d->needreadsignal = true;
    }
}

void GPGProc::enableReadSignals(bool enable)
{
    d->readsignalon = enable;

    if ( enable && d->needreadsignal ) {
        emit readReady(this);
    }
}

int GPGProc::readln(QString &line, bool autoAck, bool *partial)
{
  int len;

  if ( autoAck ) {
     d->readsignalon=true;
  }

  len = d->recvbuffer.indexOf('\n', d->rbi) - d->rbi;

  //in case there's no '\n' at the end of the buffer
  if ( ( len < 0 ) &&
       ( d->rbi < d->recvbuffer.length() ) ) {
     d->recvbuffer = d->recvbuffer.mid( d->rbi );
     d->rbi = 0;
     if (partial)
     {
        len = d->recvbuffer.length();
        line = d->recvbuffer;
        d->recvbuffer = "";
        *partial = true;
        return len;
     }
     return -1;
  }

  if (len>=0)
  {
     int pos = 0;
     while ( (pos = d->recvbuffer.indexOf("\\x", pos)) >= 0) {
       if (pos > d->recvbuffer.length() - 4)
         break;

       char c1, c2;
       c1 = d->recvbuffer[pos + 2];
       c2 = d->recvbuffer[pos + 3];

       if (!isxdigit(c1) || !isxdigit(c2))
         continue;

       unsigned char n[2] = { 0, 0 };

       if ((c1 >= '0') && (c1 <= '9'))
         n[0] = c1 - '0';
       else
         n[0] = tolower(c1) - 'a' + 10;
       n[0] *= 16;

       if ((c2 >= '0') && (c2 <= '9'))
         n[0] += c2 - '0';
       else
         n[0] += tolower(c2) - 'a' + 10;

       // this must be skipped, the ':' is used as colon delimiter
       // since it is pure ascii it can be replaced in QString.
       if (n[0] == ':') {
         pos += 3;
         continue;
       }

       // it is very likely to find the same byte sequence more than once
       int npos = 0;
       QByteArray pattern = d->recvbuffer.mid(pos, 4);
       while ( (npos = d->recvbuffer.indexOf(pattern, npos)) >= 0) {
         d->recvbuffer.replace(npos, 4, (char *)&n);
	 if (npos < len)
	   len -= 3;
       }
     }


     line = d->codec->toUnicode( d->recvbuffer.mid( d->rbi, len ) );
     if (d->rbi + len + 1 >= 4096) {
       d->recvbuffer.remove(0, d->rbi + len + 1);
       d->rbi = 0;
     } else
       d->rbi += len + 1;
     if (partial)
       partial = false;
     return len;
  }

  d->recvbuffer = "";
  d->rbi = 0;

  //-1 on return signals "no more data" not error
  return -1;

}

#include "gpgproc.moc"
