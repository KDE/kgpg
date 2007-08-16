/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgkey.h"

#include <KLocale>

#include "convert.h"


namespace KgpgCore
{

bool KgpgKeySignPrivate::operator==(const KgpgKeySignPrivate &other) const
{
    if (gpgsignrevocation != other.gpgsignrevocation) return false;
    if (gpgsignunlimited != other.gpgsignunlimited) return false;
    if (gpgsignlocal != other.gpgsignlocal) return false;
    if (gpgsignid != other.gpgsignid) return false;
    if (gpgsignname != other.gpgsignname) return false;
    if (gpgsignemail != other.gpgsignemail) return false;
    if (gpgsigncomment != other.gpgsigncomment) return false;
    if (gpgsignexpiration != other.gpgsignexpiration) return false;
    if (gpgsigncreation != other.gpgsigncreation) return false;
    return true;
}

KgpgKeySign::KgpgKeySign()
{
    d = new KgpgKeySignPrivate;
    d->gpgsignunlimited = false;
    d->gpgsignrevocation = false;
    d->gpgsignlocal = false;
}

KgpgKeySign::KgpgKeySign(const KgpgKeySign &other)
       : QObject()
{
    d = other.d;
}

void KgpgKeySign::setId(const QString &id)
{
    d->gpgsignid = id;
}

void KgpgKeySign::setName(const QString &name)
{
    d->gpgsignname = name;
}

void KgpgKeySign::setEmail(const QString &email)
{
    d->gpgsignemail = email;
}

void KgpgKeySign::setComment(const QString &comment)
{
    d->gpgsigncomment = comment;
}

void KgpgKeySign::setUnlimited(const bool &unlimited)
{
    d->gpgsignunlimited = unlimited;
}

void KgpgKeySign::setExpiration(const QDate &date)
{
    d->gpgsignexpiration = date;
}

void KgpgKeySign::setCreation(const QDate &date)
{
    d->gpgsigncreation = date;
}

void KgpgKeySign::setLocal(const bool &local)
{
    d->gpgsignlocal = local;
}

void KgpgKeySign::setRevocation(const bool &revoc)
{
    d->gpgsignrevocation = revoc;
}

QString KgpgKeySign::id() const
{
    return d->gpgsignid;
}

QString KgpgKeySign::name() const
{
    return d->gpgsignname;
}

QString KgpgKeySign::email() const
{
    return d->gpgsignemail;
}

QString KgpgKeySign::comment() const
{
    return d->gpgsigncomment;
}

bool KgpgKeySign::unlimited() const
{
    return d->gpgsignunlimited;
}

QDate KgpgKeySign::expirationDate() const
{
    return d->gpgsignexpiration;
}

QDate KgpgKeySign::creationDate() const
{
    return d->gpgsigncreation;
}

bool KgpgKeySign::local() const
{
    return d->gpgsignlocal;
}

bool KgpgKeySign::revocation() const
{
    return d->gpgsignrevocation;
}

QString KgpgKeySign::expiration() const
{
    return KgpgKey::expiration(d->gpgsignexpiration, d->gpgsignunlimited);
}

QString KgpgKeySign::creation() const
{
    return Convert::toString(d->gpgsigncreation);
}

bool KgpgKeySign::operator==(const KgpgKeySign &other) const
{
    if (d == other.d) return true;
    if ((*d) == (*(other.d))) return true;
    return false;
}

KgpgKeySign& KgpgKeySign::operator=(const KgpgKeySign &other)
{
    d = other.d;
    return *this;
}

bool KgpgKeyUatPrivate::operator==(const KgpgKeyUatPrivate &other) const
{
    if (gpguatid != other.gpguatid) return false;
    if (gpgsignlist != other.gpgsignlist) return false;
    return true;
}

KgpgKeyUat::KgpgKeyUat()
      : QObject()
{
    d = new KgpgKeyUatPrivate;
}

KgpgKeyUat::KgpgKeyUat(const KgpgKeyUat &other)
      : QObject()
{
    d = other.d;
}

void KgpgKeyUat::setId(const QString &id)
{
    d->gpguatid = id;
}

QString KgpgKeyUat::id() const
{
    return d->gpguatid;
}

void KgpgKeyUat::addSign(const KgpgKeySign &sign)
{
    d->gpgsignlist << sign;
}

KgpgKeySignList KgpgKeyUat::signList()
{
    return d->gpgsignlist;
}

bool KgpgKeyUat::operator==(const KgpgKeyUat &other) const
{
    if (d == other.d) return true;
    if ((*d) == (*(other.d))) return true;
    return false;
}

KgpgKeyUat& KgpgKeyUat::operator=(const KgpgKeyUat &other)
{
    d = other.d;
    return *this;
}

bool KgpgKeyUidPrivate::operator==(const KgpgKeyUidPrivate &other) const
{
    if (gpguidvalide != other.gpguidvalide) return false;
    if (gpguidtrust != other.gpguidtrust) return false;
    if (gpguidname != other.gpguidname) return false;
    if (gpguidemail != other.gpguidemail) return false;
    if (gpguidcomment != other.gpguidcomment) return false;
    if (gpguidindex != other.gpguidindex) return false;
    if (gpgsignlist != other.gpgsignlist) return false;
    return true;
}

KgpgKeyUid::KgpgKeyUid()
          : QObject()
{
    d = new KgpgKeyUidPrivate;
}

KgpgKeyUid::KgpgKeyUid(const KgpgKeyUid &other)
          : QObject()
{
    d = other.d;
}

void KgpgKeyUid::setName(const QString &name)
{
    d->gpguidname = name;
}

void KgpgKeyUid::setEmail(const QString &email)
{
    d->gpguidemail = email;
}

void KgpgKeyUid::setComment(const QString &comment)
{
    d->gpguidcomment = comment;
}

void KgpgKeyUid::setValide(const bool &valide)
{
    d->gpguidvalide = valide;
}

void KgpgKeyUid::setTrust(const KgpgKeyTrust &trust)
{
    d->gpguidtrust = trust;
}

void KgpgKeyUid::setIndex(const unsigned int &index)
{
    d->gpguidindex = index;
}

QString KgpgKeyUid::name() const
{
    return d->gpguidname;
}

QString KgpgKeyUid::email() const
{
    return d->gpguidemail;
}

QString KgpgKeyUid::comment() const
{
    return d->gpguidcomment;
}

bool KgpgKeyUid::valide() const
{
    return d->gpguidvalide;
}

KgpgKeyTrust KgpgKeyUid::trust() const
{
    return d->gpguidtrust;
}

unsigned int KgpgKeyUid::index() const
{
    return d->gpguidindex;
}

void KgpgKeyUid::addSign(const KgpgKeySign &sign)
{
    d->gpgsignlist << sign;
}

KgpgKeySignList KgpgKeyUid::signList()
{
    return d->gpgsignlist;
}

bool KgpgKeyUid::operator==(const KgpgKeyUid &other) const
{
    if (d == other.d) return true;
    if ((*d) == (*(other.d))) return true;
    return false;
}

KgpgKeyUid& KgpgKeyUid::operator=(const KgpgKeyUid &other)
{
    d = other.d;
    return *this;
}

bool KgpgKeySubPrivate::operator==(const KgpgKeySubPrivate &other) const
{
    if (gpgsubvalide != other.gpgsubvalide) return false;
    if (gpgsubunlimited != other.gpgsubunlimited) return false;
    if (gpgsubalgo != other.gpgsubalgo) return false;
    if (gpgsubid != other.gpgsubid) return false;
    if (gpgsubsize != other.gpgsubsize) return false;
    if (gpgsubexpiration != other.gpgsubexpiration) return false;
    if (gpgsubcreation != other.gpgsubcreation) return false;
    if (gpgsubtrust != other.gpgsubtrust) return false;
    if (gpgsignlist != other.gpgsignlist) return false;
    return true;
}

KgpgKeySub::KgpgKeySub()
          : QObject()
{
    d = new KgpgKeySubPrivate;
    d->gpgsubsize = 0;
}

KgpgKeySub::KgpgKeySub(const KgpgKeySub &other)
          : QObject()
{
    d = other.d;
}

void KgpgKeySub::setId(const QString &id)
{
    d->gpgsubid = id;
}

void KgpgKeySub::setSize(const uint &size)
{
    d->gpgsubsize = size;
}

void KgpgKeySub::setUnlimited(const bool &unlimited)
{
    d->gpgsubunlimited = unlimited;
}

void KgpgKeySub::setExpiration(const QDate &date)
{
    d->gpgsubexpiration = date;
}

void KgpgKeySub::setCreation(const QDate &date)
{
    d->gpgsubcreation = date;
}

void KgpgKeySub::setTrust(const KgpgKeyTrust &trust)
{
    d->gpgsubtrust = trust;
}

void KgpgKeySub::setAlgorithm(const KgpgKeyAlgo &algo)
{
    d->gpgsubalgo = algo;
}

void KgpgKeySub::setValide(const bool &valide)
{
    d->gpgsubvalide = valide;
}

QString KgpgKeySub::id() const
{
    return d->gpgsubid;
}

uint KgpgKeySub::size() const
{
    return d->gpgsubsize;
}

bool KgpgKeySub::unlimited() const
{
    return d->gpgsubunlimited;
}

QDate KgpgKeySub::expirationDate() const
{
    return d->gpgsubexpiration;
}

QDate KgpgKeySub::creationDate() const
{
    return d->gpgsubcreation;
}

KgpgKeyTrust KgpgKeySub::trust() const
{
    return d->gpgsubtrust;
}

KgpgKeyAlgo KgpgKeySub::algorithm() const
{
    return d->gpgsubalgo;
}

bool KgpgKeySub::valide() const
{
    return d->gpgsubvalide;
}

QString KgpgKeySub::creation() const
{
    return Convert::toString(d->gpgsubcreation);
}

QString KgpgKeySub::expiration() const
{
    return KgpgKey::expiration(d->gpgsubexpiration, d->gpgsubunlimited);
}

void KgpgKeySub::addSign(const KgpgKeySign &sign)
{
    d->gpgsignlist << sign;
}

KgpgKeySignList KgpgKeySub::signList()
{
    return d->gpgsignlist;
}

bool KgpgKeySub::operator==(const KgpgKeySub &other) const
{
    if (d == other.d) return true;
    if ((*d) == (*(other.d))) return true;
    return false;
}

KgpgKeySub& KgpgKeySub::operator=(const KgpgKeySub &other)
{
    d = other.d;
    return *this;
}

QString KgpgKey::expiration(const QDate &date, const bool &unlimited)
{
    if (unlimited)
        return i18n("Unlimited");
    return Convert::toString(date);
}

KgpgKeyPrivate::KgpgKeyPrivate()
{
    gpguatlist = new KgpgKeyUatList;
    gpguidlist = new KgpgKeyUidList;
    gpgsublist = new KgpgKeySubList;
}

bool KgpgKeyPrivate::operator==(const KgpgKeyPrivate &other) const
{
    if (gpgkeysecret != other.gpgkeysecret) return false;
    if (gpgkeyvalide != other.gpgkeyvalide) return false;
    if (gpgkeyid != other.gpgkeyid) return false;
    if (gpgfullid != other.gpgfullid) return false;
    if (gpgkeymail != other.gpgkeymail) return false;
    if (gpgkeyname != other.gpgkeyname) return false;
    if (gpgkeycomment != other.gpgkeycomment) return false;
    if (gpgkeyfingerprint != other.gpgkeyfingerprint) return false;
    if (gpgkeysize != other.gpgkeysize) return false;
    if (gpgkeyownertrust != other.gpgkeyownertrust) return false;
    if (gpgkeytrust != other.gpgkeytrust) return false;
    if (gpgkeycreation != other.gpgkeycreation) return false;
    if (gpgkeyunlimited != other.gpgkeyunlimited) return false;
    if (gpgkeyexpiration != other.gpgkeyexpiration) return false;
    if (gpgkeyalgo != other.gpgkeyalgo) return false;
    if (gpgsignlist != other.gpgsignlist) return false;
    if (gpguatlist != other.gpguatlist) return false;
    if (gpguidlist != other.gpguidlist) return false;
    if (gpgsublist != other.gpgsublist) return false;
    return true;
}

KgpgKey::KgpgKey()
       : QObject()
{
    d = new KgpgKeyPrivate;
    d->gpgkeyunlimited = false;
    d->gpgkeyalgo = ALGO_UNKNOWN;
    d->gpgkeyvalide = false;
    d->gpgkeysecret = false;
}

KgpgKey::KgpgKey(const KgpgKey &other)
       : QObject()
{
    d = other.d;
}

void KgpgKey::setSecret(const bool &secret)
{
    d->gpgkeysecret = secret;
}

void KgpgKey::setValide(const bool &valide)
{
    d->gpgkeyvalide = valide;
}

void KgpgKey::setId(const QString &id)
{
    d->gpgkeyid = id;
}

void KgpgKey::setFullId(const QString &fullid)
{
    d->gpgfullid = fullid;
}

void KgpgKey::setName(const QString &name)
{
    d->gpgkeyname = name;
}

void KgpgKey::setEmail(const QString &email)
{
    d->gpgkeymail = email;
}

void KgpgKey::setComment(const QString &comment)
{
    d->gpgkeycomment = comment;
}

void KgpgKey::setFingerprint(const QString &fingerprint)
{
    d->gpgkeyfingerprint = fingerprint;
}

void KgpgKey::setSize(const QString &size)
{
    d->gpgkeysize = size;
}

void KgpgKey::setOwnerTrust(const KgpgKeyOwnerTrust &owtrust)
{
    d->gpgkeyownertrust = owtrust;
}

void KgpgKey::setTrust(const KgpgKeyTrust &trust)
{
    d->gpgkeytrust = trust;
}

void KgpgKey::setCreation(const QDate &date)
{
    d->gpgkeycreation = date;
}

void KgpgKey::setExpiration(const QDate &date)
{
    d->gpgkeyexpiration = date;
}

void KgpgKey::setUnlimited(const bool &unlimited)
{
    d->gpgkeyunlimited = unlimited;
}

void KgpgKey::setAlgorithm(const KgpgKeyAlgo &algo)
{
    d->gpgkeyalgo = algo;
}

bool KgpgKey::secret() const
{
    return d->gpgkeysecret;
}

bool KgpgKey::valide() const
{
    return d->gpgkeyvalide;
}

QString KgpgKey::id() const
{
    return d->gpgkeyid;
}

QString KgpgKey::fullId() const
{
    return d->gpgfullid;
}

QString KgpgKey::name() const
{
    return d->gpgkeyname;
}

QString KgpgKey::email() const
{
    return d->gpgkeymail;
}

QString KgpgKey::comment() const
{
    return d->gpgkeycomment;
}

QString KgpgKey::fingerprint() const
{
    return d->gpgkeyfingerprint;
}

QString KgpgKey::size() const
{
    return d->gpgkeysize;
}

KgpgKeyOwnerTrust KgpgKey::ownerTrust() const
{
    return d->gpgkeyownertrust;
}

KgpgKeyTrust KgpgKey::trust() const
{
    return d->gpgkeytrust;
}

QDate KgpgKey::creationDate() const
{
    return d->gpgkeycreation;
}

QDate KgpgKey::expirationDate() const
{
    return d->gpgkeyexpiration;
}

bool KgpgKey::unlimited() const
{
    return d->gpgkeyunlimited;
}

KgpgKeyAlgo KgpgKey::algorithm() const
{
    return d->gpgkeyalgo;
}

QString KgpgKey::creation() const
{
    return Convert::toString(d->gpgkeycreation);
}

QString KgpgKey::expiration() const
{
    return expiration(d->gpgkeyexpiration, d->gpgkeyunlimited);
}

QStringList KgpgKey::photoList() const
{
    QStringList result;

    for (int i = 0; i < d->gpguatlist->size(); ++i)
        result << d->gpguatlist->at(i).id();

    return result;
}

void KgpgKey::addSign(const KgpgKeySign &sign)
{
    d->gpgsignlist << sign;
}

KgpgKeySignList KgpgKey::signList()
{
    return d->gpgsignlist;
}

KgpgKeyUatListPtr KgpgKey::uatList()
{
    return d->gpguatlist;
}

KgpgKeyUidListPtr KgpgKey::uidList()
{
    return d->gpguidlist;
}

KgpgKeySubListPtr KgpgKey::subList()
{
    return d->gpgsublist;
}

bool KgpgKey::operator==(const KgpgKey &other) const
{
    if (d == other.d) return true;
    if ((*d) == (*(other.d))) return true;
    return false;
}

KgpgKey& KgpgKey::operator=(const KgpgKey &other)
{
    d = other.d;
    return *this;
}

} // namespace KgpgCore
