// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/contactspage.h>
#include <qt/forms/ui_contactspage.h>

#include <qt/platformstyle.h>
#include <qt/addcontactdialog.h>
#include <qt/switchbitnamesdialog.h>

#include <bitnamescontacts.h>
#include <txdb.h>
#include <uint256.h>
#include <validation.h>

ContactsPage::ContactsPage(const PlatformStyle *_platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContactsPage),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);

    Update();
}

ContactsPage::~ContactsPage()
{
    delete ui;
}

void ContactsPage::on_pushButtonAdd_clicked()
{
    AddContactDialog dialog;
    dialog.exec();

    Update();
}

void ContactsPage::on_pushButtonSwitch_clicked()
{
    SwitchBitNamesDialog dialog;
    dialog.exec();
}

void ContactsPage::Update()
{
    // List my contacts on table

    ui->tableWidgetContacts->setRowCount(0);

    std::vector<uint256> vContact = bitnamesContacts.GetContacts();

    int nRow = 0;
    for (const uint256& u : vContact) {
        ui->tableWidgetContacts->insertRow(nRow);

        // Get BitNameDB data
        BitName bitname;
        if (!pbitnametree->GetBitName(u, bitname)) {
            return;
        }

        QString name = QString::fromStdString(bitname.name_hash.ToString());

        // Add to table
        QTableWidgetItem* nameItem = new QTableWidgetItem(name);
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        ui->tableWidgetContacts->setItem(nRow /* row */, 0 /* col */, nameItem);

        nRow++;
    }
}
