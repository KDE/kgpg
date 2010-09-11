/*
 * Copyright (C) 2006,2007 Jimmy Gilles <jimmygilles@gmail.com>
 * Copyright (C) 2007,2008,2009,2010 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

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

//BEGIN KeyUid

bool KgpgKeyUidPrivate::operator==(const KgpgKeyUidPrivate &other) const
{
    if (gpguidvalid != other.gpguidvalid) return false;
    if (gpguidtrust != other.gpguidtrust) return false;
    if (gpguidname != other.gpguidname) return false;
    if (gpguidemail != other.gpguidemail) return false;
    if (gpguidcomment != other.gpguidcomment) return false;
    if (gpguidindex != other.gpguidindex) return false;
    if (gpgsignlist != other.gpgsignlist) return false;
    return true;
}

KgpgKeyUid::KgpgKeyUid()
    : d(new  KgpgKeyUidPrivate)
{
    d->gpguidvalid = false;
    d->gpguidindex = 0;
}

KgpgKeyUid::KgpgKeyUid(const KgpgKeyUid &other)
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

void KgpgKeyUid::setValid(const bool &valid)
{
    d->gpguidvalid = valid;
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

bool KgpgKeyUid::valid() const
{
    return d->gpguidvalid;
}

KgpgKeyTrust KgpgKeyUid::trust() const
{
    return d->gpguidtrust;
}

unsigned int KgpgKeyUid::index() const
{
    return d->gpguidindex;
}

void KgpgKeyUid::addSign(const QString &sign)
{
    d->gpgsignlist << sign;
}

QStringList KgpgKeyUid::signList()
{
	QStringList ret = d->gpgsignlist;
	d->gpgsignlist.clear();
	return ret;
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

//END KeyUid


//BEGIN KeySub

bool KgpgKeySubPrivate::operator==(const KgpgKeySubPrivate &other) const
{
    if (gpgsubvalid != other.gpgsubvalid) return false;
    if (gpgsubalgo != other.gpgsubalgo) return false;
    if (gpgsubid != other.gpgsubid) return false;
    if (gpgsubsize != other.gpgsubsize) return false;
    if (gpgsubexpiration != other.gpgsubexpiration) return false;
    if (gpgsubcreation != other.gpgsubcreation) return false;
    if (gpgsubtrust != other.gpgsubtrust) return false;
    if (gpgsignlist != other.gpgsignlist) return false;
    if (gpgsubtype != other.gpgsubtype) return false;
    return true;
}

KgpgKeySub::KgpgKeySub()
    : d(new  KgpgKeySubPrivate)
{
    d->gpgsubsize = 0;
    d->gpgsubvalid = false;
}

KgpgKeySub::KgpgKeySub(const KgpgKeySub &other)
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

void KgpgKeySub::setExpiration(const QDateTime &date)
{
    d->gpgsubexpiration = date;
}

void KgpgKeySub::setCreation(const QDateTime &date)
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

void KgpgKeySub::setValid(const bool &valid)
{
    d->gpgsubvalid = valid;
}

void KgpgKeySub::setType(const KgpgSubKeyType &type)
{
    d->gpgsubtype = type;
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
    return d->gpgsubexpiration.isNull();
}

QDateTime KgpgKeySub::expirationDate() const
{
    return d->gpgsubexpiration;
}

QDateTime KgpgKeySub::creationDate() const
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

bool KgpgKeySub::valid() const
{
    return d->gpgsubvalid;
}

KgpgSubKeyType KgpgKeySub::type() const
{
    return d->gpgsubtype;
}

void KgpgKeySub::addSign(const QString &sign)
{
    d->gpgsignlist << sign;
}

QStringList KgpgKeySub::signList()
{
	QStringList ret = d->gpgsignlist;
	d->gpgsignlist.clear();
	return ret;
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

//END KeySub


//BEGIN Key

KgpgKeyPrivate::KgpgKeyPrivate()
    : gpgkeysecret(false),
    gpgkeyvalid(false),
    gpgkeysize(0),
    gpgkeyalgo(ALGO_UNKNOWN),
    gpguidlist(new KgpgKeyUidList()),
    gpgsublist(new KgpgKeySubList())
{
}

bool KgpgKeyPrivate::operator==(const KgpgKeyPrivate &other) const
{
    if (gpgkeysecret != other.gpgkeysecret) return false;
    if (gpgkeyvalid != other.gpgkeyvalid) return false;
    if (gpgkeymail != other.gpgkeymail) return false;
    if (gpgkeyname != other.gpgkeyname) return false;
    if (gpgkeycomment != other.gpgkeycomment) return false;
    if (gpgkeyfingerprint != other.gpgkeyfingerprint) return false;
    if (gpgkeysize != other.gpgkeysize) return false;
    if (gpgkeyownertrust != other.gpgkeyownertrust) return false;
    if (gpgkeytrust != other.gpgkeytrust) return false;
    if (gpgkeycreation != other.gpgkeycreation) return false;
    if (gpgkeyexpiration != other.gpgkeyexpiration) return false;
    if (gpgkeyalgo != other.gpgkeyalgo) return false;
    if (gpgsignlist != other.gpgsignlist) return false;
    if (gpguidlist != other.gpguidlist) return false;
    if (gpgsublist != other.gpgsublist) return false;
    return true;
}

KgpgKey::KgpgKey()
    : d(new KgpgKeyPrivate())
{
}

KgpgKey::KgpgKey(const KgpgKey &other)
{
    d = other.d;
}

void KgpgKey::setSecret(const bool &secret)
{
    d->gpgkeysecret = secret;
}

void KgpgKey::setValid(const bool &valid)
{
    d->gpgkeyvalid = valid;
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

void KgpgKey::setSize(const uint &size)
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

void KgpgKey::setCreation(const QDateTime &date)
{
    d->gpgkeycreation = date;
}

void KgpgKey::setExpiration(const QDateTime &date)
{
    d->gpgkeyexpiration = date;
}

void KgpgKey::setAlgorithm(const KgpgKeyAlgo &algo)
{
    d->gpgkeyalgo = algo;
}

bool KgpgKey::secret() const
{
    return d->gpgkeysecret;
}

bool KgpgKey::valid() const
{
    return d->gpgkeyvalid;
}

QString KgpgKey::id() const
{
    return d->gpgkeyfingerprint.right(8);
}

QString KgpgKey::fullId() const
{
    return d->gpgkeyfingerprint.right(16);
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

QString KgpgKey::fingerprintBeautified() const
{
    QString fingervalue =d->gpgkeyfingerprint;
    uint len = fingervalue.length();
    if ((len > 0) && (len % 4 == 0))
      for (uint n = 0; 4 * (n + 1) < len; ++n)
        fingervalue.insert(5 * n + 4, ' ');
    return fingervalue;
}

uint KgpgKey::size() const
{
    return d->gpgkeysize;
}

uint KgpgKey::encryptionSize() const
{
	// Get the first encryption subkey
	for (int i = 0; i < d->gpgsublist->count(); ++i) {
		KgpgKeySub temp = d->gpgsublist->at(i);
		if (temp.type() & SKT_ENCRYPTION) {
			return temp.size();
		}
	}
	return 0;
}

KgpgKeyOwnerTrust KgpgKey::ownerTrust() const
{
    return d->gpgkeyownertrust;
}

KgpgKeyTrust KgpgKey::trust() const
{
    return d->gpgkeytrust;
}

QDateTime KgpgKey::creationDate() const
{
    return d->gpgkeycreation;
}

QDateTime KgpgKey::expirationDate() const
{
    return d->gpgkeyexpiration;
}

bool KgpgKey::unlimited() const
{
    return d->gpgkeyexpiration.isNull();
}

KgpgKeyAlgo KgpgKey::algorithm() const
{
    return d->gpgkeyalgo;
}

KgpgKeyAlgo KgpgKey::encryptionAlgorithm() const
{
	// Get the first encryption subkey
	for (int i = 0; i < d->gpgsublist->count(); ++i) {
		KgpgKeySub temp = d->gpgsublist->at(i);
		if (temp.type() & SKT_ENCRYPTION) {
			return temp.algorithm();
		}
	}
	return ALGO_UNKNOWN;
}

void KgpgKey::addSign(const QString &sign)
{
    d->gpgsignlist << sign;
}

QStringList KgpgKey::signList()
{
	QStringList ret = d->gpgsignlist;
	d->gpgsignlist.clear();
	return ret;
}

KgpgKeyUidListPtr KgpgKey::uidList() const
{
    return d->gpguidlist;
}

KgpgKeySubListPtr KgpgKey::subList() const
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

//END Key

} // namespace KgpgCore
