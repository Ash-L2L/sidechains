// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/contactselectiondialog.h>
#include <qt/forms/ui_contactselectiondialog.h>

ContactSelectionDialog::ContactSelectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ContactSelectionDialog)
{
    ui->setupUi(this);
}

ContactSelectionDialog::~ContactSelectionDialog()
{
    delete ui;
}
