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

#include <QPixmap>

#include <kdialogbase.h>

class QCheckBox;
class QLabel;

class KeyProperties;
class KDialogBase;
class KDatePicker;
class KTempFile;
class KProcess;
class KProcIO;

class KgpgInterface;

class KgpgKeyInfo : public KDialogBase
{
    Q_OBJECT

public:
    KgpgKeyInfo(QWidget *parent = 0, const char *name = 0, QString sigkey = 0);
    ~KgpgKeyInfo();

signals:
    void signalPhotoId(const QPixmap&);
    void changeMainPhoto(const QPixmap&);
    void keyNeedsRefresh();

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
    void slotInfoExpirationChanged(int res, KgpgInterface *interface);
    void slotInfoTrustChanged();
    void slotChangeTrust(int newTrust);
    void loadKey(QString Keyid);
    void slotDisableKey(bool isOn);

private:
    QLabel *keyinfoPhoto;
    QString displayedKeyID;
    QString expirationDate;
    QCheckBox *kb;

    KTempFile *kgpginfotmp;
    KDialogBase *chdate;
    KDatePicker *kdt;

    KeyProperties *prop;

    bool hasPhoto;
    bool keyWasChanged;
    int counter;
};

#endif // KEYINFOWIDGET_H
