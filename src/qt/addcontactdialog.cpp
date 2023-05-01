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
        QMessageBox::critical(this, tr("Failed to lookup BitName!"),
            tr("The name you have entered could not be resolved!\n"),
            QMessageBox::Ok);
        return;
    }

    bitnamesContacts.AddContact(id, str);

    QMessageBox::information(this, tr("Contact added!"),
        tr("BitNames contact saved!\n"),
        QMessageBox::Ok);

    this->close();
}

void AddContactDialog::on_lineEditName_textChanged()
{
    std::string str = ui->lineEditName->text().toStdString();

    uint256 id;
    CHash256().Write((unsigned char*)&str[0], str.size()).Finalize((unsigned char*)&id);

    // Get BitNameDB data
    BitName bitname;
    if (pbitnametree->GetBitName(id, bitname)) {
        ui->labelError->setVisible(false);

        QString result = "";
        result += "Name hash: " + QString::fromStdString(bitname.name_hash.ToString());
        result += "\nTxID: " + QString::fromStdString(bitname.txid.front().ToString());

        boost::optional<uint256> commitment = bitname.commitment.front();
        if (commitment)
            result += "\nCommitment: " + QString::fromStdString((*commitment).ToString());

        boost::optional<in_addr_t> opt_in4 = bitname.in4.front();
        if (opt_in4) {
            struct in_addr in4;
            in4.s_addr = *opt_in4;
            result += "\nIPv4: " + QString::fromStdString(std::string(inet_ntoa(in4)));
        }

        ui->textBrowser->insertPlainText(result);
    } else {
        ui->labelError->setVisible(true);
        ui->textBrowser->clear();
    }
}
