// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BROWSEPAGE_H
#define BROWSEPAGE_H

#include <QWidget>

class PlatformStyle;

QT_BEGIN_NAMESPACE
class QTableWidgetItem;
QT_END_NAMESPACE

namespace Ui {
class BrowsePage;
}

class BrowsePage : public QWidget
{
    Q_OBJECT

public:
    explicit BrowsePage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~BrowsePage();

private:
    Ui::BrowsePage *ui;

    const PlatformStyle *platformStyle;

    void Update();

    bool Resolve(const std::string& strName);

private Q_SLOTS:
    void on_lineEditSearch_returnPressed();
    void on_tableWidgetContacts_itemClicked(QTableWidgetItem* item);
};

#endif // BROWSEPAGE_H
