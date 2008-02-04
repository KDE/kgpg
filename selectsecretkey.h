/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGSELECTSECRETKEY_H
#define KGPGSELECTSECRETKEY_H

#include <KDialog>

class QCheckBox;
class QTableView;

class KComboBox;

class KGpgItemModel;
class SelectSecretKeyProxyModel;

class KgpgSelectSecretKey : public KDialog
{
    Q_OBJECT

public:
    /**
     * Constructor.
     * @param parent is the parent object
     * @param signkey set to \em true if you are going to sign a key, set
     * to \em false if you are going to sign a message. Default is \em false
     * @param countkey if \em signkey is set to \em true, \em countkey is the
     * number of keys that you are going to sign. Default is 1.
     * @param model pass a pointer to a KGpgItemModel here if you alredy have
     * one around. Default is NULL which means to create a new one.
     */
    explicit KgpgSelectSecretKey(QWidget *parent = 0, const bool &signkey = false, const int &countkey = 1, KGpgItemModel *model = NULL);
    ~KgpgSelectSecretKey();

    QString getKeyID() const;
    QString getKeyMail() const;
    int getSignTrust() const;

    /**
     * @return true if the constructor is called with \em signkey to true
     * and if the user checked \em localsign
     * @return false otherwise
     */
    bool isLocalSign() const;

    /**
     * @return true if the constructor is called with \em signkey to true
     * and if the user checked \em terminalsign
     * @return false otherwise
     */
    bool isTerminalSign() const;

private slots:
    void slotOk();
    void slotSelectionChanged();

private:
    QCheckBox *m_localsign;
    QCheckBox *m_terminalsign;

    KComboBox *m_signtrust;
    QTableView *m_keyslist;
    SelectSecretKeyProxyModel *m_proxy;
};

#endif // KGPGSELECTSECRETKEY_H
