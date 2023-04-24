// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WRITEPAYMAILDIALOG_H
#define WRITEPAYMAILDIALOG_H

#include <QDialog>

namespace Ui {
class WritePaymailDialog;
}

class WritePaymailDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WritePaymailDialog(QWidget *parent = nullptr);
    ~WritePaymailDialog();

private:
    Ui::WritePaymailDialog *ui;
};

#endif // WRITEPAYMAILDIALOG_H
