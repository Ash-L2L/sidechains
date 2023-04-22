// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BITNAMES_CONTACTS_H
#define BITCOIN_BITNAMES_CONTACTS_H

#include <uint256.h>

#include <string>
#include <vector>

struct Contact
{
    uint256 id;
    std::string name;
};

class BitNamesContacts
{
public:
    BitNamesContacts();

    void AddContact(const uint256& id, const std::string& name);
    void SetContacts(const std::vector<Contact> vContact);
    void SetCurrentID(const uint256 id);
    bool GetName(const uint256& id, std::string& strName);

    std::vector<Contact> GetContacts() const;
    uint256 GetCurrentID() const;

private:
    // My contacts
    std::vector<Contact> vContact;

    // My current ID
    uint256 current;
};

#endif // BITCOIN_BITNAMES_CONTACTS_H
