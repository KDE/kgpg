/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGKEY_H
#define KGPGKEY_H

#include <QPointer>
#include <QObject>
#include <QString>
#include <QColor>
#include <QList>
#include <QChar>
#include <QDate>

#include <kdebug.h>

class KgpgKey : public QObject
{
public:
    bool    gpgsecretkey;
    bool    gpghasphoto;
    bool    gpgkeyvalide;
    QString gpgkeyid;
    QString gpgfullid;
    QString gpgkeymail;
    QString gpgkeyname;
    QString gpgkeycomment;
    QString gpgkeyfingerprint;
    QString gpgkeysize;
    QChar   gpgkeytrust;
    QChar   gpgkeyownertrust;
    QDate   gpgkeycreation;
    bool    gpgkeyunlimited;
    QDate   gpgkeyexpiration;
    uint    gpgkeyalgo;

    inline ~KgpgKey()
    {
        kdDebug(2100) << "Key " << gpgkeyid << " is deleted (Address : " << this << ")" << endl;
    }

    static QString algorithme(const int &v);
    static QString trust(const QChar &c);
    static QColor color(const QChar &c);
    static QString ownerTrust(const QChar &c);
    static int ownerTrustIndex(const QChar &c);
};

typedef QPointer<KgpgKey> KgpgKeyPtr;

// FIXME : why KgpgKeyPtrs are not automatically deleted ?
class KgpgListKeys : public QList<KgpgKeyPtr>
{
public:
    inline KgpgListKeys()
    {
    }

    inline KgpgListKeys(KgpgKeyPtr key)
    {
        append(key);
    }

    inline KgpgListKeys(const KgpgListKeys &other) : QList<KgpgKeyPtr>(other)
    {
    }

    inline KgpgListKeys(const QList<KgpgKeyPtr> &other) : QList<KgpgKeyPtr>(other)
    {
    }

    inline KgpgListKeys operator+(const KgpgListKeys &other) const
    {
        KgpgListKeys n = *this;
        n += other;
        return n;
    }

    inline KgpgListKeys &operator<<(KgpgKeyPtr key)
    {
        append(key);
        return *this;
    }

    inline KgpgListKeys &operator<<(const KgpgListKeys &l)
    {
        *this += l;
        return *this;
    }
};

#endif // KGPGKEY_H
