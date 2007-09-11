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
#ifndef GPGPROC_H
#define GPGPROC_H

#include <k3process.h>

#include <QtCore/QString>

class GPGProcPrivate;

/**
 * This is more or less a copy of K3ProcIO.
 *
 * In addition it handles the GnuPG formatted UTF8 output correctly.
 * GnuPG recodes some characters as \\xnn where nn is the hex representation
 * of the character. This can't be fixed up simply when using QString as
 * QString already did it's own UTF8 conversion. Therefore we replace this
 * sequences by their corresponding character so QString will work just fine.
 *
 * As we know that GnuPG limits it's columns by ':' we skip \\x3a. Since this
 * is an ascii character (single byte) the replacement can be done later without
 * problems after the line has been split into pieces.
 *
 * Some functions not needed in KGpg are removed. Since the base class K3Process
 * is marked as obsolete this has to be ported to KProcess.
 *
 * @author David Sweet, Rolf Eike Beer
 * @short A interface to GnuPG handling UTF8 recoding correctly
 **/


class GPGProc : public K3Process
{
  Q_OBJECT

public:
  /**
   * Constructor
   */
  explicit GPGProc();

  /**
   * Destructor
   */
  ~GPGProc();

  /**
   *  Starts the process. It will fail in the following cases:
   *  @li The process is already running.
   *  @li The command line argument list is empty.
   *  @li The starting of the process failed (could not fork).
   *  @li The executable was not found.
   *
   *  @param runmode For a detailed description of the
   *  various run modes, have a look at the
   *  general description of the K3Process class.
   *  @param includeStderr If true, data from both stdout and stderr is
   *  listened to. If false, only stdout is listened to.
   *  @return true on success, false on error.
   **/
  bool start(RunMode runmode = NotifyOnExit, bool includeStderr = false);

  /**
   * Reads a line of text (up to and including '\\n').
   *
   * Use readln() in response to a readReady() signal.
   * You may use it multiple times if more than one line of data is
   *  available.
   * Be sure to use ackRead() when you have finished processing the
   *  readReady() signal.  This informs GPGProc that you are ready for
   *  another readReady() signal.
   *
   * readln() never blocks.
   *
   * autoAck==true makes these functions call ackRead() for you.
   *
   * @param line is used to store the line that was read.
   * @param autoAck when true, ackRead() is called for you.
   * @param partial when provided the line is returned
   * even if it does not contain a '\\n'. *partial will be set to
   * false if the line contains a '\\n' and false otherwise.
   * @return the number of characters read, or -1 if no data is available.
   **/
  int readln(QString &line, bool autoAck=true, bool *partial = NULL);

  /**
   * Call this after you have finished processing a readReady()
   * signal.  This call need not be made in the slot that was signalled
   * by readReady().  You won't receive any more readReady() signals
   * until you acknowledge with ackRead().  This prevents your slot
   * from being reentered while you are still processing the current
   * data.  If this doesn't matter, then call ackRead() right away in
   * your readReady()-processing slot.
   **/
  void ackRead();

  /**
   *  Turns readReady() signals on and off.
   *   You can turn this off at will and not worry about losing any data.
   *   (as long as you turn it back on at some point...)
   * @param enable true to turn the signals on, false to turn them off
   */
  void enableReadSignals(bool enable);

Q_SIGNALS:
  /**
   * Emitted when the process is ready for reading.
   * @param pio the process that emitted the signal
   * @see enableReadSignals()
   */
  void readReady(GPGProc *pio);

protected:
  void controlledEmission();

protected Q_SLOTS:
  void received(K3Process *proc, char *buffer, int buflen);

private:
  GPGProcPrivate* const d;
};

#endif // GPGPROC_H
