/***************************************************************************
                    selectpublickeydialog.h  -  description
                             -------------------
    begin                : Sat Jun 29 2002
    copyright            : (C) 2002 by Jean-Baptiste Mardelle
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

#ifndef SELECTPUBLICKEYDIALOG_H
#define SELECTPUBLICKEYDIALOG_H

#include <QKeySequence>

#include <KShortcut>
#include <KDialog>
#include <KVBox>

#include "kgpgkey.h"

class QCheckBox;

class K3ListViewSearchLine;
class K3ListView;
class KLineEdit;
class KHBox;

class KgpgInterface;

class KgpgSelectPublicKeyDlg : public KDialog
{
    Q_OBJECT

public:
    /* TODO: the goDefaultKey shortcut should be stored in a way it can be accessed from everywhere. So we don't have to pass it as an argument.
     */

    explicit KgpgSelectPublicKeyDlg(QWidget *parent = 0, const QString &sfile = "", const bool &filemode = false, const KShortcut &goDefaultKey = KShortcut(QKeySequence(Qt::CTRL + Qt::Key_Home)), const bool &hideasciioption = false);

    QStringList selectedKeys() const;
    QString getCustomOptions() const;
    bool getSymmetric() const;
    bool getUntrusted() const;
    bool getHideId() const;
    bool getArmor() const;

    KVBox *optionsbox;

private slots:
    void slotOk();
    void slotFillKeysList();
    void slotFillKeysListReady(KgpgCore::KgpgKeyList keys, KgpgInterface *interface);
    void slotPreSelect();
    void slotSelectionChanged();
    void slotSymmetric(const bool &state);
    void slotUntrusted(const bool &state);
    void slotShowAllKeys();
    void slotHideUntrustedKeys();
    void slotGotoDefaultKey();

private:
    QCheckBox *m_cbarmor;
    QCheckBox *m_cbuntrusted;
    QCheckBox *m_cbhideid;
    QCheckBox *m_cbsymmetric;

    KHBox *m_searchbar;
    KLineEdit *m_customoptions;
    K3ListView *m_keyslist;
    K3ListViewSearchLine *m_searchlineedit;

    bool m_hideasciioption;
    bool m_fmode;
};

#endif // SELECTPUBLICKEYDIALOG_H
