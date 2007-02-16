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

#include <kdialog.h>

#include "kgpginterface.h"
#include "ui_searchres.h"

class Q3ListViewItem;

class KConfig;
class KProcIO;
class KProcess;

class keyServerWidget;

class searchRes : public QWidget, public Ui::searchRes
{
public:
  searchRes( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};



class KeyServer : public KDialog
{
    Q_OBJECT

public:
    KeyServer(QWidget *parent = 0, const bool &modal = false, const bool &autoClose = false);

signals:
    void importFinished(QString);

public slots:
    void slotImport();
    void slotAbortImport();
    void slotExport(const QString &keyId);
    void slotAbortExport();
    void slotSearch();
    void slotAbortSearch();
    void slotSetText(const QString &text);
    void slotTextChanged(const QString &text);
    void slotSetExportAttribute(const bool &state);
    void slotEnableProxyI(const bool &on);
    void slotEnableProxyE(const bool &on);

    void transferKeyID();
    void slotPreImport();
    void slotPreExport();

    void slotOk();
    void syncCombobox();
    void handleQuit();

private slots:
    void slotReadKeys(KgpgCore::KeyList list, KgpgInterface *interface);

    void slotImportRead(KProcIO *p);
    void slotImportResult(KProcess *p);
    void slotExportResult(KProcess *p);
    void slotSearchRead(KProcIO *p);
    void slotSearchResult(KProcess *p);

private:
    QDialog *m_importpop;
    QString m_readmessage;
    Q3ListViewItem *m_kitem;

    KDialog *m_dialogserver;
    KSimpleConfig *m_config;
    KProcIO *m_importproc;
    KProcIO *m_exportproc;
    KProcIO *m_searchproc;

    keyServerWidget *page;
    searchRes *m_listpop;

    int m_count;
    uint m_keynumbers;
    bool m_cycle;
    bool m_autoclosewindow;
};

#endif // KEYSERVERS_H
