/***************************************************************************
                          kgpgshredwidget.h  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright            : (C) 2002 by y0k0
    email                : bj@altern.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGSHREDW_H
#define KGPGSHREDW_H

#include <qwidget.h>
#include <qfile.h>
#include <qlabel.h>

#include <kurl.h>
#include <kshred.h>
#include <kprogress.h>
#include <kmessagebox.h>
#include <klocale.h>

#include "kgpgshred.h"

class kgpgShredWidget : public KgpgShred
{
        Q_OBJECT
public:
        kgpgShredWidget(QWidget *parent=0, const char *name=0);
        ~kgpgShredWidget();

private:
        ulong fileSize;
        KShred *shredres;

private slots:
        //void setValue(const QString & txt);
        void setValue(KIO::filesize_t);

public slots:
        void kgpgShredFile(KURL);
};

#endif
