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

#include <KDialog>

#include "kgpgkey.h"

class QCheckBox;
class QGroupBox;

class KPushButton;
class KDatePicker;
class KUrlLabel;
class KComboBox;

class KgpgInterface;
class KGpgKeyNode;
class KGpgChangeKey;
class KGpgChangePass;

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
    explicit KgpgDateDialog(QWidget *parent = 0, QDate date = QDate());

    QDate date() const;

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
    explicit KgpgKeyInfo(KGpgKeyNode *node, QWidget *parent = 0);
    explicit KgpgKeyInfo(KgpgCore::KgpgKey *key, QWidget *parent = 0);
    ~KgpgKeyInfo();

    KGpgChangeKey *keychange;

signals:
    void keyNeedsRefresh(KGpgKeyNode *node);

private:
    QGroupBox *_keypropertiesGroup(QWidget *parent);
    QGroupBox *_photoGroup(QWidget *parent);
    QGroupBox *_buttonsGroup(QWidget *parent);
    QGroupBox *_fingerprintGroup(QWidget *parent);

    void reloadKey();
    void displayKey();
    void setControlEnable(const bool &b);

private slots:
    void slotPreOk();
    void slotPreCancel();
    void slotOpenUrl(const QString &url) const;

    void slotChangeDate();

    void slotDisableKey(const bool &ison);

    void slotChangePass();
    void slotInfoPasswordChanged(int result);

    void slotChangeTrust(const int &newtrust);

    void slotLoadPhoto(const QString &uid);
    void slotSetPhoto(const QPixmap &pixmap, KgpgInterface *interface);

    void slotApply();
    void slotApplied(int result);
    
private:
	const KgpgCore::KgpgKey *m_key;
	KGpgKeyNode *m_node;
	KGpgChangePass *m_changepass;

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

    KPushButton *m_expirationbtn;
    KPushButton *m_password;
    KUrlLabel *m_email;
    KComboBox *m_photoid;
    KComboBox *m_owtrust;

    KgpgTrustLabel *m_trust;

    bool m_hasphoto;
    bool m_keywaschanged;

    void init();
};

#endif // KGPGKEYINFODIALOG_H
