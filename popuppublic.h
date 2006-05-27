/***************************************************************************
                          popuppublic.h  -  description
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

#ifndef KGPGSELECTPUBLICKEYDLG_H
#define KGPGSELECTPUBLICKEYDLG_H

#include <QKeySequence>
#include <QStringList>
#include <QString>
#include <QPixmap>

#include <kdialogbase.h>
#include <kshortcut.h>

#include "kgpgkey.h"

class QPushButton;
class QCheckBox;
class QWidget;

class K3ListView;
class KProcIO;

class KgpgInterface;

class KgpgSelectPublicKeyDlg : public KDialogBase
{
    Q_OBJECT

public:
    KgpgSelectPublicKeyDlg(QWidget *parent = 0, const char *name = 0, const QString &sfile = "", const bool &filemode = false, const KShortcut &goDefaultKey = QKeySequence(Qt::CTRL + Qt::Key_Home), const bool &enabledshred = true);

    QStringList selectedKeys() const;
    bool getSymmetric() const;
    bool getUntrusted() const;
    bool getArmor() const;
    bool getHideId() const;
    bool getShred() const;

signals:
    void selectedKey(QStringList, QStringList, bool, bool);
    void keyListFilled();

public slots:
    void slotAccept();
    void slotSetVisible();

protected slots:
    virtual void slotOk();

private slots:
    void symmetric(const bool &state);
    void selectionChanged();
    void customOpts(const QString &str);
    void refreshKeys();
    void refreshKeysReady(KgpgListKeys keys, KgpgInterface *interface);
    void slotPreSelect();
    void refresh(const bool &state);
    void sort();
    void enable();
    void slotGotoDefaultKey();

private:
    QPixmap m_keysingle;
    QPixmap m_keypair;
    QPixmap m_keygroup;

    QCheckBox *m_cbarmor;
    QCheckBox *m_cbuntrusted;
    QCheckBox *m_cbhideid;
    QCheckBox *m_cbshred;
    QCheckBox *m_cbsymmetric;

    QString m_customoptions;
    QString m_seclist; // list of IDs of secrets keys
    QStringList m_untrustedlist; // list of keys that are untrusted

    K3ListView *m_keyslist;

    bool m_fmode;
};

#endif // KGPGSELECTPUBLICKEYDLG_H
