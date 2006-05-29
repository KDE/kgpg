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

class KgpgSelectPublicKeyDlg : public KDialog
{
    Q_OBJECT

public:
    KgpgSelectPublicKeyDlg(QWidget *parent = 0, const QString &sfile = "", const bool &filemode = false, const bool &enabledshred = true, const KShortcut &goDefaultKey = QKeySequence(Qt::CTRL + Qt::Key_Home));

    QStringList selectedKeys() const;
    bool getSymmetric() const;
    bool getUntrusted() const;
    bool getArmor() const;
    bool getHideId() const;
    bool getShred() const;

signals:
    void selectedKey(QStringList, QStringList, bool, bool);

private slots:
    void slotOk();
    void slotFillKeysList();
    void slotFillKeysListReady(KgpgListKeys keys, KgpgInterface *interface);
    void slotPreSelect();
    void slotSelectionChanged();
    void slotCustomOpts(const QString &str);
    void slotSymmetric(const bool &state);
    void slotUntrusted(const bool &state);
    void slotShowAllKeys();
    void slotHideUntrustedKeys();
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
    QString m_seclist; // list of IDs of secret keys
    QStringList m_untrustedlist; // list of keys that are untrusted

    K3ListView *m_keyslist;

    bool m_fmode;
};

#endif // KGPGSELECTPUBLICKEYDLG_H
