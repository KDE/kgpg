/***************************************************************************
                          keyinfowidget.h  -  description
                             -------------------
    begin                : Thu Jul 4 2002
    copyright          : (C) 2002 by Jean-Baptiste Mardelle
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

#include <kdialogbase.h>
#include <qpixmap.h>

class KTempFile;
class QLabel;
class KDialogBase;
class QCheckBox;
class KDatePicker;
class KeyProperties;
class KProcess;
class KProcIO;

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
        void slotPreOk();
	void slotChangeExp();
	void slotEnableDate(bool isOn);
	void slotChangeDate();
	void slotCheckDate(QDate date);
	void openPhoto();
	void slotSetPhoto(const QPixmap &pix);
	void finishphotoreadprocess(KProcIO *p);
	void slotMainImageRead(KProcess *);
	void slotSetMainPhoto(QStringList list);
	void reloadMainPhoto(const QString &uid);
	void slotInfoPasswordChanged();
	void slotInfoExpirationChanged(int res);
	void slotInfoTrustChanged();
	void slotChangeTrust(int newTrust);
	void loadKey(QString Keyid);
	void slotDisableKey(bool isOn);

private:
        KTempFile *kgpginfotmp;
        QLabel *keyinfoPhoto;
        QString displayedKeyID;
        QString expirationDate;
        bool hasPhoto,keyWasChanged;
	KDialogBase *chdate;
	QCheckBox *kb;
	KDatePicker *kdt;
	int counter;

signals:
	
	void signalPhotoId(const QPixmap&);
	void changeMainPhoto(const QPixmap&);
	void keyNeedsRefresh();

};

#endif // KEYINFOWIDGET_H

