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
#include <kactioncollection.h>
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
    if (def && column < 2)
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

KgpgSelectPublicKeyDlg::KgpgSelectPublicKeyDlg(QWidget *parent, const QString &sfile, const bool &filemode, const bool &enabledshred, const KShortcut &goDefaultKey)
                      : KDialog(parent, i18n("Select Public Key"), Details | Ok | Cancel)
{
    setModal(true);
    setButtonText(Details, i18n("O&ptions"));

    m_fmode = filemode;
    if (m_fmode)
        setCaption(i18n("Select Public Key for %1", sfile));

    KIconLoader *loader = KGlobal::iconLoader();
    m_keypair = loader->loadIcon("kgpg_key2", K3Icon::Small, 20);
    m_keysingle = loader->loadIcon("kgpg_key1", K3Icon::Small, 20);
    m_keygroup = loader->loadIcon("kgpg_key3", K3Icon::Small, 20);

    QWidget *page = new QWidget(this);

    KHBox *searchbar = new KHBox(page);
    searchbar->setFrameShape(QFrame::StyledPanel);

    QToolButton *clearsearch = new QToolButton(searchbar);
    clearsearch->setText(i18n("Clear Search"));
    clearsearch->setIcon(SmallIconSet(QApplication::isRightToLeft() ? "clear_left" : "locationbar_erase"));

    QLabel *searchlabel = new QLabel(i18n("&Search: "), searchbar);

    K3ListViewSearchLine* searchlineedit = new K3ListViewSearchLine(searchbar);
    searchlabel->setBuddy(searchlineedit);

    m_keyslist = new K3ListView(page);
    m_keyslist->setFullWidth(true);
    m_keyslist->setRootIsDecorated(false);
    m_keyslist->setShowSortIndicator(true);
    m_keyslist->setAllColumnsShowFocus(true);
    m_keyslist->addColumn(i18n("Name"));
    m_keyslist->addColumn(i18n("Email"));
    m_keyslist->addColumn(i18n("ID"));
    m_keyslist->setSelectionModeExt(K3ListView::Extended);
    m_keyslist->setColumnWidthMode(0, K3ListView::Manual);
    m_keyslist->setColumnWidthMode(1, K3ListView::Manual);
    m_keyslist->setColumnWidth(0, 210);
    m_keyslist->setColumnWidth(1, 210);
    m_keyslist->setWhatsThis(i18n("<b>Public keys list</b>: select the key that will be used for encryption."));
    searchlineedit->setListView(m_keyslist);

    KVBox *optionsbox = new KVBox(page);
    optionsbox->setFrameShape(QFrame::StyledPanel);
    setDetailsWidget(optionsbox);

    m_cbarmor = new QCheckBox(i18n("ASCII armored encryption"), optionsbox);
    m_cbarmor->setChecked(KGpgSettings::asciiArmor());
    m_cbarmor->setWhatsThis(i18n("<b>ASCII encryption</b>: makes it possible to open the encrypted file/message in a text editor"));

    m_cbuntrusted = new QCheckBox(i18n("Allow encryption with untrusted keys"), optionsbox);
    m_cbuntrusted->setChecked(KGpgSettings::allowUntrustedKeys());
    m_cbuntrusted->setWhatsThis(i18n("<b>Allow encryption with untrusted keys</b>: when you import a public key, it is usually "
                    "marked as untrusted and you cannot use it unless you sign it in order to make it 'trusted'. Checking this "
                    "box enables you to use any key, even if it has not be signed."));

    m_cbhideid = new QCheckBox(i18n("Hide user id"), optionsbox);
    m_cbhideid->setChecked(KGpgSettings::hideUserID());
    m_cbhideid->setWhatsThis(i18n("<b>Hide user ID</b>: Do not put the keyid into encrypted packets. This option hides the receiver "
                    "of the message and is a countermeasure against traffic analysis. It may slow down the decryption process because "
                    "all available secret keys are tried."));

    if (filemode)
    {
        KHBox *shredbox = new KHBox(optionsbox);
        shredbox->setMargin(0);

        m_cbshred = new QCheckBox(i18n("Shred source file"), shredbox);
        m_cbshred->setWhatsThis(i18n("<b>Shred source file</b>: permanently remove source file. No recovery will be possible"));
        m_cbshred->setEnabled(enabledshred);
        m_cbshred->setChecked(KGpgSettings::shredSource());

        QString shredWhatsThis = i18n("<qt><b>Shred source file:</b><br /><p>Checking this option will shred (overwrite several times before erasing) the files you have encrypted. This way, it is almost impossible that the source file is recovered.</p><p><b>But you must be aware that this is not secure</b> on all file systems, and that parts of the file may have been saved in a temporary file or in the spooler of your printer if you previously opened it in an editor or tried to print it. Only works on files (not on folders).</p></qt>");

        (void) new KActiveLabel(i18n("<a href=\"whalinehis:%1\">Read this before using shredding</a>", shredWhatsThis), shredbox);
    }

    m_cbsymmetric = new QCheckBox(i18n("Symmetrical encryption"), optionsbox);
    m_cbsymmetric->setWhatsThis(i18n("<b>Symmetrical encryption</b>: encryption does not use keys. You just need to give a password "
                                     "to encrypt/decrypt the file"));

    QVBoxLayout *dialoglayout = new QVBoxLayout(page);
    dialoglayout->setSpacing(spacingHint());
    dialoglayout->setMargin(marginHint());
    dialoglayout->addWidget(searchbar);
    dialoglayout->addWidget(m_keyslist);
    dialoglayout->addWidget(optionsbox);
    page->setLayout(dialoglayout);

    if (KGpgSettings::allowCustomEncryptionOptions())
    {
        m_customoptions = KGpgSettings::customEncryptionOptions();

        KHBox *expertbox = new KHBox(page);
        (void) new QLabel(i18n("Custom option:"), expertbox);

        KLineEdit *optiontxt = new KLineEdit(expertbox);
        optiontxt->setText(m_customoptions);
        optiontxt->setWhatsThis(i18n("<b>Custom option</b>: for experienced users only, allows you to enter a gpg command line option, like: '--armor'"));

        dialoglayout->addWidget(expertbox);

        connect(optiontxt, SIGNAL(textChanged(const QString &)), this, SLOT(slotCustomOpts(const QString &)));
    }

    KActionCollection *actcol = new KActionCollection(this);
    KAction *action = new KAction(i18n("&Go to Default Key"), actcol, "go_default_key");
    action->setShortcut(goDefaultKey);

    connect(action, SIGNAL(triggered(bool)), SLOT(slotGotoDefaultKey()));
    connect(clearsearch, SIGNAL(pressed()), searchlineedit, SLOT(clear()));
    connect(m_cbsymmetric, SIGNAL(toggled(bool)), this, SLOT(slotSymmetric(bool)));
    connect(m_cbuntrusted, SIGNAL(toggled(bool)), this, SLOT(slotUntrusted(bool)));
    connect(m_keyslist, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()));
    connect(m_keyslist, SIGNAL(doubleClicked(Q3ListViewItem *, const QPoint &, int)), this, SLOT(slotOk()));

    KgpgInterface *interface = new KgpgInterface();
    KgpgListKeys list = interface->readSecretKeys(true);
    delete interface;

    m_seclist = QString::null;
    for (int i = 0; i < list.size(); ++i)
        m_seclist += ", 0x" + list.at(i).id();

    slotFillKeysList();
    setMinimumSize(550, 200);
    updateGeometry();
    slotSelectionChanged();
    setMainWidget(page);
}

