// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitnamescontacts.h>

BitNamesContacts::BitNamesContacts()
{

}
void BitNamesContacts::AddContact(const uint256& id, const std::string& name)
{
    Contact contact;
    contact.id = id;
    contact.name = name;
    vContact.push_back(contact);
}

void BitNamesContacts::SetContacts(const std::vector<Contact> vContactIn)
{
    vContact = vContactIn;
}

void BitNamesContacts::SetCurrentID(const uint256 id)
{
    current = id;
}

std::vector<Contact> BitNamesContacts::GetContacts() const
{
    return vContact;
}

uint256 BitNamesContacts::GetCurrentID() const
{
    return current;
}

bool BitNamesContacts::GetName(const uint256& id, std::string& strName)
{
    for (const Contact& c : vContact) {
        if (c.id == id) {
            strName = c.name;
            return true;
        }
    }
    return false;
}
