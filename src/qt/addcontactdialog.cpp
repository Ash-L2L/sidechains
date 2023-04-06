// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/addcontactdialog.h>
#include <qt/forms/ui_addcontactdialog.h>

#include <QMessageBox>

#include <bitnamescontacts.h>
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
    std::string strName = ui->lineEditName->text().toStdString();

    // Get BitNameDB data
    BitName bitname;
    if (!pbitnametree->GetBitName(strName, bitname)) {
        return; // todo messagebox
    }

    bitnamesContacts.AddContact(bitname.nID);

    QMessageBox::information(this, tr("Contact added!"),
        tr("BitNames contact saved!\n"),
        QMessageBox::Ok);
}