QStringList KgpgSelectPublicKeyDlg::selectedKeys() const
{
    QStringList selectedKeys;

    QList<Q3ListViewItem*> list = m_keyslist->selectedItems();
    for (int i = 0; i < list.count(); ++i)
        if (list.at(i) && list.at(i)->isVisible())
        {
            if (!list.at(i)->text(2).isEmpty())
                selectedKeys << list.at(i)->text(2); // get the id
            else
                selectedKeys << list.at(i)->text(0); // get the name
        }

    return selectedKeys;
}

bool KgpgSelectPublicKeyDlg::getSymmetric() const
{
    return m_cbsymmetric->checkState() == Qt::Checked;
}

bool KgpgSelectPublicKeyDlg::getUntrusted() const
{
    return m_cbuntrusted->checkState() == Qt::Checked;
}

bool KgpgSelectPublicKeyDlg::getArmor() const
{
    return m_cbarmor->checkState() == Qt::Checked;
}

bool KgpgSelectPublicKeyDlg::getHideId() const
{
    return m_cbhideid->checkState() == Qt::Checked;
}

bool KgpgSelectPublicKeyDlg::getShred() const
{
    return m_cbhideid->checkState() == Qt::Checked;
}

void KgpgSelectPublicKeyDlg::slotOk()
{
    QStringList selectedkeys;
    if (!m_cbsymmetric->isChecked())
    {
        selectedkeys = selectedKeys();
        if (selectedkeys.isEmpty())
            return;
    }

    QStringList returnOptions;
    if (m_cbuntrusted->isChecked())
        returnOptions << "--always-trust";
    if (m_cbarmor->isChecked())
        returnOptions << "--armor";
    if (m_cbhideid->isChecked())
        returnOptions << "--throw-keyid";

    if (KGpgSettings::allowCustomEncryptionOptions() && !m_customoptions.simplified().isEmpty())
        returnOptions << m_customoptions.simplified().split(" ");

    if (m_fmode)
        emit selectedKey(selectedkeys, returnOptions, m_cbshred->isChecked(), m_cbsymmetric->isChecked());
    else
        emit selectedKey(selectedkeys, returnOptions, false, m_cbsymmetric->isChecked());

    slotButtonClicked(Ok);
}

