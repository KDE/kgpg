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

#ifndef __KGPGKEYINFOWIDGET_H__
#define __KGPGKEYINFOWIDGET_H__

#include <QPixmap>
#include <QString>
#include <QDate>

#include <kdialogbase.h>

class QCheckBox;

class KDatePicker;

class KgpgInterface;
class KeyProperties;

class KgpgKeyInfo : public KDialogBase
{
    Q_OBJECT

public:
    KgpgKeyInfo(const QString &sigkey, QWidget *parent = 0, const char *name = 0);

signals:
    void changeMainPhoto(const QPixmap&);
    void keyNeedsRefresh();

private:
    void loadKey(const QString &Keyid);

private slots:
    void slotPreOk();

    void slotDisableKey(const bool &ison);
    void slotDisableKeyFinished(KgpgInterface *interface);

    void slotChangeExp();
    void slotCheckDate(const QDate &date);
    void slotChangeDate();
    void slotEnableDate(const bool &ison);
    void slotInfoExpirationChanged(const int &res, KgpgInterface *interface);

    void slotChangePass();
    void slotInfoPasswordChanged(const int &res, KgpgInterface *interface);

    void slotChangeTrust(const int &newtrust);
    void slotInfoTrustChanged(KgpgInterface *interface);

    void slotReloadMainPhoto(const QString &uid);
    void slotMainImageRead(const QPixmap &pixmap, KgpgInterface *interface);
    void slotSetPhoto(const QPixmap &pixmap);

private:
    QString m_displayedkeyid;
    QCheckBox *m_kb;
    QDate m_date;

    KDialogBase *m_chdate;
    KDatePicker *m_kdt;

    KeyProperties *m_prop;

    bool m_hasphoto;
    bool m_keywaschanged;
    bool m_isunlimited;
};

#endif // __KGPGKEYINFOWIDGET_H__
