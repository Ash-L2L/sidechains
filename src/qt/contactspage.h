// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CONTACTSPAGE_H
#define CONTACTSPAGE_H

#include <QWidget>

class PlatformStyle;

namespace Ui {
class ContactsPage;
}

class ContactsPage : public QWidget
{
    Q_OBJECT

public:
    explicit ContactsPage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~ContactsPage();

private:
    Ui::ContactsPage *ui;

    const PlatformStyle *platformStyle;
};

#endif // CONTACTSPAGE_H
