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

#include <QSharedDataPointer>
#include <QSharedData>
#include <QPointer>
#include <QObject>
#include <QString>
#include <QColor>
#include <QList>
#include <QChar>
#include <QDate>

class KgpgKey;

namespace Kgpg
{
    enum KeyAlgoFlag
    {
        DSA_ELGAMAL = 1,
        RSA = 2
    };
    Q_DECLARE_FLAGS(KeyAlgo, KeyAlgoFlag)
    Q_DECLARE_OPERATORS_FOR_FLAGS(KeyAlgo)
}

class KgpgKeySignPrivate : public QSharedData
{
public:
    bool    gpgsignrevocation;
    QString gpgsignid;
    QString gpgsignname;
    QString gpgsignemail;
    QString gpgsigncomment;
    bool    gpgsignunlimited;
    QDate   gpgsignexpiration;
    QDate   gpgsigncreation;
    bool    gpgsignlocal;

    bool operator==(const KgpgKeySignPrivate &other) const;
    bool operator!=(const KgpgKeySignPrivate &other) const;
};

class KgpgKeySign : public QObject
{
public:
    KgpgKeySign();
    KgpgKeySign(const KgpgKeySign &other);

    void setId(const QString &id);
    void setName(const QString &name);
    void setEmail(const QString &email);
    void setComment(const QString &comment);
    void setUnlimited(const bool &unlimited);
    void setExpiration(const QDate &date);
    void setCreation(const QDate &date);
    void setLocal(const bool &local);
    void setRevocation(const bool &revoc);

    QString id() const;
    QString name() const;
    QString email() const;
    QString comment() const;
    bool unlimited() const;
    QDate expirationDate() const;
    QDate creationDate() const;
    bool local() const;
    bool revocation() const;

    QString expiration() const;
    QString creation() const;

    bool operator==(const KgpgKeySign &other) const;
    bool operator!=(const KgpgKeySign &other) const;
    KgpgKeySign& operator=(const KgpgKeySign &other);

private:
    QSharedDataPointer<KgpgKeySignPrivate> d;
};

class KgpgKeySignList : public QList<KgpgKeySign>
{
public:
    inline KgpgKeySignList() { }
    inline explicit KgpgKeySignList(const KgpgKeySign &sign) { append(sign); }
    inline KgpgKeySignList(const KgpgKeySignList &other) : QList<KgpgKeySign>(other) { }
    inline KgpgKeySignList(const QList<KgpgKeySign> &other) : QList<KgpgKeySign>(other) { }

    inline KgpgKeySignList operator+(const KgpgKeySignList &other) const
    {
        KgpgKeySignList n = *this;
        n += other;
        return n;
    }

    inline KgpgKeySignList &operator<<(KgpgKeySign sign)
    {
        append(sign);
        return *this;
    }

    inline KgpgKeySignList &operator<<(const KgpgKeySignList &l)
    {
        *this += l;
        return *this;
    }
};
typedef QPointer<KgpgKeySignList> KgpgKeySignListPtr;

class KgpgKeyUatPrivate : public QSharedData
{
public:
    QString gpguatid;
    KgpgKeySignList gpgsignlist;

    bool operator==(const KgpgKeyUatPrivate &other) const;
    bool operator!=(const KgpgKeyUatPrivate &other) const;
};

class KgpgKeyUat : public QObject
{
public:
    KgpgKeyUat();
    KgpgKeyUat(const KgpgKeyUat &other);

    void setId(const QString &id);
    QString id() const;

    void addSign(const KgpgKeySign &sign);
    KgpgKeySignList signList();

    bool operator==(const KgpgKeyUat &other) const;
    bool operator!=(const KgpgKeyUat &other) const;
    KgpgKeyUat& operator=(const KgpgKeyUat &other);

private:
    QSharedDataPointer<KgpgKeyUatPrivate> d;
};

class KgpgKeyUatList : public QList<KgpgKeyUat>, public QObject
{
public:
    inline KgpgKeyUatList() { }
    inline explicit KgpgKeyUatList(const KgpgKeyUat &uat) { append(uat); }
    inline KgpgKeyUatList(const KgpgKeyUatList &other) : QList<KgpgKeyUat>(other), QObject() { }
    inline KgpgKeyUatList(const QList<KgpgKeyUat> &other) : QList<KgpgKeyUat>(other), QObject() { }

    inline KgpgKeyUatList operator+(const KgpgKeyUatList &other) const
    {
        KgpgKeyUatList n = *this;
        n += other;
        return n;
    }

