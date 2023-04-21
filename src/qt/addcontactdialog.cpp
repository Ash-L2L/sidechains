// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/addcontactdialog.h>
#include <qt/forms/ui_addcontactdialog.h>

#include <QMessageBox>

#include <bitnamescontacts.h>
#include <hash.h>
#include <txdb.h>
#include <uint256.h>
#include <validation.h>

AddContactDialog::AddContactDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddContactDialog)
{
    ui->setupUi(this);
}

AddContactDialog::~AddContactDialog()
{
    delete ui;
}

void AddContactDialog::on_pushButtonAdd_clicked()
{
    std::string str = ui->lineEditName->text().toStdString();

    uint256 id;
    CHash256().Write((unsigned char*)&str[0], str.size()).Finalize((unsigned char*)&id);

    // Get BitNameDB data
    BitName bitname;
    if (!pbitnametree->GetBitName(id, bitname)) {
        return; // todo messagebox
    }

    bitnamesContacts.AddContact(bitname.name_hash);

    QMessageBox::information(this, tr("Contact added!"),
        tr("BitNames contact saved!\n"),
        QMessageBox::Ok);
}
