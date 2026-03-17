#pragma once
#include "Bank.h"
#include <string>

// Admin inherits access to Bank operations through composition.
// It acts as the single privileged entity that can:
//   - Create accounts
//   - Transfer funds between accounts
//   - Retrieve the full account list
class Admin {
private:
    Bank&       bank;
    std::string username;

public:
    explicit Admin(Bank& bank, const std::string& username = "admin");

    bool        createAccount(const std::string& accNum,
                              const std::string& name,
                              double             initialBalance);

    bool        transferFunds(const std::string& from,
                              const std::string& to,
                              double             amount);

    std::string getAccountsList() const;
    std::string getUsername()     const;
};
