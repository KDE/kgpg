/***************************************************************************
                          keyservers.h  -  description
                             -------------------
    begin                : Tue Nov 26 2002
    copyright          : (C) 2002 by Jean-Baptiste Mardelle
    email                : bj@altern.org
 ***************************************************************************/

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

class GPGProc;
class KeyListProxyModel;
class KgpgInterface;
class KGpgItemModel;
class KGpgSearchResultModel;

class keyServerWidget : public QWidget, public Ui::keyServerWidget
{
public:
    keyServerWidget(QWidget *parent = 0) : QWidget(parent)
    {
        setupUi(this);
    }
};

class searchRes : public QWidget, public Ui::searchRes
{
public:
    searchRes(QWidget *parent) : QWidget(parent)
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

signals:
	void importFinished(QStringList);

public slots:
	void slotImport();

	void slotExport(const QStringList &keyIds);

	void slotSetText(const QString &text);
	void slotTextChanged(const QString &text);
	void slotSetExportAttribute(const QString &attr);
	void slotEnableProxyI(const bool &on);
	void slotEnableProxyE(const bool &on);
	void slotSetKeyserver(const QString &server);

	void transferKeyID();
	void slotPreImport();
	void slotPreExport();

	void slotOk();
	void handleQuit();

private slots:
	void slotDownloadKeysFinished(int resultcode);
	void slotUploadKeysFinished(int resultcode);

	void slotSearchRead(GPGProc *p);
	void slotSearchResult(GPGProc *p);
	void slotSearch();
	void slotAbortSearch();
	void slotSetFilterString(const QString &expression);

private:
	QString m_readmessage;

	KDialog *m_dialogserver;
	GPGProc *m_searchproc;

	keyServerWidget *page;
	searchRes *m_listpop;

	int m_count;
	bool m_autoclose;
	QString expattr;

	KGpgSearchResultModel *m_resultmodel;
	QSortFilterProxyModel m_filtermodel;

	KeyListProxyModel *m_itemmodel;
};

#endif // KEYSERVERS_H
