/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010,2012,2013 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KEYSERVERS_H
#define KEYSERVERS_H

#include <QSortFilterProxyModel>

#include <KDialog>

#include "core/kgpgkey.h"
#include "ui_searchres.h"
#include "ui_keyserver.h"

class KGpgKeyserverSearchTransaction;
class KeyListProxyModel;
class KGpgItemModel;
class KGpgSearchResultModel;

class keyServerWidget : public QWidget, public Ui::keyServerWidget
{
public:
    explicit keyServerWidget(QWidget *parent = 0)
      : QWidget(parent)
    {
        setupUi(this);
    }
};

class searchRes : public QWidget, public Ui::searchRes
{
public:
    explicit searchRes(QWidget *parent)
      : QWidget(parent)
    {
        setupUi(this);
    }
};

class KeyServer : public KDialog
{
	Q_OBJECT

public:
	explicit KeyServer(QWidget *parent = 0, KGpgItemModel *model = 0, const bool autoclose = false);
	~KeyServer();

	/**
	 * Returns the server list.
	 * The first item is the server configured in gpg.
	 */
	static QStringList getServerList();

	/**
	 * @brief import the given keys
	 * @param keys the key fingerprints or ids to import
	 * @param server the key server to use, if empty the default server is used
	 * @param proxy the proxy to use
	 *
	 * This will set up the underlying transaction to fetch the keys as well
	 * as it will take care of setting up everything so the user will get the
	 * results shown.
	 */
	void startImport(const QStringList &keys, QString server = QString(), const QString &proxy = QString());

signals:
	void importFinished(QStringList);
	/**
	 * @brief importing failed
	 *
	 * This is emitted when key importing is finished and importFinished() is
	 * _not_ emitted. The main usage is to properly clean up the object when
	 * using startImport().
	 */
	void importFailed();

public slots:
	void slotImport();

	void slotExport(const QStringList &keyIds);

	void slotSetText(const QString &text);
	void slotTextChanged(const QString &text);
	void slotSetExportAttribute(const QString &attr);
	void slotEnableProxyI(const bool on);
	void slotEnableProxyE(const bool on);
	void slotSetKeyserver(const QString &server);

	void transferKeyID();
	void slotPreImport();
	void slotPreExport();

	void slotOk();
	void handleQuit();

private slots:
	void slotDownloadKeysFinished(int resultcode);
	void slotUploadKeysFinished(int resultcode);

	void slotSearchResult(int result);
	void slotSearch();
	void slotSetFilterString(const QString &expression);

private:
	QString m_readmessage;

	KDialog *m_dialogserver;
	KGpgKeyserverSearchTransaction *m_searchproc;

	keyServerWidget *page;
	searchRes *m_listpop;

	bool m_autoclose;
	QString expattr;

	KGpgSearchResultModel *m_resultmodel;
	QSortFilterProxyModel m_filtermodel;

	KeyListProxyModel *m_itemmodel;
};

#endif // KEYSERVERS_H
