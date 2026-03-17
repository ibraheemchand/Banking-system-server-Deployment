#pragma once
#include "Account.h"
#include <vector>
#include <string>

class Bank {
private:
    std::vector<Account> accounts;

public:
    bool accountExists(const std::string& accNum) const;

    // Returns false if account number already exists or balance is negative
    bool createAccount(const std::string& accNum,
                       const std::string& name,
                       double             initialBalance);

    // Returns false if either account is not found or insufficient funds
    bool transfer(const std::string& fromAcc,
                  const std::string& toAcc,
                  double             amount);

    std::string getAllAccountsJson() const;
};
