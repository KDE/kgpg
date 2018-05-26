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

#ifndef KLINEBUFFEREDPROCESSPRIVATE_H
#define KLINEBUFFEREDPROCESSPRIVATE_H

class KLineBufferedProcess;

class KLineBufferedProcessPrivate : public QObject
{
public:
    explicit KLineBufferedProcessPrivate(KLineBufferedProcess *parent);

    void _k_receivedStdout();
    void _k_receivedStderr();

    QByteArray m_stdoutBuffer;
    QByteArray m_stderrBuffer;
    int m_newlineInStdout;
    int m_newlineInStderr;
    KLineBufferedProcess * const m_parent;
    const QByteArray m_lineEnd;
};

#endif // KLINEBUFFEREDPROCESSPRIVATE_H
