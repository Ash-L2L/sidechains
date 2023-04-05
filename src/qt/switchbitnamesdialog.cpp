// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/switchbitnamesdialog.h>
#include <qt/forms/ui_switchbitnamesdialog.h>

#include <qt/claimbitnamedialog.h>
#include <qt/registerbitnamedialog.h>
#include <qt/recoverbitnamedialog.h>

SwitchBitNamesDialog::SwitchBitNamesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SwitchBitNamesDialog)
{
    ui->setupUi(this);
}

SwitchBitNamesDialog::~SwitchBitNamesDialog()
{
    delete ui;
}

void SwitchBitNamesDialog::on_pushButtonRegister_clicked()
{
    RegisterBitNameDialog dialog;
    dialog.exec();
}

void SwitchBitNamesDialog::on_pushButtonClaim_clicked()
{
    ClaimBitNameDialog dialog;
    dialog.exec();
}

void SwitchBitNamesDialog::on_pushButtonRecover_clicked()
{
    RecoverBitNameDialog dialog;
    dialog.exec();
}
