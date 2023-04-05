// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitnamescontacts.h>

BitNamesContacts::BitNamesContacts()
{

}

void BitNamesContacts::SetContacts(const std::vector<uint256> vContact)
{
    vContactID = vContact;
}

void BitNamesContacts::SetCurrentID(const uint256 id)
{
    current = id;
}

std::vector<uint256> BitNamesContacts::GetContacts() const
{
    return vContactID;
}

uint256 BitNamesContacts::GetCurrentID() const
{
    return current;
}

