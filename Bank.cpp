#include "Bank.h"
#include <sstream>

bool Bank::accountExists(const std::string& accNum) const {
    for (const auto& acc : accounts)
        if (acc.getAccountNumber() == accNum) return true;
    return false;
}

bool Bank::createAccount(const std::string& accNum,
                         const std::string& name,
                         double             initialBalance) {
    if (initialBalance < 0) return false;
    for (const auto& acc : accounts)
        if (acc.getAccountNumber() == accNum) return false;   // duplicate
    accounts.emplace_back(accNum, name, initialBalance);
    return true;
}

bool Bank::transfer(const std::string& fromAcc,
                    const std::string& toAcc,
                    double             amount) {
    if (amount <= 0) return false;

    Account* from = nullptr;
    Account* to   = nullptr;
    for (auto& acc : accounts) {
        if (acc.getAccountNumber() == fromAcc) from = &acc;
        if (acc.getAccountNumber() == toAcc)   to   = &acc;
    }
    if (!from || !to || from == to) return false;
    if (!from->withdraw(amount))    return false;   // insufficient funds
    to->deposit(amount);
    return true;
}

std::string Bank::getAllAccountsJson() const {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < accounts.size(); ++i) {
        if (i > 0) oss << ",";
        oss << accounts[i].toJson();
    }
    oss << "]";
    return oss.str();
}
