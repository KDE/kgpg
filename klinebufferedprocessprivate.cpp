/*
 * Copyright (C) 2008 Rolf Eike Beer <kde@opensource.sf-tec.de>
 * Copyright (C) 2016 Andrius Štikonas <andrius@stikonas.eu>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "klinebufferedprocess.h"

KLineBufferedProcessPrivate::KLineBufferedProcessPrivate(KLineBufferedProcess *parent)
 : m_newlineInStdout(-1),
   m_newlineInStderr(-1),
   m_parent(parent),
#ifdef Q_OS_WIN 	//krazy:exclude=cpp
   m_lineEnd("\r\n")
#else
   m_lineEnd("\n")
#endif
{
}

void KLineBufferedProcessPrivate::_k_receivedStdout()
{
    QByteArray ndata = m_parent->readAllStandardOutput();
    int oldBufferSize = m_stdoutBuffer.size();
    m_stdoutBuffer.append(ndata);

    if (m_newlineInStdout < 0) {
        m_newlineInStdout = ndata.indexOf(m_lineEnd);
        if (m_newlineInStdout >= 0) {
            m_newlineInStdout += oldBufferSize;
            emit m_parent->lineReadyStandardOutput();
        }
    }
}

void KLineBufferedProcessPrivate::_k_receivedStderr()
{
    QByteArray ndata = m_parent->readAllStandardError();
    int oldBufferSize = m_stderrBuffer.size();
    m_stderrBuffer.append(ndata);

   if (m_newlineInStderr < 0) {
        m_newlineInStderr = ndata.indexOf(m_lineEnd);
        if (m_newlineInStderr >= 0) {
            m_newlineInStderr += oldBufferSize;
            emit m_parent->lineReadyStandardError();
        }
    }
}