    inline KgpgKeyUatList &operator<<(KgpgKeyUat uat)
    {
        append(uat);
        return *this;
    }

    inline KgpgKeyUatList &operator<<(const KgpgKeyUatList &l)
    {
        *this += l;
        return *this;
    }
};
typedef QPointer<KgpgKeyUatList> KgpgKeyUatListPtr;

class KgpgKeyUidPrivate : public QSharedData
{
public:
    bool    gpguidvalide;
    QChar   gpguidtrust;
    QString gpguidname;
    QString gpguidemail;
    QString gpguidcomment;
    KgpgKeySignList gpgsignlist;

    bool operator==(const KgpgKeyUidPrivate &other) const;
    bool operator!=(const KgpgKeyUidPrivate &other) const;
};

class KgpgKeyUid : public QObject
{
public:
    KgpgKeyUid();
    KgpgKeyUid(const KgpgKeyUid &other);

    void setName(const QString &name);
    void setEmail(const QString &email);
    void setComment(const QString &comment);
    void setValide(const bool &valide);
    void setTrust(const QChar &c);
    void setTrust(const QString &s);

    QString name() const;
    QString email() const;
    QString comment() const;
    bool valide() const;
    QChar trust() const;

    void addSign(const KgpgKeySign &sign);
    KgpgKeySignList signList();

    bool operator==(const KgpgKeyUid &other) const;
    bool operator!=(const KgpgKeyUid &other) const;
    KgpgKeyUid& operator=(const KgpgKeyUid &other);

private:
    QSharedDataPointer<KgpgKeyUidPrivate> d;
};

class KgpgKeyUidList : public QList<KgpgKeyUid>, public QObject
{
public:
    inline KgpgKeyUidList() { }
    inline explicit KgpgKeyUidList(const KgpgKeyUid &uid) { append(uid); }
    inline KgpgKeyUidList(const KgpgKeyUidList &other) : QList<KgpgKeyUid>(other), QObject() { }
    inline KgpgKeyUidList(const QList<KgpgKeyUid> &other) : QList<KgpgKeyUid>(other), QObject() { }

    inline KgpgKeyUidList operator+(const KgpgKeyUidList &other) const
    {
        KgpgKeyUidList n = *this;
        n += other;
        return n;
    }

    inline KgpgKeyUidList &operator<<(KgpgKeyUid uid)
    {
        append(uid);
        return *this;
    }

    inline KgpgKeyUidList &operator<<(const KgpgKeyUidList &l)
    {
        *this += l;
        return *this;
    }
};
typedef QPointer<KgpgKeyUidList> KgpgKeyUidListPtr;

class KgpgKeySubPrivate : public QSharedData
{
public:
    bool            gpgsubvalide;
    QString         gpgsubid;
    QString         gpgsubsize;
    bool            gpgsubunlimited;
    QDate           gpgsubexpiration;
    QDate           gpgsubcreation;
    QChar           gpgsubtrust;
    uint            gpgsubalgo;
    KgpgKeySignList gpgsignlist;

    bool operator==(const KgpgKeySubPrivate &other) const;
    bool operator!=(const KgpgKeySubPrivate &other) const;
};

class KgpgKeySub : public QObject
{
public:
    KgpgKeySub();
    KgpgKeySub(const KgpgKeySub &other);

    void setId(const QString &id);
    void setSize(const QString &size);
    void setUnlimited(const bool &unlimited);
    void setExpiration(const QDate &date);
    void setCreation(const QDate &date);
    void setTrust(const QChar &c);
    void setTrust(const QString &s);
    void setAlgorithme(const uint &algo);
    void setValide(const bool &valide);

    QString id() const;
    QString size() const;
    bool unlimited() const;
    QDate expirationDate() const;
    QDate creationDate() const;
    QChar trust() const;
    uint algorithme() const;
    bool valide() const;

    QString creation() const;
    QString expiration() const;

    void addSign(const KgpgKeySign &sign);
    KgpgKeySignList signList();

    bool operator==(const KgpgKeySub &other) const;
    bool operator!=(const KgpgKeySub &other) const;
    KgpgKeySub& operator=(const KgpgKeySub &other);

private:
    QSharedDataPointer<KgpgKeySubPrivate> d;
};

class KgpgKeySubList : public QList<KgpgKeySub>, public QObject
{
public:
    inline KgpgKeySubList() { }
    inline explicit KgpgKeySubList(const KgpgKeySub &sub) { append(sub); }
    inline KgpgKeySubList(const KgpgKeySubList &other) : QList<KgpgKeySub>(other), QObject() { }
    inline KgpgKeySubList(const QList<KgpgKeySub> &other) : QList<KgpgKeySub>(other), QObject() { }

