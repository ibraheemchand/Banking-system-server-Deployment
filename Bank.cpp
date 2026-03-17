#include "Bank.h"
#include <sstream>

Account* Bank::findAccount(const std::string& accNum) {
    for (auto& acc : accounts)
        if (acc.getAccountNumber() == accNum) return &acc;
    return nullptr;
}

const Account* Bank::findAccount(const std::string& accNum) const {
    for (const auto& acc : accounts)
        if (acc.getAccountNumber() == accNum) return &acc;
    return nullptr;
}

bool Bank::accountExists(const std::string& accNum) const {
    return findAccount(accNum) != nullptr;
}

bool Bank::validateCredentials(const std::string& accNum,
                               const std::string& password) const {
    const Account* acc = findAccount(accNum);
    return acc && (acc->getPassword() == password);
}

bool Bank::createAccount(const std::string& accNum,
                         const std::string& name,
                         double             initialBalance,
                         const std::string& password) {
    if (initialBalance < 0 || password.empty()) return false;
    if (accountExists(accNum)) return false;
    accounts.emplace_back(accNum, name, initialBalance, password);
    return true;
}

bool Bank::transfer(const std::string& fromAcc,
                    const std::string& toAcc,
                    double             amount) {
    if (amount <= 0) return false;
    Account* from = findAccount(fromAcc);
    Account* to   = findAccount(toAcc);
    if (!from || !to || from == to) return false;
    if (!from->withdraw(amount)) return false;
    to->deposit(amount);
    return true;
}

bool Bank::changePassword(const std::string& accNum,
                          const std::string& newPass) {
    if (newPass.empty()) return false;
    Account* acc = findAccount(accNum);
    if (!acc) return false;
    acc->setPassword(newPass);
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

std::string Bank::getAccountJson(const std::string& accNum) const {
    const Account* acc = findAccount(accNum);
    return acc ? acc->toJson() : "null";
}
