// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ADDCONTACTDIALOG_H
#define ADDCONTACTDIALOG_H

#include <QDialog>

namespace Ui {
class AddContactDialog;
}

class AddContactDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddContactDialog(QWidget *parent = nullptr);
    ~AddContactDialog();

private:
    Ui::AddContactDialog *ui;

private Q_SLOTS:
   void on_pushButtonAdd_clicked();
   void on_lineEditName_textChanged();
};

#endif // ADDCONTACTDIALOG_H
