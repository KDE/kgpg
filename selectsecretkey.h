/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __KGPGSELECTSECRETKEY_H__
#define __KGPGSELECTSECRETKEY_H__

#include <QString>

#include <kdialogbase.h>

class QCheckBox;

class Q3ListViewItem;

class KComboBox;
class KListView;

class KgpgSelectSecretKey : public KDialogBase
{
    Q_OBJECT

public:
    KgpgSelectSecretKey(QWidget *parent = 0, const char *name = 0, const bool &signkey = false, const int &countkey = 1);

    QString getKeyID() const;
    QString getKeyMail() const;
    int getSignTrust() const;
    bool isLocalSign() const;
    bool isTerminalSign() const;

private slots:
    void slotOk();
    void slotSelect(Q3ListViewItem *item);
    void selectionChanged();

private:
    QCheckBox *m_localsign;
    QCheckBox *m_terminalsign;

    KComboBox *m_signtrust;
    KListView *m_keyslistpr;

    bool m_signkey;
};

#endif // __KGPGSELECTSECRETKEY_H__
