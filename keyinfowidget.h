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
#include <QDate>

#include <kdialogbase.h>

class QCheckBox;
class QLabel;

class KDialogBase;
class KDatePicker;
class KTempFile;
class KProcess;
class KProcIO;

class KgpgInterface;
class KeyProperties;

class KgpgKeyInfo : public KDialogBase
{
    Q_OBJECT

public:
    KgpgKeyInfo(QWidget *parent = 0, const char *name = 0, QString sigkey = 0);

signals:
    void signalPhotoId(const QPixmap&);
    void changeMainPhoto(const QPixmap&);
    void keyNeedsRefresh();

private:
    void loadKey(const QString &Keyid);

private slots:
    void slotPreOk();

    void slotDisableKey(const bool &ison);

    void slotChangeExp();
    void slotCheckDate(QDate date);
    void slotChangeDate();
    void slotEnableDate(bool ison);
    void slotInfoExpirationChanged(int res, KgpgInterface *interface);

    void slotChangePass();
    void slotInfoPasswordChanged(int res, KgpgInterface *interface);

    void slotChangeTrust(int newTrust);
    void slotInfoTrustChanged(KgpgInterface *interface);


    void slotinfoimgread(KProcess *);
    void openPhoto();
    void slotSetPhoto(const QPixmap &pix);
    void finishphotoreadprocess(KProcIO *p);
    void slotMainImageRead(KProcess *);
    void slotSetMainPhoto(QStringList list);
    void slotReloadMainPhoto(const QString &uid);

private:
    QString m_displayedkeyid;
    QCheckBox *m_kb;
    QDate m_date;

    KTempFile *m_kgpginfotmp;
    KDialogBase *m_chdate;
    KDatePicker *m_kdt;

    KeyProperties *m_prop;

    bool m_hasphoto;
    bool m_keywaschanged;
    bool m_isunlimited;
};

#endif // KEYINFOWIDGET_H
