// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/contactspage.h>
#include <qt/forms/ui_contactspage.h>

ContactsPage::ContactsPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContactsPage)
{
    ui->setupUi(this);
}

ContactsPage::~ContactsPage()
{
    delete ui;
}
