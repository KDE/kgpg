/***************************************************************************
                          keyinfowidget.h  -  description
                             -------------------
    begin                : Thu Jul 4 2002
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

#ifndef KEYINFOWIDGET_H
#define KEYINFOWIDGET_H

#include <qcolor.h>
#include <qstring.h>
#include <qcheckbox.h>
#include <qfile.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qimage.h>


#include <kactivelabel.h>
#include <kprocess.h>
#include <klineedit.h>
#include <kurl.h>
#include <ktempfile.h>
#include <kglobal.h>
#include <kdialogbase.h>
#include <kdatepicker.h>
#include <klocale.h>
#include <kcombobox.h>
#include <ktrader.h>
#include <kservice.h>
#include <kfiledialog.h>
#include <kiconview.h>

#include "keyproperties.h"
#include "kgpginterface.h"

class KgpgKeyInfo : public KDialogBase
{
        Q_OBJECT

public:

	KgpgKeyInfo( QWidget *parent = 0, const char *name = 0,QString sigkey=0);
	~KgpgKeyInfo();
	KeyProperties *prop;

private slots:
        void slotinfoimgread(KProcess *);
        void slotChangePass();
        void slotPreOk1();
        void slotPreOk2(int);
	void slotChangeExp();
	void slotEnableDate(bool isOn);
	void slotChangeDate();
	void openPhoto();
	void slotSetPhoto(const QPixmap &pix);
	void finishphotoreadprocess(KProcIO *p);
	void slotMainImageRead(KProcess *);
	void slotSetMainPhoto(QStringList list);
	void reloadMainPhoto(const QString &uid);

private:
        KTempFile *kgpginfotmp;
        QLabel *keyinfoPhoto;
        QString displayedKeyID,ownerTrust;
        QString expirationDate;
        bool isUnlimited,hasPhoto;
	KDialogBase *chdate;
	QCheckBox *kb;
	KDatePicker *kdt;
	int counter;

signals:
void signalPhotoId(const QPixmap&);
void changeMainPhoto(const QPixmap&);
};

#endif
