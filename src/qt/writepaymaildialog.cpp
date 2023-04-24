// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/writepaymaildialog.h>
#include <qt/forms/ui_writepaymaildialog.h>

WritePaymailDialog::WritePaymailDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WritePaymailDialog)
{
    ui->setupUi(this);
}

WritePaymailDialog::~WritePaymailDialog()
{
    delete ui;
}
