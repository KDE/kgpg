/*
 * Copyright (C) 2009,2009 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGIMPORT_H
#define KGPGIMPORT_H

#include <QObject>
#include <QList>
#include <QStringList>

#include <KUrl>

#include "kgpgtransaction.h"

/**
 * @brief import one or more keys into the keyring
 */
class KGpgImport: public KGpgTransaction {
	Q_OBJECT

public:
	/**
	 * @brief additional status codes for KGpgImport
	 */
	enum ts_import {
		TS_IMPORT_KIO_FAILED = TS_COMMON_END + 1	///< download of remote file failed
	};

	/**
	 * @brief import given text
	 * @param parent parent object
	 * @param text key text to import
	 */
	KGpgImport(QObject *parent, const QString &text);

	/**
	 * @brief import key(s) from file(s)
	 * @param parent parent object
	 * @param keys list of file locations to import from
	 */
	KGpgImport(QObject *parent, const KUrl::List &keys);

	/**
	 * @brief destructor
	 */
	virtual ~KGpgImport();

	/**
	 * @brief set text to import
	 * @param text key text to import
	 */
	void setText(const QString &text);
	/**
	 * @brief set file locations to import from
	 * @param keys list of file locations
	 */
	void setUrls(const KUrl::List &keys);

	/**
	 * @brief get import info message
	 * @return the raw messages from gpg during the import operation
	 */
	const QStringList &getMessages() const;

	/**
	 * @brief get the names and short fingerprints of the imported keys
	 * @return list of keys that were imported
	 */
	QStringList getImportedKeys() const;

	/**
	 * @brief get the full fingerprints of the imported keys
	 * @param log transaction log to scan
	 * @param reason key import reason
	 * @return list of ids that were imported
	 *
	 * You can filter the list of keys returned by the status of that key
	 * as reported by GnuPG. See doc/DETAILS of GnuPG for the meaning of
	 * the different flags.
	 *
	 * If reason is -1 (the default) all processed key ids are returned.
	 * If reason is 0 only keys of status 0 (unchanged) are returned. For
	 * any other value a key is returned if one of his status bits matched
	 * one of the bits in reason (i.e. (reason & status) != 0).
	 */
	static QStringList getImportedIds(const QStringList &log, const int reason = -1);
	/**
	 * @brief get the full fingerprints of the imported keys
	 *
	 * This is an overloaded member. It calls the static function with the
	 * result log from this transaction object.
	 */
	QStringList getImportedIds(const int reason = -1) const;

	/**
	 * @brief get textual summary of the import events
	 * @return messages describing what was imported
	 *
	 * This is an overloaded member. It calls the static function with the
	 * result log from this transaction object.
	 */
	QString getImportMessage() const;

	/**
	 * @brief get textual summary of the import events
	 * @param log import log
	 * @return messages describing what was imported
	 *
	 * The log must contain a "IMPORT_RES" line. If this is not present
	 * the result string will contain an error message.
	 */
	static QString getImportMessage(const QStringList &log);

	/**
	 * @brief get detailed summary of import
	 * @param log import log
	 * @return message describing which keys changed and how
	 *
	 * The log must contain a "IMPORT_RES" line. If this is not present
	 * the result string will contain an error message.
	 */
	static QString getDetailedImportMessage(const QStringList &log);

protected:
	virtual bool preStart();
	virtual bool nextLine(const QString &line);
	virtual void finish();

private:
	QStringList m_tempfiles;
	QStringList m_locfiles;
	KUrl::List m_inpfiles;
	QString m_text;
	QStringList m_messages;

	void cleanUrls();

private slots:
	void postStart();
};

#endif // KGPGIMPORT_H
