// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/claimbitnamedialog.h>
#include <qt/forms/ui_claimbitnamedialog.h>

ClaimBitNameDialog::ClaimBitNameDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ClaimBitNameDialog)
{
    ui->setupUi(this);
}

ClaimBitNameDialog::~ClaimBitNameDialog()
{
    delete ui;
}