void KgpgSelectPublicKeyDlg::slotFillKeysList()
{
    m_keyslist->clear();

    QStringList groups = KGpgSettings::groups().split(",");
    if (!groups.isEmpty())
        for (QStringList::Iterator it = groups.begin(); it != groups.end(); ++it)
            if (!QString(*it).isEmpty())
            {
                KeyViewItem *item = new KeyViewItem(m_keyslist, QString(*it), QString::null, QString::null, false);
                item->setPixmap(0, m_keygroup);
            }

    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(readPublicKeysFinished(KgpgListKeys, KgpgInterface*)), this, SLOT(slotFillKeysListReady(KgpgListKeys, KgpgInterface*)));
    interface->readPublicKeys();
}

void KgpgSelectPublicKeyDlg::slotFillKeysListReady(KgpgListKeys keys, KgpgInterface *interface)
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
            if (m_seclist.contains(id, Qt::CaseInsensitive))
                item->setPixmap(0, m_keypair);
            else
                item->setPixmap(0, m_keysingle);
        }
    }

    slotPreSelect();
    slotSelectionChanged();
}

void KgpgSelectPublicKeyDlg::slotPreSelect()
{
    Q3ListViewItem *it = 0;
    if (!m_keyslist->firstChild())
        return;

    if (m_fmode)
        it = m_keyslist->findItem(KGpgSettings::defaultKey(), 2);

    if (!m_cbuntrusted->isChecked())
        slotShowAllKeys();

    if (m_fmode)
    {
        m_keyslist->clearSelection();
        m_keyslist->setSelected(it, true);
        m_keyslist->setCurrentItem(it);
        m_keyslist->ensureItemVisible(it);
    }

    m_keyslist->ensureItemVisible(m_keyslist->currentItem());
}

void KgpgSelectPublicKeyDlg::slotSelectionChanged()
{
    if (m_cbsymmetric->isChecked())
        enableButtonOK(true);
    else
        enableButtonOK(!m_keyslist->selectedItems().isEmpty());
}

void KgpgSelectPublicKeyDlg::slotCustomOpts(const QString &str)
{
    m_customoptions = str;
}

void KgpgSelectPublicKeyDlg::slotSymmetric(const bool &state)
{
    m_keyslist->setEnabled(!state);
    m_cbuntrusted->setEnabled(!state);
    m_cbhideid->setEnabled(!state);
    slotSelectionChanged();
}

void KgpgSelectPublicKeyDlg::slotUntrusted(const bool &state)
{
    if (state)
        slotShowAllKeys();
    else
        slotHideUntrustedKeys();
}

void KgpgSelectPublicKeyDlg::slotShowAllKeys()
{
    Q3ListViewItem *current = m_keyslist->firstChild();
    if (current == 0)
        return;

    current->setVisible(true);
    while (current->nextSibling())
    {
        current = current->nextSibling();
        current->setVisible(true);
    }

    m_keyslist->ensureItemVisible(m_keyslist->currentItem());
}

void KgpgSelectPublicKeyDlg::slotHideUntrustedKeys()
{
    bool reselect = false;
    Q3ListViewItem *current = m_keyslist->firstChild();
    if (current == NULL)
        return;

    if (!current->text(2).isEmpty() && m_untrustedlist.contains(current->text(2)))
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
        if (!current->text(2).isEmpty() && m_untrustedlist.contains(current->text(2)))
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

void KgpgSelectPublicKeyDlg::slotGotoDefaultKey()
{
    Q3ListViewItem *myDefaulKey = m_keyslist->findItem(KGpgSettings::defaultKey(), 2);
    m_keyslist->clearSelection();
    m_keyslist->setCurrentItem(myDefaulKey);
    m_keyslist->setSelected(myDefaulKey, true);
    m_keyslist->ensureItemVisible(myDefaulKey);
}

#include "popuppublic.moc"
