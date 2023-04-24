// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CONTACTSELECTIONDIALOG_H
#define CONTACTSELECTIONDIALOG_H

#include <QDialog>

namespace Ui {
class ContactSelectionDialog;
}

class ContactSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ContactSelectionDialog(QWidget *parent = nullptr);
    ~ContactSelectionDialog();

private:
    Ui::ContactSelectionDialog *ui;
};

#endif // CONTACTSELECTIONDIALOG_H
