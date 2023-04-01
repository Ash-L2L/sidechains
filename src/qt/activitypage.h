// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ACTIVITYPAGE_H
#define ACTIVITYPAGE_H

#include <QWidget>

namespace Ui {
class ActivityPage;
}

class ActivityPage : public QWidget
{
    Q_OBJECT

public:
    explicit ActivityPage(QWidget *parent = nullptr);
    ~ActivityPage();

private:
    Ui::ActivityPage *ui;
};

#endif // ACTIVITYPAGE_H
