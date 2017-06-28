/*
 * Copyright (C) 2008 Rolf Eike Beer <kde@opensource.sf-tec.de>
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

KLineBufferedProcess::KLineBufferedProcess(QObject *parent)
 : KProcess(parent),
   d(new KLineBufferedProcessPrivate(this))
{
    connect(this, &KLineBufferedProcess::readyReadStandardOutput, d, &KLineBufferedProcessPrivate::_k_receivedStdout);
    connect(this, &KLineBufferedProcess::readyReadStandardError, d, &KLineBufferedProcessPrivate::_k_receivedStderr);
}

KLineBufferedProcess::~KLineBufferedProcess()
{
}

bool KLineBufferedProcess::readLineStandardOutput(QByteArray *line)
{
    if (d->m_newlineInStdout < 0) {
        return false;
    }

    // don't copy '\n'
    *line = d->m_stdoutBuffer.left(d->m_newlineInStdout);
    d->m_stdoutBuffer.remove(0, d->m_newlineInStdout + d->m_lineEnd.length());

    d->m_newlineInStdout = d->m_stdoutBuffer.indexOf(d->m_lineEnd);

    return true;
}

bool KLineBufferedProcess::readLineStandardError(QByteArray *line)
{
    if (d->m_newlineInStderr < 0) {
        return false;
    }

    // don't copy '\n'
    *line = d->m_stderrBuffer.left(d->m_newlineInStderr);
    d->m_stderrBuffer.remove(0, d->m_newlineInStderr + d->m_lineEnd.length());

    d->m_newlineInStderr = d->m_stderrBuffer.indexOf(d->m_lineEnd);

    return true;
}

bool KLineBufferedProcess::hasLineStandardOutput() const
{
    return d->m_newlineInStdout >= 0;
}

bool KLineBufferedProcess::hasLineStandardError() const
{
    return d->m_newlineInStderr >= 0;
}
