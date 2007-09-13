/***************************************************************************
                          keyinfodialog.h  -  description
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

#ifndef KGPGKEYINFODIALOG_H
#define KGPGKEYINFODIALOG_H

#include <QPixmap>
#include <QString>
#include <QLabel>
#include <QColor>
#include <QDate>

#include <KComboBox>
#include <KDialog>

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
    explicit KgpgKeyInfo(const QString &keyid, QWidget *parent = 0);

signals:
    void keyNeedsRefresh(); // TODO add the keyid parameter

private:
    QGroupBox *_keypropertiesGroup(QWidget *parent);
    QGroupBox *_photoGroup(QWidget *parent);
    QGroupBox *_buttonsGroup(QWidget *parent);
    QGroupBox *_fingerprintGroup(QWidget *parent);

    void loadKey();

private slots:
    void slotPreOk();
    void slotOpenUrl(const QString &url) const;

    void slotChangeDate();
    void slotInfoExpirationChanged(const int &res, KgpgInterface *interface);

    void slotDisableKey(const bool &ison);
    void slotDisableKeyFinished(KgpgInterface *interface);

    void slotChangePass();
    void slotInfoPasswordChanged(const int &res, KgpgInterface *interface);

    void slotChangeTrust(const int &newtrust);
    void slotInfoTrustChanged(KgpgInterface *interface);

    void slotLoadPhoto(const QString &uid);
    void slotSetPhoto(const QPixmap &pixmap, KgpgInterface *interface);

private:
    QString m_keyid;
    QDate m_expirationdate;

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

    KUrlLabel *m_email;
    KComboBox *m_photoid;
    KComboBox *m_owtrust;

    KgpgTrustLabel *m_trust;

    bool m_hasphoto;
    bool m_isunlimited;
    bool m_keywaschanged;
};

#endif // KGPGKEYINFODIALOG_H
