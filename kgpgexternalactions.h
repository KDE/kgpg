/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2008,2009,2010,2011 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KGPGEXTERNALACTIONS_H
#define _KGPGEXTERNALACTIONS_H

#include <QObject>
#include <QClipboard>
#include <QPointer>
#include <QStringList>

#include <KUrl>

class KeysManager;
class KGpgFirstAssistant;
class KGpgItemModel;
class KGpgTextInterface;
class KJob;
class KShortcut;
class KTemporaryFile;
class QFont;
class QString;

/**
 * @brief handle actions called from outside the application
 *
 * This class takes care about everything sent to us from outside the
 * application, e.g. command line arguments given on startup.
 */
class KGpgExternalActions : public QObject
{
	Q_OBJECT

public:
	KGpgExternalActions(KeysManager *parent, KGpgItemModel *model);
	~KGpgExternalActions();

	void showDroppedFile(const KUrl &file);
	void verifyFile(KUrl url);
	/**
	 * @brief create a detached signature for a chosen files
	 */
	void signDroppedFiles(const KUrl::List &urls);
	void decryptDroppedFiles(const KUrl::List &urls);
	void encryptDroppedFolders(const KUrl::List &urls);

signals:
	void createNewKey();
	void updateDefault(QString);

public slots:
	void slotEncryptDroppedFiles(const KUrl::List &urls);

	void startFolderEncode();
	void slotAbortEnc();

private:
	QStringList customDecrypt;
	QPointer<KGpgFirstAssistant> m_assistant;
	KTemporaryFile *kgpgFolderExtract;
	int compressionScheme;
	QClipboard::Mode clipboardMode;
	KGpgItemModel *m_model;
	KTemporaryFile *m_kgpgfoldertmp;

	void startAssistant();
	void firstRun();

	KUrl::List m_decryptionFailed;
	QWidget *m_parentWidget;
	KeysManager *m_keysmanager;
	KUrl::List droppedUrls;

	KShortcut goDefaultKey() const;
	void decryptFile(KUrl::List urls);

private slots:
	void slotSaveOptionsPath();
	void slotVerificationDone(int result);
	void help();
	void readOptions();
	void slotSetCompression(int cp);
	void slotDecryptionDone(int status);
	void slotFolderFinished(KJob *job);
	void slotSigningFinished();
	void slotSignFiles();
	void slotEncryptionKeySelected();
};

#endif /* _KGPGEXTERNALACTIONS_H */
