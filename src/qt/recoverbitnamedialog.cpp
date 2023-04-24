// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/recoverbitnamedialog.h>
#include <qt/forms/ui_recoverbitnamedialog.h>

RecoverBitNameDialog::RecoverBitNameDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RecoverBitNameDialog)
{
    ui->setupUi(this);
}

RecoverBitNameDialog::~RecoverBitNameDialog()
{
    delete ui;
}
