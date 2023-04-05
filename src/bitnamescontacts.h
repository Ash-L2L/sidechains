// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BITNAMES_CONTACTS_H
#define BITCOIN_BITNAMES_CONTACTS_H

#include <uint256.h>

#include <vector>

class BitNamesContacts
{
public:
    BitNamesContacts();

    void SetContacts(const std::vector<uint256> vContact);
    void SetCurrentID(const uint256 id);

    std::vector<uint256> GetContacts() const;
    uint256 GetCurrentID() const;

private:
    // My contact's ID's
    std::vector<uint256> vContactID;

    // My current ID
    uint256 current;
};

#endif // BITCOIN_BITNAMES_CONTACTS_H