    inline KgpgKeySubList operator+(const KgpgKeySubList &other) const
    {
        KgpgKeySubList n = *this;
        n += other;
        return n;
    }

    inline KgpgKeySubList &operator<<(KgpgKeySub sub)
    {
        append(sub);
        return *this;
    }

    inline KgpgKeySubList &operator<<(const KgpgKeySubList &l)
    {
        *this += l;
        return *this;
    }
};
typedef QPointer<KgpgKeySubList> KgpgKeySubListPtr;

class KgpgKeyPrivate : public QSharedData
{
public:
    KgpgKeyPrivate();

    bool    gpgkeysecret;
    bool    gpgkeyvalide;
    QString gpgkeyid;
    QString gpgfullid;
    QString gpgkeymail;
    QString gpgkeyname;
    QString gpgkeycomment;
    QString gpgkeyfingerprint;
    QString gpgkeysize;
    QChar   gpgkeyownertrust;
    QChar   gpgkeytrust;
    QDate   gpgkeycreation;
    bool    gpgkeyunlimited;
    QDate   gpgkeyexpiration;
    uint    gpgkeyalgo;

    KgpgKeySignList gpgsignlist;
    KgpgKeyUatListPtr gpguatlist;
    KgpgKeyUidListPtr gpguidlist;
    KgpgKeySubListPtr gpgsublist;

    bool operator==(const KgpgKeyPrivate &other) const;
    bool operator!=(const KgpgKeyPrivate &other) const;
};

class KgpgKey : public QObject
{
public:
    static QString creation(const QDate &date);
    static QString expiration(const QDate &date, const bool &unlimited);
    static QString algorithmeToString(const int &v);
    static QString trustToString(const QChar &c);
    static QColor color(const QChar &c);
    static QString ownerTrustToString(const QChar &c);
    static int ownerTrustIndex(const QChar &c);  // FIXME keep here ?

    KgpgKey();
    KgpgKey(const KgpgKey &other);

    void setSecret(const bool &secret);
    void setValide(const bool &valide);
    void setId(const QString &id);
    void setFullId(const QString &fullid);
    void setName(const QString &name);
    void setEmail(const QString &email);
    void setComment(const QString &comment);
    void setFingerprint(const QString &fingerprint);
    void setSize(const QString &size);
    void setOwnerTrust(const QChar &c);
    void setOwnerTrust(const QString &s);
    void setTrust(const QChar &c);
    void setTrust(const QString &s);
    void setCreation(const QDate &date);
    void setExpiration(const QDate &date);
    void setUnlimited(const bool &unlimited);
    void setAlgorithme(const uint &algo);

    bool secret() const;
    bool valide() const;
    QString id() const;
    QString fullId() const;
    QString name() const;
    QString email() const;
    QString comment() const;
    QString fingerprint() const;
    QString size() const;
    QChar ownerTrust() const;
    QChar trust() const;
    QDate creationDate() const;
    QDate expirationDate() const;
    bool unlimited() const;
    uint algorithme() const;

    QString creation() const;
    QString expiration() const;
    QStringList photoList() const;

    void addSign(const KgpgKeySign &sign);
    KgpgKeySignList signList();

    KgpgKeyUatListPtr uatList();
    KgpgKeyUidListPtr uidList();
    KgpgKeySubListPtr subList();

    bool operator==(const KgpgKey &other) const;
    bool operator!=(const KgpgKey &other) const;
    KgpgKey& operator=(const KgpgKey &other);

private:
    QSharedDataPointer<KgpgKeyPrivate> d;
};

class KgpgListKeys : public QList<KgpgKey>, public QObject
{
public:
    inline KgpgListKeys() { }
    inline explicit KgpgListKeys(const KgpgKey &key) { append(key); }
    inline KgpgListKeys(const KgpgListKeys &other) : QList<KgpgKey>(other), QObject() { }
    inline KgpgListKeys(const QList<KgpgKey> &other) : QList<KgpgKey>(other), QObject() { }

    inline KgpgListKeys& operator=(const KgpgListKeys &other)
    {
        QList<KgpgKey>::operator=(static_cast<const QList<KgpgKey> >(other));
        return *this;
    }

    inline KgpgListKeys operator+(const KgpgListKeys &other) const
    {
        KgpgListKeys n = *this;
        n += other;
        return n;
    }

    inline KgpgListKeys &operator<<(KgpgKey key)
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
typedef QPointer<KgpgListKeys> KgpgListKeysPtr;

#endif // KGPGKEY_H
