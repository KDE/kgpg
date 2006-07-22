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
#include <QList>
#include <QDate>

class KgpgKey;

namespace KgpgCore
{

enum KeyAlgoFlag
{
    ALGO_UNKNOWN = 0,
    ALGO_RSA = 1,
    ALGO_DSA = 2,
    ALGO_ELGAMAL = 4,
    ALGO_DSA_ELGAMAL = ALGO_DSA | ALGO_ELGAMAL
};
Q_DECLARE_FLAGS(KeyAlgo, KeyAlgoFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(KeyAlgo)

enum KeyTrustFlag
{
    TRUST_UNKNOWN = 0,
    TRUST_INVALID = 1,
    TRUST_DISABLED = 2,
    TRUST_REVOKED = 3,
    TRUST_EXPIRED = 4,
    TRUST_UNDEFINED = 5,
    TRUST_NONE = 6,
    TRUST_MARGINAL = 7,
    TRUST_FULL = 8,
    TRUST_ULTIMATE = 9
};
Q_DECLARE_FLAGS(KeyTrust, KeyTrustFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(KeyTrust)

enum KeyOwnerTrustFlag
{
    OWTRUST_UNKNOWN = 0,
    OWTRUST_UNDEFINED = 1,
    OWTRUST_NONE = 2,
    OWTRUST_MARGINAL = 3,
    OWTRUST_FULL = 4,
    OWTRUST_ULTIMATE = 5
};
Q_DECLARE_FLAGS(KeyOwnerTrust, KeyOwnerTrustFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(KeyOwnerTrust)


class KeySignPrivate : public QSharedData
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

    bool operator==(const KeySignPrivate &other) const;
    bool operator!=(const KeySignPrivate &other) const;
};

class KeySign : public QObject
{
public:
    KeySign();
    KeySign(const KeySign &other);

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

    bool operator==(const KeySign &other) const;
    bool operator!=(const KeySign &other) const;
    KeySign& operator=(const KeySign &other);

private:
    QSharedDataPointer<KeySignPrivate> d;
};

class KeySignList : public QList<KeySign>
{
public:
    inline KeySignList() { }
    inline explicit KeySignList(const KeySign &sign) { append(sign); }
    inline KeySignList(const KeySignList &other) : QList<KeySign>(other) { }
    inline KeySignList(const QList<KeySign> &other) : QList<KeySign>(other) { }

    inline KeySignList operator+(const KeySignList &other) const
    {
        KeySignList n = *this;
        n += other;
        return n;
    }

    inline KeySignList &operator<<(KeySign sign)
    {
        append(sign);
        return *this;
    }

    inline KeySignList &operator<<(const KeySignList &l)
    {
        *this += l;
        return *this;
    }
};
typedef QPointer<KeySignList> KeySignListPtr;

class KeyUatPrivate : public QSharedData
{
public:
    QString gpguatid;
    KeySignList gpgsignlist;

    bool operator==(const KeyUatPrivate &other) const;
    bool operator!=(const KeyUatPrivate &other) const;
};

class KeyUat : public QObject
{
public:
    KeyUat();
    KeyUat(const KeyUat &other);

    void setId(const QString &id);
    QString id() const;

    void addSign(const KeySign &sign);
    KeySignList signList();

    bool operator==(const KeyUat &other) const;
    bool operator!=(const KeyUat &other) const;
    KeyUat& operator=(const KeyUat &other);

private:
    QSharedDataPointer<KeyUatPrivate> d;
};

class KeyUatList : public QList<KeyUat>, public QObject
{
public:
    inline KeyUatList() { }
    inline explicit KeyUatList(const KeyUat &uat) { append(uat); }
    inline KeyUatList(const KeyUatList &other) : QList<KeyUat>(other), QObject() { }
    inline KeyUatList(const QList<KeyUat> &other) : QList<KeyUat>(other), QObject() { }

    inline KeyUatList operator+(const KeyUatList &other) const
    {
        KeyUatList n = *this;
        n += other;
        return n;
    }

    inline KeyUatList &operator<<(KeyUat uat)
    {
        append(uat);
        return *this;
    }

    inline KeyUatList &operator<<(const KeyUatList &l)
    {
        *this += l;
        return *this;
    }
};
typedef QPointer<KeyUatList> KeyUatListPtr;

class KeyUidPrivate : public QSharedData
{
public:
    bool     gpguidvalide;
    KeyTrust gpguidtrust;
    QString  gpguidname;
    QString  gpguidemail;
    QString  gpguidcomment;
    KeySignList gpgsignlist;

    bool operator==(const KeyUidPrivate &other) const;
    bool operator!=(const KeyUidPrivate &other) const;
};

class KeyUid : public QObject
{
public:
    KeyUid();
    KeyUid(const KeyUid &other);

    void setName(const QString &name);
    void setEmail(const QString &email);
    void setComment(const QString &comment);
    void setValide(const bool &valide);
    void setTrust(const KeyTrust &trust);

    QString name() const;
    QString email() const;
    QString comment() const;
    bool valide() const;
    KeyTrust trust() const;

    void addSign(const KeySign &sign);
    KeySignList signList();

    bool operator==(const KeyUid &other) const;
    bool operator!=(const KeyUid &other) const;
    KeyUid& operator=(const KeyUid &other);

private:
    QSharedDataPointer<KeyUidPrivate> d;
};

class KeyUidList : public QList<KeyUid>, public QObject
{
public:
    inline KeyUidList() { }
    inline explicit KeyUidList(const KeyUid &uid) { append(uid); }
    inline KeyUidList(const KeyUidList &other) : QList<KeyUid>(other), QObject() { }
    inline KeyUidList(const QList<KeyUid> &other) : QList<KeyUid>(other), QObject() { }

    inline KeyUidList operator+(const KeyUidList &other) const
    {
        KeyUidList n = *this;
        n += other;
        return n;
    }

    inline KeyUidList &operator<<(KeyUid uid)
    {
        append(uid);
        return *this;
    }

    inline KeyUidList &operator<<(const KeyUidList &l)
    {
        *this += l;
        return *this;
    }
};
typedef QPointer<KeyUidList> KeyUidListPtr;

class KeySubPrivate : public QSharedData
{
public:
    bool            gpgsubvalide;
    QString         gpgsubid;
    uint            gpgsubsize;
    bool            gpgsubunlimited;
    QDate           gpgsubexpiration;
    QDate           gpgsubcreation;
    KeyTrust        gpgsubtrust;
    KeyAlgo         gpgsubalgo;
    KeySignList     gpgsignlist;

    bool operator==(const KeySubPrivate &other) const;
    bool operator!=(const KeySubPrivate &other) const;
};

class KeySub : public QObject
{
public:
    KeySub();
    KeySub(const KeySub &other);

    void setId(const QString &id);
    void setSize(const uint &size);
    void setUnlimited(const bool &unlimited);
    void setExpiration(const QDate &date);
    void setCreation(const QDate &date);
    void setTrust(const KeyTrust &trust);
    void setAlgorithme(const KeyAlgo &algo);
    void setValide(const bool &valide);

    QString id() const;
    uint size() const;
    bool unlimited() const;
    QDate expirationDate() const;
    QDate creationDate() const;
    KeyTrust trust() const;
    KeyAlgo algorithme() const;
    bool valide() const;

    QString creation() const;
    QString expiration() const;

    void addSign(const KeySign &sign);
    KeySignList signList();

    bool operator==(const KeySub &other) const;
    bool operator!=(const KeySub &other) const;
    KeySub& operator=(const KeySub &other);

private:
    QSharedDataPointer<KeySubPrivate> d;
};

class KeySubList : public QList<KeySub>, public QObject
{
public:
    inline KeySubList() { }
    inline explicit KeySubList(const KeySub &sub) { append(sub); }
    inline KeySubList(const KeySubList &other) : QList<KeySub>(other), QObject() { }
    inline KeySubList(const QList<KeySub> &other) : QList<KeySub>(other), QObject() { }

    inline KeySubList operator+(const KeySubList &other) const
    {
        KeySubList n = *this;
        n += other;
        return n;
    }

    inline KeySubList &operator<<(KeySub sub)
    {
        append(sub);
        return *this;
    }

    inline KeySubList &operator<<(const KeySubList &l)
    {
        *this += l;
        return *this;
    }
};
typedef QPointer<KeySubList> KeySubListPtr;

class KeyPrivate : public QSharedData
{
public:
    KeyPrivate();

    bool          gpgkeysecret;
    bool          gpgkeyvalide;
    QString       gpgkeyid;
    QString       gpgfullid;
    QString       gpgkeymail;
    QString       gpgkeyname;
    QString       gpgkeycomment;
    QString       gpgkeyfingerprint;
    QString       gpgkeysize;
    KeyOwnerTrust gpgkeyownertrust;
    KeyTrust      gpgkeytrust;
    QDate         gpgkeycreation;
    bool          gpgkeyunlimited;
    QDate         gpgkeyexpiration;
    KeyAlgo       gpgkeyalgo;

    KeySignList gpgsignlist;
    KeyUatListPtr gpguatlist;
    KeyUidListPtr gpguidlist;
    KeySubListPtr gpgsublist;

    bool operator==(const KeyPrivate &other) const;
    bool operator!=(const KeyPrivate &other) const;
};

class Key : public QObject
{
public:
    static QString expiration(const QDate &date, const bool &unlimited);

    Key();
    Key(const Key &other);

    void setSecret(const bool &secret);
    void setValide(const bool &valide);
    void setId(const QString &id);
    void setFullId(const QString &fullid);
    void setName(const QString &name);
    void setEmail(const QString &email);
    void setComment(const QString &comment);
    void setFingerprint(const QString &fingerprint);
    void setSize(const QString &size);
    void setOwnerTrust(const KeyOwnerTrust &owtrust);
    void setTrust(const KeyTrust &trust);
    void setCreation(const QDate &date);
    void setExpiration(const QDate &date);
    void setUnlimited(const bool &unlimited);
    void setAlgorithme(const KeyAlgo &algo);

    bool secret() const;
    bool valide() const;
    QString id() const;
    QString fullId() const;
    QString name() const;
    QString email() const;
    QString comment() const;
    QString fingerprint() const;
    QString size() const;
    KeyOwnerTrust ownerTrust() const;
    KeyTrust trust() const;
    QDate creationDate() const;
    QDate expirationDate() const;
    bool unlimited() const;
    KeyAlgo algorithme() const;

    QString creation() const;
    QString expiration() const;
    QStringList photoList() const;

    void addSign(const KeySign &sign);
    KeySignList signList();

    KeyUatListPtr uatList();
    KeyUidListPtr uidList();
    KeySubListPtr subList();

    bool operator==(const Key &other) const;
    bool operator!=(const Key &other) const;
    Key& operator=(const Key &other);

private:
    QSharedDataPointer<KeyPrivate> d;
};

class KeyList : public QList<Key>, public QObject
{
public:
    inline KeyList() { }
    inline explicit KeyList(const Key &key) { append(key); }
    inline KeyList(const KeyList &other) : QList<Key>(other), QObject() { }
    inline KeyList(const QList<Key> &other) : QList<Key>(other), QObject() { }

    inline KeyList& operator=(const KeyList &other)
    {
        QList<Key>::operator=(static_cast<const QList<Key> >(other));
        return *this;
    }

    inline KeyList operator+(const KeyList &other) const
    {
        KeyList n = *this;
        n += other;
        return n;
    }

    inline KeyList &operator<<(Key key)
    {
        append(key);
        return *this;
    }

    inline KeyList &operator<<(const KeyList &l)
    {
        *this += l;
        return *this;
    }
};
typedef QPointer<KeyList> KeyListPtr;

} // namespace

#endif // KGPGKEY_H
