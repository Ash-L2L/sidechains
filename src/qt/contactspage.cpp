// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/contactspage.h>
#include <qt/forms/ui_contactspage.h>

#include <qt/platformstyle.h>
#include <qt/addcontactdialog.h>
#include <qt/switchbitnamesdialog.h>

ContactsPage::ContactsPage(const PlatformStyle *_platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContactsPage),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);
}

ContactsPage::~ContactsPage()
{
    delete ui;
}

void ContactsPage::on_pushButtonAdd_clicked()
{
    AddContactDialog dialog;
    dialog.exec();
}

void ContactsPage::on_pushButtonSwitch_clicked()
{
    SwitchBitNamesDialog dialog;
    dialog.exec();
}
