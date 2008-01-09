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
#include <QStringList>
#include <QPointer>
#include <QObject>
#include <QList>
#include <QDate>

namespace KgpgCore
{

//BEGIN Enums

enum KgpgKeyAlgoFlag
{
    ALGO_UNKNOWN = 0,
    ALGO_RSA = 1,
    ALGO_DSA = 2,
    ALGO_ELGAMAL = 4,
    ALGO_DSA_ELGAMAL = ALGO_DSA | ALGO_ELGAMAL
};
Q_DECLARE_FLAGS(KgpgKeyAlgo, KgpgKeyAlgoFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(KgpgKeyAlgo)

enum KgpgKeyTrustFlag
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
    TRUST_ULTIMATE = 9,
    TRUST_NOKEY = 10    // item is not a key
};
Q_DECLARE_FLAGS(KgpgKeyTrust, KgpgKeyTrustFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(KgpgKeyTrust)

enum KgpgKeyOwnerTrustFlag
{
    OWTRUST_UNKNOWN = 0,
    OWTRUST_UNDEFINED = 1,
    OWTRUST_NONE = 2,
    OWTRUST_MARGINAL = 3,
    OWTRUST_FULL = 4,
    OWTRUST_ULTIMATE = 5
};
Q_DECLARE_FLAGS(KgpgKeyOwnerTrust, KgpgKeyOwnerTrustFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(KgpgKeyOwnerTrust)

enum KgpgSubKeyTypeFlag
{
    SKT_ENCRYPTION = 0,
    SKT_SIGNATURE = 1
};
Q_DECLARE_FLAGS(KgpgSubKeyType, KgpgSubKeyTypeFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(KgpgSubKeyType)

//END Enums


//BEGIN KeySign

class KgpgKeySignPrivate : public QSharedData
{
public:
    bool    gpgsignrevocation;
    QString gpgsignid;
    QString gpgsignname;
    QString gpgsignemail;
    QString gpgsigncomment;
    QDate   gpgsignexpiration;
    QDate   gpgsigncreation;
    bool    gpgsignlocal;

    bool operator==(const KgpgKeySignPrivate &other) const;
    inline bool operator!=(const KgpgKeySignPrivate &other) const
    { return !operator==(other); }
};

class KgpgKeySign : public QObject
{
    Q_OBJECT

public:
    KgpgKeySign();
    KgpgKeySign(const KgpgKeySign &other);

    void setId(const QString &id);
    void setName(const QString &name);
    void setEmail(const QString &email);
    void setComment(const QString &comment);
    void setExpiration(const QDate &date);
    void setCreation(const QDate &date);
    void setLocal(const bool &local);
    void setRevocation(const bool &revoc);

    QString id() const;
    QString fullId() const;
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
    inline bool operator!=(const KgpgKeySign &other) const
    { return !operator==(other); }
    KgpgKeySign& operator=(const KgpgKeySign &other);

private:
    QSharedDataPointer<KgpgKeySignPrivate> d;
};

class KgpgKeySignList : public QList<KgpgKeySign>, QObject
{
public:
    inline KgpgKeySignList() { }
    inline explicit KgpgKeySignList(const KgpgKeySign &sign) { append(sign); }
    inline KgpgKeySignList(const KgpgKeySignList &other) : QList<KgpgKeySign>(other), QObject() { }
    inline KgpgKeySignList(const QList<KgpgKeySign> &other) : QList<KgpgKeySign>(other), QObject() { }

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

//END KeySign


//BEGIN KeyUat

class KgpgKeyUatPrivate : public QSharedData
{
public:
    QString gpguatid;
    QDate   gpguatcreation;
    KgpgKeySignList gpgsignlist;

    bool operator==(const KgpgKeyUatPrivate &other) const;
    inline bool operator!=(const KgpgKeyUatPrivate &other) const
    { return !operator==(other); }
};

class KgpgKeyUat : public QObject
{
    Q_OBJECT

public:
    KgpgKeyUat();
    KgpgKeyUat(const KgpgKeyUat &other);

    void setId(const QString &id);
    void setCreation(const QDate &date);
    QString id() const;
    QDate creationDate() const;
    QString creation() const;

    void addSign(const KgpgKeySign &sign);
    KgpgKeySignList signList() const;

    bool operator==(const KgpgKeyUat &other) const;
    inline bool operator!=(const KgpgKeyUat &other) const
    { return !operator==(other); }
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

//END KeyUat


//BEGIN KeyUid

class KgpgKeyUidPrivate : public QSharedData
{
public:
    bool     gpguidvalid;
    unsigned int gpguidindex;
    KgpgKeyTrust gpguidtrust;
    QString  gpguidname;
    QString  gpguidemail;
    QString  gpguidcomment;
    KgpgKeySignList gpgsignlist;

    bool operator==(const KgpgKeyUidPrivate &other) const;
    inline bool operator!=(const KgpgKeyUidPrivate &other) const
    { return !operator==(other); }
};

class KgpgKeyUid : public QObject
{
    Q_OBJECT

public:
    KgpgKeyUid();
    KgpgKeyUid(const KgpgKeyUid &other);

    void setName(const QString &name);
    void setEmail(const QString &email);
    void setComment(const QString &comment);
    void setValid(const bool &valid);
    void setTrust(const KgpgKeyTrust &trust);
    void setIndex(const unsigned int &index);

    QString name() const;
    QString email() const;
    QString comment() const;
    bool valid() const;
    KgpgKeyTrust trust() const;
    unsigned int index() const;

    void addSign(const KgpgKeySign &sign);
    KgpgKeySignList signList() const;

    bool operator==(const KgpgKeyUid &other) const;
    inline bool operator!=(const KgpgKeyUid &other) const
    { return !operator==(other); }
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

//END KeyUid


//BEGIN KeySub

class KgpgKeySubPrivate : public QSharedData
{
public:
    bool            gpgsubvalid;
    QString         gpgsubid;
    uint            gpgsubsize;
    QDate           gpgsubexpiration;
    QDate           gpgsubcreation;
    KgpgKeyTrust    gpgsubtrust;
    KgpgKeyAlgo     gpgsubalgo;
    KgpgKeySignList gpgsignlist;
    KgpgSubKeyType  gpgsubtype;

    bool operator==(const KgpgKeySubPrivate &other) const;
    inline bool operator!=(const KgpgKeySubPrivate &other) const
    { return !operator==(other); }
};

class KgpgKeySub : public QObject
{
    Q_OBJECT

public:
    KgpgKeySub();
    KgpgKeySub(const KgpgKeySub &other);

    void setId(const QString &id);
    void setSize(const uint &size);
    void setExpiration(const QDate &date);
    void setCreation(const QDate &date);
    void setTrust(const KgpgKeyTrust &trust);
    void setAlgorithm(const KgpgKeyAlgo &algo);
    void setValid(const bool &valid); // FIXME : is it possible to have a subkey that is not valid (disabled)? Please give an example. Thx. If not, this method should be removed.
    void setType(const KgpgSubKeyType &type); // a sub key can be a signature key or a encryption key

    QString id() const;
    uint size() const;
    bool unlimited() const;
    QDate expirationDate() const;
    QDate creationDate() const;
    KgpgKeyTrust trust() const;
    KgpgKeyAlgo algorithm() const;
    bool valid() const;
    KgpgSubKeyType type() const;

    QString creation() const;
    QString expiration() const;

    void addSign(const KgpgKeySign &sign);
    KgpgKeySignList signList() const;

    bool operator==(const KgpgKeySub &other) const;
    inline bool operator!=(const KgpgKeySub &other) const
    { return !operator==(other); }
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

//END KeySub


//BEGIN Key

class KgpgKeyPrivate : public QSharedData
{
public:
    KgpgKeyPrivate();

    bool          gpgkeysecret;
    bool          gpgkeyvalid;
    QString       gpgkeymail;
    QString       gpgkeyname;
    QString       gpgkeycomment;
    QString       gpgkeyfingerprint;
    uint          gpgkeysize;
    KgpgKeyOwnerTrust gpgkeyownertrust;
    KgpgKeyTrust  gpgkeytrust;
    QDate         gpgkeycreation;
    QDate         gpgkeyexpiration;
    KgpgKeyAlgo   gpgkeyalgo;

    KgpgKeySignList   gpgsignlist;
    KgpgKeyUatListPtr gpguatlist;
    KgpgKeyUidListPtr gpguidlist;
    KgpgKeySubListPtr gpgsublist;

    bool operator==(const KgpgKeyPrivate &other) const;
    inline bool operator!=(const KgpgKeyPrivate &other) const
    { return !operator==(other); }
};

class KgpgKey : public QObject
{
    Q_OBJECT

public:
    static QString expiration(const QDate &date);

    KgpgKey();
    KgpgKey(const KgpgKey &other);

    void setSecret(const bool &secret);
    void setValid(const bool &valid);
    void setName(const QString &name);
    void setEmail(const QString &email);
    void setComment(const QString &comment);
    void setFingerprint(const QString &fingerprint);
    void setSize(const uint &size);
    void setOwnerTrust(const KgpgKeyOwnerTrust &owtrust);
    void setTrust(const KgpgKeyTrust &trust);
    void setCreation(const QDate &date);
    void setExpiration(const QDate &date);
    void setAlgorithm(const KgpgKeyAlgo &algo);

    bool secret() const;
    bool valid() const;
    QString id() const;
    QString fullId() const;
    QString name() const;
    QString email() const;
    QString comment() const;
    QString fingerprint() const;
    QString fingerprintBeautified() const;
    uint size() const;
    KgpgKeyOwnerTrust ownerTrust() const;
    KgpgKeyTrust trust() const;
    QDate creationDate() const;
    QDate expirationDate() const;
    bool unlimited() const;
    KgpgKeyAlgo algorithm() const;

    QString creation() const;
    QString expiration() const;
    QStringList photoList() const;

    void addSign(const KgpgKeySign &sign);
    KgpgKeySignList signList() const;

    KgpgKeyUatListPtr uatList();
    KgpgKeyUidListPtr uidList();
    KgpgKeySubListPtr subList();

    bool operator==(const KgpgKey &other) const;
    inline bool operator!=(const KgpgKey &other) const
    { return !operator==(other); }
    KgpgKey& operator=(const KgpgKey &other);

private:
    QSharedDataPointer<KgpgKeyPrivate> d;
};

class KgpgKeyList : public QList<KgpgKey>, public QObject
{
public:
    inline KgpgKeyList() { }
    inline explicit KgpgKeyList(const KgpgKey &key) { append(key); }
    inline KgpgKeyList(const KgpgKeyList &other) : QList<KgpgKey>(other), QObject() { }
    inline KgpgKeyList(const QList<KgpgKey> &other) : QList<KgpgKey>(other), QObject() { }

    inline KgpgKeyList& operator=(const KgpgKeyList &other)
    {
        QList<KgpgKey>::operator=(static_cast<const QList<KgpgKey> >(other));
        return *this;
    }

    inline KgpgKeyList operator+(const KgpgKeyList &other) const
    {
        KgpgKeyList n = *this;
        n += other;
        return n;
    }

    inline KgpgKeyList &operator<<(KgpgKey key)
    {
        append(key);
        return *this;
    }

    inline KgpgKeyList &operator<<(const KgpgKeyList &l)
    {
        *this += l;
        return *this;
    }

    operator QStringList() const
    {
        QStringList res;
        foreach(KgpgKey key, *this)
            res << key.fullId();
        return res;
    }
};

//END Key

} // namespace

#endif // KGPGKEY_H
