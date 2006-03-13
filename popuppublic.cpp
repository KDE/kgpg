/***************************************************************************
                          popuppublic.cpp  -  description
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

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QCheckBox>
#include <QPainter>
#include <QLabel>

#include <Q3ListViewItem>

#include <k3listviewsearchline.h>
#include <kactivelabel.h>
#include <kiconloader.h>
#include <kdeversion.h>
#include <k3listview.h>
#include <klineedit.h>
#include <klocale.h>
#include <kaction.h>
#include <kdebug.h>
#include <khbox.h>
#include <kvbox.h>

#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "popuppublic.h"

class KeyViewItem : public Q3ListViewItem
{
public:
    KeyViewItem(K3ListView *parent, const QString &name, const QString &mail, const QString &id, const bool &isDefault);
    virtual void paintCell(QPainter *p, const QColorGroup &cg, const int &col, const int &width, const int &align);
    virtual QString key(const int &c) const;

private:
    bool def;
};

KeyViewItem::KeyViewItem(K3ListView *parent, const QString &name, const QString &mail, const QString &id, const bool &isDefault)
           : Q3ListViewItem(parent)
{
    def = isDefault;
    setText(0, name);
    setText(1, mail);
    setText(2, id);
}

void KeyViewItem::paintCell(QPainter *p, const QColorGroup &cg, const int &column, const int &width, const int &alignment)
{
    if ((def) && (column < 2))
    {
        QFont font(p->font());
        font.setBold(true);
        p->setFont(font);
    }

    Q3ListViewItem::paintCell(p, cg, column, width, alignment);
}

QString KeyViewItem::key(const int &c) const
{
    return text(c).toLower();
}

KgpgSelectPublicKeyDlg::KgpgSelectPublicKeyDlg(QWidget *parent, const char *name, const QString &sfile, const bool &filemode, const KShortcut &goDefaultKey, const bool &enabledshred)
                      : KDialogBase(Plain, i18n("Select Public Key"), Details | Ok | Cancel, Ok, parent, name, true)
{
    QWidget *page = plainPage();

    QVBoxLayout *vbox = new QVBoxLayout(page);
    vbox->setSpacing(spacingHint());
    vbox->setMargin(0);
    page->setLayout(vbox);

    setButtonText(KDialogBase::Details, i18n("O&ptions"));

    if (KGpgSettings::allowCustomEncryptionOptions())
        m_customoptions = KGpgSettings::customEncryptionOptions();

    KIconLoader *loader = KGlobal::iconLoader();
    m_keypair = loader->loadIcon("kgpg_key2", KIcon::Small, 20);
    m_keysingle = loader->loadIcon("kgpg_key1", KIcon::Small, 20);
    m_keygroup = loader->loadIcon("kgpg_key3", KIcon::Small, 20);

    if (filemode)
        setCaption(i18n("Select Public Key for %1").arg(sfile));
    m_fmode = filemode;

    KHBox *hBar = new KHBox(page);
    hBar->setFrameShape(QFrame::StyledPanel);
    vbox->addWidget(hBar);

    QToolButton *clearSearch = new QToolButton(hBar);
    clearSearch->setTextLabel(i18n("Clear Search"), true);
    clearSearch->setIconSet(SmallIconSet(QApplication::reverseLayout() ? "clear_left" : "locationbar_erase"));

    QLabel *searchlabel = new QLabel(i18n("&Search: "), hBar);
    K3ListViewSearchLine* listViewSearch = new K3ListViewSearchLine(hBar);
    searchlabel->setBuddy(listViewSearch);
    connect(clearSearch, SIGNAL(pressed()), listViewSearch, SLOT(clear()));

    m_keyslist = new K3ListView(page);
    m_keyslist->addColumn(i18n("Name"));
    m_keyslist->addColumn(i18n("Email"));
    m_keyslist->addColumn(i18n("ID"));
    vbox->addWidget(m_keyslist);

    listViewSearch->setListView(m_keyslist);

    m_keyslist->setRootIsDecorated(false);
    page->setMinimumSize(540,200);
    m_keyslist->setShowSortIndicator(true);
    m_keyslist->setFullWidth(true);
    m_keyslist->setAllColumnsShowFocus(true);
    m_keyslist->setSelectionModeExt(K3ListView::Extended);
    m_keyslist->setColumnWidthMode(0, K3ListView::Manual);
    m_keyslist->setColumnWidthMode(1, K3ListView::Manual);
    m_keyslist->setColumnWidth(0, 210);
    m_keyslist->setColumnWidth(1, 210);
    m_keyslist->setWhatsThis(i18n("<b>Public keys list</b>: select the key that will be used for encryption."));
    connect(m_keyslist, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));

    KVBox *boutonboxoptions = new KVBox(page);
    boutonboxoptions->setFrameShape(QFrame::StyledPanel);
    vbox->addWidget(boutonboxoptions);

    KActionCollection *actcol = new KActionCollection(this);
    (void) new KAction(i18n("&Go to Default Key"), goDefaultKey, this, SLOT(slotGotoDefaultKey()), actcol, "go_default_key");

    m_cbarmor = new QCheckBox(i18n("ASCII armored encryption"), boutonboxoptions);
    m_cbarmor->setWhatsThis(i18n("<b>ASCII encryption</b>: makes it possible to open the encrypted file/message in a text editor"));

    m_cbuntrusted = new QCheckBox(i18n("Allow encryption with untrusted keys"), boutonboxoptions);
    m_cbuntrusted->setWhatsThis(i18n("<b>Allow encryption with untrusted keys</b>: when you import a public key, it is usually "
                    "marked as untrusted and you cannot use it unless you sign it in order to make it 'trusted'. Checking this "
                    "box enables you to use any key, even if it has not be signed."));

    m_cbhideid = new QCheckBox(i18n("Hide user id"), boutonboxoptions);
    m_cbhideid->setWhatsThis(i18n("<b>Hide user ID</b>: Do not put the keyid into encrypted packets. This option hides the receiver "
                    "of the message and is a countermeasure against traffic analysis. It may slow down the decryption process because "
                    "all available secret keys are tried."));

    setDetailsWidget(boutonboxoptions);

    if (filemode)
    {
        QWidget *parentBox = new QWidget(boutonboxoptions);
        QHBoxLayout *shredBox = new QHBoxLayout(parentBox);
        shredBox->setMargin(0);

        m_cbshred = new QCheckBox(i18n("Shred source file"), parentBox);
        m_cbshred->setWhatsThis(i18n("<b>Shred source file</b>: permanently remove source file. No recovery will be possible"));

        QString shredWhatsThis = i18n("<qt><b>Shred source file:</b><br /><p>Checking this option will shred (overwrite several times before erasing) the files you have encrypted. This way, it is almost impossible that the source file is recovered.</p><p><b>But you must be aware that this is not secure</b> on all file systems, and that parts of the file may have been saved in a temporary file or in the spooler of your printer if you previously opened it in an editor or tried to print it. Only works on files (not on folders).</p></qt>");

        KActiveLabel *warn = new KActiveLabel(i18n("<a href=\"whalinehis:%1\">Read this before using shredding</a>").arg(shredWhatsThis), parentBox);
        shredBox->addWidget(m_cbshred);
        shredBox->addWidget(warn);
        m_cbshred->setEnabled(enabledshred);
    }

    m_cbsymmetric = new QCheckBox(i18n("Symmetrical encryption"), boutonboxoptions);
    m_cbsymmetric->setWhatsThis(i18n("<b>Symmetrical encryption</b>: encryption does not use keys. You just need to give a password "
                                     "to encrypt/decrypt the file"));
    connect(m_cbsymmetric, SIGNAL(toggled(bool)), this, SLOT(symmetric(bool)));

    m_cbarmor->setChecked(KGpgSettings::asciiArmor());
    m_cbuntrusted->setChecked(KGpgSettings::allowUntrustedKeys());
    m_cbhideid->setChecked(KGpgSettings::hideUserID());

    if (filemode)
        m_cbshred->setChecked(KGpgSettings::shredSource());

    if (KGpgSettings::allowCustomEncryptionOptions())
    {
        KHBox *bGroup = new KHBox(page);
        (void) new QLabel(i18n("Custom option:"), bGroup);

        KLineEdit *optiontxt = new KLineEdit(bGroup);
        optiontxt->setText(m_customoptions);
        optiontxt->setWhatsThis(i18n("<b>Custom option</b>: for experienced users only, allows you to enter a gpg command line option, like: '--armor'"));

        connect(optiontxt, SIGNAL(textChanged(const QString &)), this, SLOT(customOpts(const QString &)));
    }

    connect(m_keyslist, SIGNAL(doubleClicked(Q3ListViewItem *, const QPoint &, int)), this, SLOT(slotOk()));
    connect(m_cbuntrusted, SIGNAL(toggled(bool)), this, SLOT(refresh(bool)));

    QString line;
    m_seclist = QString::null;

    KgpgInterface *interface = new KgpgInterface();
    KgpgListKeys list = interface->readSecretKeys(true);
    delete interface;
    for (int i = 0; i < list.size(); ++i)
        m_seclist += ", 0x" + list.at(i).id();

    refreshKeys();
    //m_keyslist->setFocus();
    setMinimumSize(550, 200);
    updateGeometry();
    selectionChanged();
    show();
}

QStringList KgpgSelectPublicKeyDlg::selectedKeys() const
{
    QStringList selectedKeys;
    QList<Q3ListViewItem*> list = m_keyslist->selectedItems();
    for (int i = 0; i < list.count(); ++i)
        if (list.at(i) && list.at(i)->isVisible())
        {
            if (!list.at(i)->text(2).isEmpty())
                selectedKeys << list.at(i)->text(2);
            else
                selectedKeys << list.at(i)->text(0);
        }

    return selectedKeys;
}

bool KgpgSelectPublicKeyDlg::getSymmetric() const
{
    return m_cbsymmetric;
}

bool KgpgSelectPublicKeyDlg::getUntrusted() const
{
    return m_cbuntrusted;
}

bool KgpgSelectPublicKeyDlg::getArmor() const
{
    return m_cbarmor;
}

bool KgpgSelectPublicKeyDlg::getHideId() const
{
    return m_cbhideid;
}

bool KgpgSelectPublicKeyDlg::getShred() const
{
    return m_cbhideid;
}

void KgpgSelectPublicKeyDlg::slotAccept()
{
    accept();
}

void KgpgSelectPublicKeyDlg::slotSetVisible()
{
    m_keyslist->ensureItemVisible(m_keyslist->currentItem());
}

void KgpgSelectPublicKeyDlg::slotOk()
{
    kDebug(2100) << "Ok pressed" << endl;

    QStringList selectedkeys = selectedKeys();
    if (selectedkeys.isEmpty() && !m_cbsymmetric->isChecked())
        return;

    if (m_cbsymmetric->isChecked())
        selectedkeys = QStringList();

    kDebug(2100) << "Selected Key:" << selectedkeys << endl;

    QStringList returnOptions;
    if (m_cbuntrusted->isChecked())
        returnOptions << "--always-trust";
    if (m_cbarmor->isChecked())
        returnOptions << "--armor";
    if (m_cbhideid->isChecked())
        returnOptions << "--throw-keyid";

    if ((KGpgSettings::allowCustomEncryptionOptions()) && (!m_customoptions.simplified().isEmpty()))
        returnOptions << QStringList::split(QString(" "), m_customoptions.simplified());

    if (m_fmode)
        emit selectedKey(selectedkeys, returnOptions, m_cbshred->isChecked(), m_cbsymmetric->isChecked());
    else
        emit selectedKey(selectedkeys, returnOptions, false, m_cbsymmetric->isChecked());

    accept();
}

void KgpgSelectPublicKeyDlg::symmetric(const bool &state)
{
    m_keyslist->setEnabled(!state);
    m_cbuntrusted->setEnabled(!state);
    m_cbhideid->setEnabled(!state);
    selectionChanged();
}

void KgpgSelectPublicKeyDlg::selectionChanged()
{
    if (m_cbsymmetric->isChecked())
        enableButtonOK(true);
    else
    {
        if (!m_keyslist->selectedItems().isEmpty())
            enableButtonOK(true);
        else
            enableButtonOK(false);
    }
}

void KgpgSelectPublicKeyDlg::customOpts(const QString &str)
{
    m_customoptions = str;
}

void KgpgSelectPublicKeyDlg::refreshKeys()
{
    m_keyslist->clear();

    QStringList groups = QStringList::split(",", KGpgSettings::groups());
    if (!groups.isEmpty())
        for (QStringList::Iterator it = groups.begin(); it != groups.end(); ++it)
            if (!QString(*it).isEmpty())
            {
                KeyViewItem *item = new KeyViewItem(m_keyslist, QString(*it), QString::null, QString::null, false);
                item->setPixmap(0, m_keygroup);
            }

    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(readPublicKeysFinished(KgpgListKeys, KgpgInterface*)), this, SLOT(refreshKeysReady(KgpgListKeys, KgpgInterface*)));
    interface->readPublicKeys();
}

void KgpgSelectPublicKeyDlg::refreshKeysReady(KgpgListKeys keys, KgpgInterface *interface)
{
    delete interface;

    for (int i = 0; i < keys.size(); ++i)
    {
        bool dead = false;
        KgpgKey key = keys.at(i);
        QString id = QString("0x" + key.id());

        QChar c = key.trust();
        if (c == 'o' || c == 'q' || c == 'n' || c == 'm')
            m_untrustedlist << id;
        else
        if (c == 'i' || c == 'd' || c == 'r' || c == 'e')
            dead = true;
        else
        if (c != 'f' && c != 'u')
            m_untrustedlist << id;

        if (key.valide() == false)
            dead = true;

        QString keyname = KgpgInterface::checkForUtf8(key.name());
        if (!dead && !keyname.isEmpty())
        {
            QString defaultKey = KGpgSettings::defaultKey().right(8);
            bool isDefaultKey = false;
            if (key.id() == defaultKey)
                isDefaultKey = true;

            KeyViewItem *item = new KeyViewItem(m_keyslist, keyname, key.email(), id, isDefaultKey);
            if (m_seclist.find(id, 0, false) != -1)
                item->setPixmap(0, m_keypair);
            else
                item->setPixmap(0, m_keysingle);
        }
    }

    slotPreSelect();
    selectionChanged();
}

void KgpgSelectPublicKeyDlg::slotPreSelect()
{
    Q3ListViewItem *it = 0;
    if (!m_keyslist->firstChild())
        return;

    if (m_fmode)
        it = m_keyslist->findItem(KGpgSettings::defaultKey(), 2);

    if (!m_cbuntrusted->isChecked())
        sort();

    if (m_fmode)
    {
        m_keyslist->clearSelection();
        m_keyslist->setSelected(it, true);
        m_keyslist->setCurrentItem(it);
        m_keyslist->ensureItemVisible(it);
    }

    emit keyListFilled();
}

void KgpgSelectPublicKeyDlg::refresh(const bool &state)
{
    if (state)
        enable();
    else
        sort();
}

void KgpgSelectPublicKeyDlg::sort()
{
    bool reselect = false;
    Q3ListViewItem *current = m_keyslist->firstChild();
    if (current == NULL)
        return;

    if ((!current->text(2).isEmpty()) && (m_untrustedlist.find(current->text(2)) != m_untrustedlist.end()))
    {
        if (current->isSelected())
        {
            current->setSelected(false);
            reselect = true;
        }
        current->setVisible(false);
    }

    while (current->nextSibling())
    {
        current = current->nextSibling();
        if ((!current->text(2).isEmpty()) && (m_untrustedlist.find(current->text(2)) != m_untrustedlist.end()))
        {
            if (current->isSelected())
            {
                current->setSelected(false);
                reselect = true;
            }
            current->setVisible(false);
        }
    }

    if (reselect || !m_keyslist->currentItem()->isVisible())
    {
        Q3ListViewItem *firstvisible;
        firstvisible = m_keyslist->firstChild();
        while (firstvisible->isVisible() != true)
        {
            firstvisible = firstvisible->nextSibling();
            if (firstvisible == NULL)
                return;
        }

        m_keyslist->setSelected(firstvisible, true);
        m_keyslist->setCurrentItem(firstvisible);
        m_keyslist->ensureItemVisible(firstvisible);
    }
}

void KgpgSelectPublicKeyDlg::enable()
{
    Q3ListViewItem *current = m_keyslist->firstChild();
    if (current == NULL)
        return;
    current->setVisible(true);

    while (current->nextSibling())
    {
        current = current->nextSibling();
        current->setVisible(true);
    }

    m_keyslist->ensureItemVisible(m_keyslist->currentItem());
}

void KgpgSelectPublicKeyDlg::slotGotoDefaultKey()
{
    Q3ListViewItem *myDefaulKey = m_keyslist->findItem(KGpgSettings::defaultKey(), 2);
    m_keyslist->clearSelection();
    m_keyslist->setCurrentItem(myDefaulKey);
    m_keyslist->setSelected(myDefaulKey, true);
    m_keyslist->ensureItemVisible(myDefaulKey);
}

#include "popuppublic.moc"
