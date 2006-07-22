/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <klocale.h>

#include "convert.h"
#include "kgpgkey.h"

namespace KgpgCore
{

bool KeySignPrivate::operator==(const KeySignPrivate &other) const
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

bool KeySignPrivate::operator!=(const KeySignPrivate &other) const
{
    if (gpgsignrevocation != other.gpgsignrevocation) return true;
    if (gpgsignunlimited != other.gpgsignunlimited) return true;
    if (gpgsignlocal != other.gpgsignlocal) return true;
    if (gpgsignid != other.gpgsignid) return true;
    if (gpgsignname != other.gpgsignname) return true;
    if (gpgsignemail != other.gpgsignemail) return true;
    if (gpgsigncomment != other.gpgsigncomment) return true;
    if (gpgsignexpiration != other.gpgsignexpiration) return true;
    if (gpgsigncreation != other.gpgsigncreation) return true;
    return false;
}

KeySign::KeySign()
{
    d = new KeySignPrivate;
    d->gpgsignunlimited = false;
    d->gpgsignrevocation = false;
    d->gpgsignlocal = false;
}

KeySign::KeySign(const KeySign &other)
       : QObject()
{
    d = other.d;
}

void KeySign::setId(const QString &id)
{
    d->gpgsignid = id;
}

void KeySign::setName(const QString &name)
{
    d->gpgsignname = name;
}

void KeySign::setEmail(const QString &email)
{
    d->gpgsignemail = email;
}

void KeySign::setComment(const QString &comment)
{
    d->gpgsigncomment = comment;
}

void KeySign::setUnlimited(const bool &unlimited)
{
    d->gpgsignunlimited = unlimited;
}

void KeySign::setExpiration(const QDate &date)
{
    d->gpgsignexpiration = date;
}

void KeySign::setCreation(const QDate &date)
{
    d->gpgsigncreation = date;
}

void KeySign::setLocal(const bool &local)
{
    d->gpgsignlocal = local;
}

void KeySign::setRevocation(const bool &revoc)
{
    d->gpgsignrevocation = revoc;
}

QString KeySign::id() const
{
    return d->gpgsignid;
}

QString KeySign::name() const
{
    return d->gpgsignname;
}

QString KeySign::email() const
{
    return d->gpgsignemail;
}

QString KeySign::comment() const
{
    return d->gpgsigncomment;
}

bool KeySign::unlimited() const
{
    return d->gpgsignunlimited;
}

QDate KeySign::expirationDate() const
{
    return d->gpgsignexpiration;
}

QDate KeySign::creationDate() const
{
    return d->gpgsigncreation;
}

bool KeySign::local() const
{
    return d->gpgsignlocal;
}

bool KeySign::revocation() const
{
    return d->gpgsignrevocation;
}

QString KeySign::expiration() const
{
    return Key::expiration(d->gpgsignexpiration, d->gpgsignunlimited);
}

QString KeySign::creation() const
{
    return Convert::toString(d->gpgsigncreation);
}

bool KeySign::operator==(const KeySign &other) const
{
    if (d == other.d) return true;
    if ((*d) == (*(other.d))) return true;
    return false;
}

bool KeySign::operator!=(const KeySign &other) const
{
    if (d == other.d) return false;
    if ((*d) == (*(other.d))) return false;
    return true;
}

KeySign& KeySign::operator=(const KeySign &other)
{
    d = other.d;
    return *this;
}

bool KeyUatPrivate::operator==(const KeyUatPrivate &other) const
{
    if (gpguatid != other.gpguatid) return false;
    if (gpgsignlist != other.gpgsignlist) return false;
    return true;
}

bool KeyUatPrivate::operator!=(const KeyUatPrivate &other) const
{
    if (gpguatid != other.gpguatid) return true;
    if (gpgsignlist != other.gpgsignlist) return true;
    return false;
}

KeyUat::KeyUat()
      : QObject()
{
    d = new KeyUatPrivate;
}

KeyUat::KeyUat(const KeyUat &other)
      : QObject()
{
    d = other.d;
}

void KeyUat::setId(const QString &id)
{
    d->gpguatid = id;
}

QString KeyUat::id() const
{
    return d->gpguatid;
}

void KeyUat::addSign(const KeySign &sign)
{
    d->gpgsignlist << sign;
}

KeySignList KeyUat::signList()
{
    return d->gpgsignlist;
}

bool KeyUat::operator==(const KeyUat &other) const
{
    if (d == other.d) return true;
    if ((*d) == (*(other.d))) return true;
    return false;
}

bool KeyUat::operator!=(const KeyUat &other) const
{
    if (d == other.d) return false;
    if ((*d) == (*(other.d))) return false;
    return true;
}

KeyUat& KeyUat::operator=(const KeyUat &other)
{
    d = other.d;
    return *this;
}

bool KeyUidPrivate::operator==(const KeyUidPrivate &other) const
{
    if (gpguidvalide != other.gpguidvalide) return false;
    if (gpguidtrust != other.gpguidtrust) return false;
    if (gpguidname != other.gpguidname) return false;
    if (gpguidemail != other.gpguidemail) return false;
    if (gpguidcomment != other.gpguidcomment) return false;
    if (gpgsignlist != other.gpgsignlist) return false;
    return true;
}

bool KeyUidPrivate::operator!=(const KeyUidPrivate &other) const
{
    if (gpguidvalide != other.gpguidvalide) return true;
    if (gpguidtrust != other.gpguidtrust) return true;
    if (gpguidname != other.gpguidname) return true;
    if (gpguidemail != other.gpguidemail) return true;
    if (gpguidcomment != other.gpguidcomment) return true;
    if (gpgsignlist != other.gpgsignlist) return true;
    return false;
}

KeyUid::KeyUid()
      : QObject()
{
    d = new KeyUidPrivate;
}

KeyUid::KeyUid(const KeyUid &other)
      : QObject()
{
    d = other.d;
}

void KeyUid::setName(const QString &name)
{
    d->gpguidname = name;
}

void KeyUid::setEmail(const QString &email)
{
    d->gpguidemail = email;
}

void KeyUid::setComment(const QString &comment)
{
    d->gpguidcomment = comment;
}

void KeyUid::setValide(const bool &valide)
{
    d->gpguidvalide = valide;
}

void KeyUid::setTrust(const KeyTrust &trust)
{
    d->gpguidtrust = trust;
}

QString KeyUid::name() const
{
    return d->gpguidname;
}

QString KeyUid::email() const
{
    return d->gpguidemail;
}

QString KeyUid::comment() const
{
    return d->gpguidcomment;
}

bool KeyUid::valide() const
{
    return d->gpguidvalide;
}

KeyTrust KeyUid::trust() const
{
    return d->gpguidtrust;
}

void KeyUid::addSign(const KeySign &sign)
{
    d->gpgsignlist << sign;
}

KeySignList KeyUid::signList()
{
    return d->gpgsignlist;
}

bool KeyUid::operator==(const KeyUid &other) const
{
    if (d == other.d) return true;
    if ((*d) == (*(other.d))) return true;
    return false;
}

bool KeyUid::operator!=(const KeyUid &other) const
{
    if (d == other.d) return false;
    if ((*d) == (*(other.d))) return false;
    return true;
}

KeyUid& KeyUid::operator=(const KeyUid &other)
{
    d = other.d;
    return *this;
}

bool KeySubPrivate::operator==(const KeySubPrivate &other) const
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

bool KeySubPrivate::operator!=(const KeySubPrivate &other) const
{
    if (gpgsubvalide != other.gpgsubvalide) return true;
    if (gpgsubunlimited != other.gpgsubunlimited) return true;
    if (gpgsubalgo != other.gpgsubalgo) return true;
    if (gpgsubid != other.gpgsubid) return true;
    if (gpgsubsize != other.gpgsubsize) return true;
    if (gpgsubexpiration != other.gpgsubexpiration) return true;
    if (gpgsubcreation != other.gpgsubcreation) return true;
    if (gpgsubtrust != other.gpgsubtrust) return true;
    if (gpgsignlist != other.gpgsignlist) return true;
    return false;
}

KeySub::KeySub()
      : QObject()
{
    d = new KeySubPrivate;
    d->gpgsubsize = 0;
}

KeySub::KeySub(const KeySub &other)
      : QObject()
{
    d = other.d;
}

void KeySub::setId(const QString &id)
{
    d->gpgsubid = id;
}

void KeySub::setSize(const uint &size)
{
    d->gpgsubsize = size;
}

void KeySub::setUnlimited(const bool &unlimited)
{
    d->gpgsubunlimited = unlimited;
}

void KeySub::setExpiration(const QDate &date)
{
    d->gpgsubexpiration = date;
}

void KeySub::setCreation(const QDate &date)
{
    d->gpgsubcreation = date;
}

void KeySub::setTrust(const KeyTrust &trust)
{
    d->gpgsubtrust = trust;
}

void KeySub::setAlgorithme(const KeyAlgo &algo)
{
    d->gpgsubalgo = algo;
}

void KeySub::setValide(const bool &valide)
{
    d->gpgsubvalide = valide;
}

QString KeySub::id() const
{
    return d->gpgsubid;
}

uint KeySub::size() const
{
    return d->gpgsubsize;
}

bool KeySub::unlimited() const
{
    return d->gpgsubunlimited;
}

QDate KeySub::expirationDate() const
{
    return d->gpgsubexpiration;
}

QDate KeySub::creationDate() const
{
    return d->gpgsubcreation;
}

KeyTrust KeySub::trust() const
{
    return d->gpgsubtrust;
}

KeyAlgo KeySub::algorithme() const
{
    return d->gpgsubalgo;
}

bool KeySub::valide() const
{
    return d->gpgsubvalide;
}

QString KeySub::creation() const
{
    return Convert::toString(d->gpgsubcreation);
}

QString KeySub::expiration() const
{
    return Key::expiration(d->gpgsubexpiration, d->gpgsubunlimited);
}

void KeySub::addSign(const KeySign &sign)
{
    d->gpgsignlist << sign;
}

KeySignList KeySub::signList()
{
    return d->gpgsignlist;
}

bool KeySub::operator==(const KeySub &other) const
{
    if (d == other.d) return true;
    if ((*d) == (*(other.d))) return true;
    return false;
}

bool KeySub::operator!=(const KeySub &other) const
{
    if (d == other.d) return false;
    if ((*d) == (*(other.d))) return false;
    return true;
}

KeySub& KeySub::operator=(const KeySub &other)
{
    d = other.d;
    return *this;
}

QString Key::expiration(const QDate &date, const bool &unlimited)
{
    if (unlimited)
        return i18n("Unlimited");
    return Convert::toString(date);
}

KeyPrivate::KeyPrivate()
{
    gpguatlist = new KeyUatList;
    gpguidlist = new KeyUidList;
    gpgsublist = new KeySubList;
}

bool KeyPrivate::operator==(const KeyPrivate &other) const
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

bool KeyPrivate::operator!=(const KeyPrivate &other) const
{
    if (gpgkeysecret != other.gpgkeysecret) return true;
    if (gpgkeyvalide != other.gpgkeyvalide) return true;
    if (gpgkeyid != other.gpgkeyid) return true;
    if (gpgfullid != other.gpgfullid) return true;
    if (gpgkeymail != other.gpgkeymail) return true;
    if (gpgkeyname != other.gpgkeyname) return true;
    if (gpgkeycomment != other.gpgkeycomment) return true;
    if (gpgkeyfingerprint != other.gpgkeyfingerprint) return true;
    if (gpgkeysize != other.gpgkeysize) return true;
    if (gpgkeyownertrust != other.gpgkeyownertrust) return true;
    if (gpgkeytrust != other.gpgkeytrust) return true;
    if (gpgkeycreation != other.gpgkeycreation) return true;
    if (gpgkeyunlimited != other.gpgkeyunlimited) return true;
    if (gpgkeyexpiration != other.gpgkeyexpiration) return true;
    if (gpgkeyalgo != other.gpgkeyalgo) return true;
    if (gpgsignlist != other.gpgsignlist) return true;
    if (gpguatlist != other.gpguatlist) return true;
    if (gpguidlist != other.gpguidlist) return true;
    if (gpgsublist != other.gpgsublist) return true;
    return false;
}

Key::Key()
   : QObject()
{
    d = new KeyPrivate;
    d->gpgkeyunlimited = false;
    d->gpgkeyalgo = ALGO_UNKNOWN;
    d->gpgkeyvalide = false;
}

Key::Key(const Key &other)
   : QObject()
{
    d = other.d;
}

void Key::setSecret(const bool &secret)
{
    d->gpgkeysecret = secret;
}

void Key::setValide(const bool &valide)
{
    d->gpgkeyvalide = valide;
}

void Key::setId(const QString &id)
{
    d->gpgkeyid = id;
}

void Key::setFullId(const QString &fullid)
{
    d->gpgfullid = fullid;
}

void Key::setName(const QString &name)
{
    d->gpgkeyname = name;
}

void Key::setEmail(const QString &email)
{
    d->gpgkeymail = email;
}

void Key::setComment(const QString &comment)
{
    d->gpgkeycomment = comment;
}

void Key::setFingerprint(const QString &fingerprint)
{
    d->gpgkeyfingerprint = fingerprint;
}

void Key::setSize(const QString &size)
{
    d->gpgkeysize = size;
}

void Key::setOwnerTrust(const KeyOwnerTrust &owtrust)
{
    d->gpgkeyownertrust = owtrust;
}

void Key::setTrust(const KeyTrust &trust)
{
    d->gpgkeytrust = trust;
}

void Key::setCreation(const QDate &date)
{
    d->gpgkeycreation = date;
}

void Key::setExpiration(const QDate &date)
{
    d->gpgkeyexpiration = date;
}

void Key::setUnlimited(const bool &unlimited)
{
    d->gpgkeyunlimited = unlimited;
}

void Key::setAlgorithme(const KeyAlgo &algo)
{
    d->gpgkeyalgo = algo;
}

bool Key::secret() const
{
    return d->gpgkeysecret;
}

bool Key::valide() const
{
    return d->gpgkeyvalide;
}

QString Key::id() const
{
    return d->gpgkeyid;
}

QString Key::fullId() const
{
    return d->gpgfullid;
}

QString Key::name() const
{
    return d->gpgkeyname;
}

QString Key::email() const
{
    return d->gpgkeymail;
}

QString Key::comment() const
{
    return d->gpgkeycomment;
}

QString Key::fingerprint() const
{
    return d->gpgkeyfingerprint;
}

QString Key::size() const
{
    return d->gpgkeysize;
}

KeyOwnerTrust Key::ownerTrust() const
{
    return d->gpgkeyownertrust;
}

KeyTrust Key::trust() const
{
    return d->gpgkeytrust;
}

QDate Key::creationDate() const
{
    return d->gpgkeycreation;
}

QDate Key::expirationDate() const
{
    return d->gpgkeyexpiration;
}

bool Key::unlimited() const
{
    return d->gpgkeyunlimited;
}

KeyAlgo Key::algorithme() const
{
    return d->gpgkeyalgo;
}

QString Key::creation() const
{
    return Convert::toString(d->gpgkeycreation);
}

QString Key::expiration() const
{
    return expiration(d->gpgkeyexpiration, d->gpgkeyunlimited);
}

QStringList Key::photoList() const
{
    QStringList result;

    for (int i = 0; i < d->gpguatlist->size(); ++i)
        result << d->gpguatlist->at(i).id();

    return result;
}

void Key::addSign(const KeySign &sign)
{
    d->gpgsignlist << sign;
}

KeySignList Key::signList()
{
    return d->gpgsignlist;
}

KeyUatListPtr Key::uatList()
{
    return d->gpguatlist;
}

KeyUidListPtr Key::uidList()
{
    return d->gpguidlist;
}

KeySubListPtr Key::subList()
{
    return d->gpgsublist;
}

bool Key::operator==(const Key &other) const
{
    if (d == other.d) return true;
    if ((*d) == (*(other.d))) return true;
    return false;
}

bool Key::operator!=(const Key &other) const
{
    if (d == other.d) return false;
    if ((*d) == (*(other.d))) return false;
    return true;
}

Key& Key::operator=(const Key &other)
{
    d = other.d;
    return *this;
}

} // namespace KgpgCore
