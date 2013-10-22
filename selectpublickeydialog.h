/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010,2011,2012,2013
 *               Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SELECTPUBLICKEYDIALOG_H
#define SELECTPUBLICKEYDIALOG_H

#include "core/kgpgkey.h"

#include <KDialog>
#include <KShortcut>
#include <KUrl>
#include <KVBox>
#include <QKeySequence>

class QCheckBox;
class QTableView;

class KLineEdit;
class KHBox;

class SelectKeyProxyModel;
class KGpgItemModel;

/**
 * @brief shows a dialog to select a public key for encryption
 */
class KgpgSelectPublicKeyDlg : public KDialog
{
    Q_OBJECT

public:
    /* TODO: the goDefaultKey shortcut should be stored in a way it can be accessed from everywhere. So we don't have to pass it as an argument.
     */

    KgpgSelectPublicKeyDlg(QWidget *parent, KGpgItemModel *model, const KShortcut &goDefaultKey = KShortcut(QKeySequence(Qt::CTRL + Qt::Key_Home)), const bool hideasciioption = false, const KUrl::List &files = KUrl::List());

    QStringList selectedKeys() const;
    QString getCustomOptions() const;
    bool getSymmetric() const;
    bool getUntrusted() const;
    bool getHideId() const;
    bool getArmor() const;
    /**
     * @brief return the files passed in the constructor
     */
    const KUrl::List &getFiles() const;

    KVBox *optionsbox;

private slots:
    void slotOk();
    void slotSelectionChanged();
    void slotSymmetric(const bool state);
    void slotUntrusted(const bool state);
    void slotGotoDefaultKey();

private:
    QCheckBox *m_cbarmor;
    QCheckBox *m_cbuntrusted;
    QCheckBox *m_cbhideid;
    QCheckBox *m_cbsymmetric;

    KHBox *m_searchbar;
    KLineEdit *m_customoptions;
    QTableView *m_keyslist;
    KLineEdit *m_searchlineedit;
    SelectKeyProxyModel *iproxy;
    KGpgItemModel *imodel;
    const KUrl::List m_files;

    bool m_hideasciioption;
};

#endif // SELECTPUBLICKEYDIALOG_H
