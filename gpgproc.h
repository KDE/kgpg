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
#ifndef GPGPROC_H
#define GPGPROC_H

#include <QString>
#include <QStringList>

#include <KProcess>

class GPGProcPrivate;

/**
 * This class handles the GnuPG formatted UTF8 output correctly.
 * GnuPG recodes some characters as \\xnn where nn is the hex representation
 * of the character. This can't be fixed up simply when using QString as
 * QString already did it's own UTF8 conversion. Therefore we replace this
 * sequences by their corresponding character so QString will work just fine.
 *
 * As we know that GnuPG limits it's columns by ':' we skip \\x3a. Since this
 * is an ascii character (single byte) the replacement can be done later without
 * problems after the line has been split into pieces.
 *
 * @author Rolf Eike Beer
 * @short A interface to GnuPG handling UTF8 recoding correctly
 */
class GPGProc : public KProcess
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    explicit GPGProc(QObject *parent = 0);

    /**
     * Destructor
     */
    ~GPGProc();

    /**
     *  Starts the process
     */
    void start();

    /**
     * Reads a line of text (excluding '\\n').
     *
     * Use readln() in response to a readReady() signal.
     * You may use it multiple times if more than one line of data is
     * available.
     *
     * readln() never blocks.
     *
     * @param line is used to store the line that was read.
     * @param colons recode also colons
     * @return the number of characters read, or -1 if no data is available.
     */
    int readln(QString &line, const bool &colons = false);

    /**
     * Reads a line of text (excluding '\\n').
     *
     * Use readRawLine() in response to a readReady() signal.
     * You may use it multiple times if more than one line of data is
     * available. This does not alter the the line in any way.
     *
     * readRawLine() never blocks.
     *
     * @param line is used to store the line that was read.
     * @return the number of characters read, or -1 if no data is available.
     */
    int readRawLine(QByteArray &line);

    /**
     * Reads a line of text and splits it into parts.
     *
     * Use readln() in response to a readReady() signal.
     * You may use it multiple times if more than one line of data is
     * available.
     *
     * readln() never blocks.
     *
     * @param l is used to store the parts of the line that was read.
     * @return the number of characters read, or -1 if no data is available.
     */
    int readln(QStringList &l);

    /**
    * Recode a line from GnuPG encoding to UTF8
    *
    * @param a data to recode
    * @param colons recode also colons
    * @return recoded string
    */
    static QString recode(QByteArray a, const bool colons = true);

signals:
    /**
     * Emitted when the process is ready for reading.
     * The signal is only emitted if at least one complete line of data is ready.
     * @param p the process that emitted the signal
     */
    void readReady(GPGProc *p);

    /**
     * Emitted when the process has finished
     * @param p the process that emitted the signal
     */
    void processExited(GPGProc *p);

protected slots:
    void finished();
    void received();

private:
    GPGProcPrivate* const d;
};

#endif // GPGPROC_H
