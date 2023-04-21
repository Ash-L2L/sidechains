// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/registerbitnamedialog.h>
#include <qt/forms/ui_registerbitnamedialog.h>

#include <QMessageBox>

#include <base58.h>
#include <primitives/transaction.h>
#include <validation.h>
#include <wallet/wallet.h>

RegisterBitNameDialog::RegisterBitNameDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterBitNameDialog)
{
    ui->setupUi(this);
}

RegisterBitNameDialog::~RegisterBitNameDialog()
{
    delete ui;
}

void RegisterBitNameDialog::on_pushButtonReserve_clicked()
{
    std::string strName = ui->lineEditReserveName->text().toStdString();

    if (strName.empty()) {
        QMessageBox::critical(this, tr("Failed to reserve BitName!"),
            tr("Invalid name!\n"),
            QMessageBox::Ok);
    }
    // Fee
    CAmount nFee = ui->amountReserveFee->value();
    if (nFee <= 0) {
        QMessageBox::critical(this, tr("Failed to reserve BitName!"),
            tr("Invalid fee amount!\n"),
            QMessageBox::Ok);
    }
    // Destination address
    CTxDestination dest = DecodeDestination(ui->lineEditReserveAddress->text().toStdString());
    if (!IsValidDestination(dest)) {
        QMessageBox::critical(this, tr("Failed to reserve BitName!"),
            tr("Invalid address!\n"),
            QMessageBox::Ok);
    }

    if (vpwallets.empty()) {
        QMessageBox::critical(this, tr("Failed to reserve BitName!"),
            tr("No active wallet!\n"),
            QMessageBox::Ok);
    }

    EnsureWalletIsUnlocked(vpwallets[0]);
    vpwallets[0]->BlockUntilSyncedToCurrentChain();

    LOCK2(cs_main, vpwallets[0]->cs_wallet);

    CTransactionRef tx;
    std::string strFail = "";
    if (!vpwallets[0]->ReserveBitName(tx, strFail, strName, nFee))
    {
        QMessageBox::critical(this, tr("Failed to reserve BitName!"),
            "Error: " + QString::fromStdString(strFail),
            QMessageBox::Ok);
    } else {
        QMessageBox::information(this, tr("Reserved BitName!"),
            "TxID:\n" + QString::fromStdString(tx->GetHash().ToString()),
            QMessageBox::Ok);
    }
}

void RegisterBitNameDialog::on_pushButtonRegister_clicked()
{
    std::string strName = ui->lineEditRegisterName->text().toStdString();
    if (strName.empty()) {
        QMessageBox::critical(this, tr("Failed to register BitName!"),
            tr("Invalid name!\n"),
            QMessageBox::Ok);
    }
    // commitment
    uint256 commitment = uint256S(ui->lineEditCommitment->text().toStdString());
    if (commitment.IsNull()) {
        QMessageBox::critical(this, tr("Failed to register BitName!"),
            tr("Invalid commitment!\n"),
            QMessageBox::Ok);
    }
    // IPv4
    struct in_addr in4;
    std::string strIn4 = ui->lineEditIPv4->text().toStdString();
    if (strIn4.empty()) {
        QMessageBox::critical(this, tr("Failed to register BitName!"),
            tr("Missing IPv4 address!\n"),
            QMessageBox::Ok);
    }
    if (inet_pton(AF_INET, strIn4.c_str(), &in4) != 1) {
        QMessageBox::critical(this, tr("Failed to register BitName!"),
            tr("Invalid IPv4 address!\n"),
            QMessageBox::Ok);
    }
    // Fee
    CAmount nFee = ui->amountRegisterFee->value();
    if (nFee <= 0) {
        QMessageBox::critical(this, tr("Failed to register BitName!"),
            tr("Invalid fee amount!\n"),
            QMessageBox::Ok);
    }
    // Destination address
    CTxDestination dest = DecodeDestination(ui->lineEditRegisterAddress->text().toStdString());
    if (!IsValidDestination(dest)) {
        QMessageBox::critical(this, tr("Failed to register BitName!"),
            tr("Invalid address!\n"),
            QMessageBox::Ok);
    }

    if (vpwallets.empty()) {
        QMessageBox::critical(this, tr("Failed to reserve BitName!"),
            tr("No active wallet!\n"),
            QMessageBox::Ok);
    }

    EnsureWalletIsUnlocked(vpwallets[0]);
    vpwallets[0]->BlockUntilSyncedToCurrentChain();

    LOCK2(cs_main, vpwallets[0]->cs_wallet);

    CTransactionRef tx;
    std::string strFail = "";
    if (!vpwallets[0]->RegisterBitName(tx, strFail, strName, commitment, in4, nFee))
    {
        QMessageBox::critical(this, tr("Failed to register BitName!"),
            "Error: " + QString::fromStdString(strFail),
            QMessageBox::Ok);
    } else {
        QMessageBox::information(this, tr("Registered BitName!"),
            "TxID:\n" + QString::fromStdString(tx->GetHash().ToString()),
            QMessageBox::Ok);
    }
}
