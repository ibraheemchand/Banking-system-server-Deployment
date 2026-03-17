#pragma once
#include "Account.h"
#include <vector>
#include <string>

class Bank {
private:
    std::vector<Account> accounts;

    Account*       findAccount(const std::string& accNum);
    const Account* findAccount(const std::string& accNum) const;

public:
    bool accountExists(const std::string& accNum) const;
    bool validateCredentials(const std::string& accNum,
                             const std::string& password) const;

    // Returns false if account number already exists, balance < 0, or password empty
    bool createAccount(const std::string& accNum,
                       const std::string& name,
                       double             initialBalance,
                       const std::string& password);

    // Returns false if either account is not found or insufficient funds
    bool transfer(const std::string& fromAcc,
                  const std::string& toAcc,
                  double             amount);

    bool changePassword(const std::string& accNum,
                        const std::string& newPass);

    std::string getAllAccountsJson()              const;
    std::string getAccountJson(const std::string& accNum) const;
};
