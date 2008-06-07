/**
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007 Jimmy Gilles <jimmygilles@gmail.com>
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

#ifndef KGPGKEYINFODIALOG_H
#define KGPGKEYINFODIALOG_H

#include <QPixmap>
#include <QString>
#include <QLabel>
#include <QColor>
#include <QDate>
#include <QPushButton>

#include <KComboBox>
#include <KDialog>

#include "kgpgkey.h"

class QCheckBox;
class QGroupBox;

class KDatePicker;
class KUrlLabel;

class KgpgInterface;

class KgpgTrustLabel : public QWidget
{
    Q_OBJECT

public:
    explicit KgpgTrustLabel(QWidget *parent = 0, const QString &text = QString(), const QColor &color = QColor());

    void setText(const QString &text);
    void setColor(const QColor &color);

    QString text() const;
    QColor color() const;

private:
    void change();

    QLabel *m_text_w;
    QLabel *m_color_w;

    QString m_text;
    QColor m_color;
};

class KgpgDateDialog : public KDialog
{
    Q_OBJECT

public:
    explicit KgpgDateDialog(QWidget *parent = 0, const bool &unlimited = false, QDate date = QDate::currentDate());

    QDate date() const;
    bool unlimited() const;

private slots:
    void slotCheckDate(const QDate &date);
    void slotEnableDate(const bool &ison);

private:
    QCheckBox *m_unlimited;
    KDatePicker *m_datepicker;
};

class KgpgKeyInfo : public KDialog
{
    Q_OBJECT

public:
    explicit KgpgKeyInfo(KgpgCore::KgpgKey *key, QWidget *parent = 0);
    ~KgpgKeyInfo();

signals:
    void keyNeedsRefresh(const QString &keyid);

private:
    QGroupBox *_keypropertiesGroup(QWidget *parent);
    QGroupBox *_photoGroup(QWidget *parent);
    QGroupBox *_buttonsGroup(QWidget *parent);
    QGroupBox *_fingerprintGroup(QWidget *parent);

    void reloadKey(KgpgInterface *interface);
    void displayKey();
    void setControlEnable(const bool &b);

private slots:
    void slotPreOk();
    void slotOpenUrl(const QString &url) const;

    void slotChangeDate();
    void slotInfoExpirationChanged(const int &res, KgpgInterface *interface);

    void slotDisableKey(const bool &ison);
    void slotDisableKeyFinished(KgpgInterface *interface, int);

    void slotChangePass();
    void slotInfoPasswordChanged(const int &res, KgpgInterface *interface);

    void slotChangeTrust(const int &newtrust);
    void slotInfoTrustChanged(KgpgInterface *interface);

    void slotLoadPhoto(const QString &uid);
    void slotSetPhoto(const QPixmap &pixmap, KgpgInterface *interface);

private:
    KgpgCore::KgpgKey *m_key;

    QCheckBox *m_disable;
    QLabel *m_name;
    QLabel *m_id;
    QLabel *m_comment;
    QLabel *m_creation;
    QLabel *m_expiration;
    QLabel *m_algorithm;
    QLabel *m_length;
    QLabel *m_fingerprint;
    QLabel *m_photo;
    QPushButton *m_expirationbtn;

    KUrlLabel *m_email;
    KComboBox *m_photoid;
    KComboBox *m_owtrust;

    KgpgTrustLabel *m_trust;

    bool m_hasphoto;
    bool m_isunlimited;
    bool m_keywaschanged;
};

#endif // KGPGKEYINFODIALOG_H
