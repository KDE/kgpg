/**
 * Copyright (C) 2009 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGEXPORT_H
#define KGPGEXPORT_H

#include <QObject>
#include <QStringList>

#include <KUrl>

#include "kgpgtransaction.h"

class QProcess;

/**
 * @brief export one or more keys from keyring
 *
 * The exported keys can be written to a file or sent to standard input of another
 * QProcess.
 */
class KGpgExport: public KGpgTransaction {
	Q_OBJECT

public:
	/**
	 * @brief export keys to QProcess
	 * @param parent parent object
	 * @param ids ids to export
	 * @param outd process to write into
	 * @param options additional options to pass to GnuPG (e.g. export ascii armored)
	 * @param secret if secret key exporting is allowed
	 */
	KGpgExport(QObject *parent, const QStringList &ids, QProcess *outp, const QStringList &options = QStringList(), const bool secret = false);

	/**
	 * @brief export keys to file
	 * @param parent parent object
	 * @param ids ids to export
	 * @param file filename to write into
	 * @param options additional options to pass to GnuPG (e.g. export ascii armored)
	 * @param secret if secret key exporting is allowed
	 */
	KGpgExport(QObject *parent, const QStringList &ids, const QString &file, const QStringList &options = QStringList(), const bool secret = false);

	/**
	 * @brief export keys to standard output
	 * @param parent parent object
	 * @param ids ids to export
	 * @param options additional options to pass to GnuPG (e.g. export ascii armored)
	 * @param secret if secret key exporting is allowed
	 *
	 * Only ascii-armored export is supported in standard output mode. If it is not
	 * already set in the given option it will be added automatically.
	 */
	KGpgExport(QObject *parent, const QStringList &ids, const QStringList &options = QStringList(), const bool secret = false);

	/**
	 * @brief destructor
	 */
	virtual ~KGpgExport();

	/**
	 * @brief set key id to export
	 * @param id key fingerprint
	 */
	void setKeyId(const QString &id);
	/**
	 * @brief set key ids to export
	 * @param ids key fingerprints
	 */
	void setKeyIds(const QStringList &ids);
	/**
	 * @brief return the key ids to export
	 * @return list of key fingerprints
	 */
	const QStringList &getKeyIds() const;
	/**
	 * @brief set the process the output is sent to
	 * @param outd process to send output to
	 */
	void setOutputProcess(QProcess *outp);
	/**
	 * @brief set filename to send output to
	 * @param outF file to send output to
	 */
	void setOutputFile(const QString &filename);
	/**
	 * @brief return the output filename currently set
	 * @return filename key will get written to
	 */
	const QString &getOutputFile() const;
	/**
	 * @brief return the data read from standard output
	 * @return standard output data
	 */
	const QByteArray &getOutputData() const;

protected:
	virtual bool preStart();
	virtual bool nextLine(const QString &line);

private:
	QStringList m_keyids;
	QProcess *m_outp;
	QString m_outf;
	QByteArray m_data;
	unsigned int m_outputmode;	// 0: file, 1: process, 2: stdout

	void procSetup(const QStringList &options, const bool secret);
};

#endif // KGPGEXPORT_H
