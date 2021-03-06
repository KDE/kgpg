/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010,2011,2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGTEXTINTERFACE_H
#define KGPGTEXTINTERFACE_H

#include <QObject>
#include <QUrl>

class KGpgTextInterfacePrivate;
class QString;
class QStringList;

class KGpgTextInterface : public QObject
{
	Q_OBJECT

private:
	KGpgTextInterfacePrivate * const d;

	KGpgTextInterface();
	Q_DISABLE_COPY(KGpgTextInterface)

public:
	explicit KGpgTextInterface(QObject *parent, const QString &keyID, const QStringList &options);
	~KGpgTextInterface();

Q_SIGNALS:
    /**
     * Emitted when all files passed to KgpgSignFile() where processed.
     */
    void fileSignFinished();

public Q_SLOTS:
    /**
     * Sign file function
     * @param srcUrl file to sign.
     */
    void signFiles(const QList<QUrl> &srcUrl);

private Q_SLOTS:
	void slotSignFile();
	void slotSignFinished();
};

#endif
