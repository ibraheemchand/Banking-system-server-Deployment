#include "Admin.h"

Admin::Admin(Bank&              bank,
             const std::string& username,
             const std::string& adminPassword)
    : bank(bank), username(username), adminPassword(adminPassword) {}

bool Admin::validateAdminPassword(const std::string& password) const {
    return password == adminPassword;
}

bool Admin::createAccount(const std::string& accNum,
                          const std::string& name,
                          double             initialBalance,
                          const std::string& password) {
    return bank.createAccount(accNum, name, initialBalance, password);
}

bool Admin::transferFunds(const std::string& from,
                          const std::string& to,
                          double             amount) {
    return bank.transfer(from, to, amount);
}

bool Admin::changeUserPassword(const std::string& accNum,
                               const std::string& newPass) {
    return bank.changePassword(accNum, newPass);
}

std::string Admin::getAccountsList() const {
    return bank.getAllAccountsJson();
}

std::string Admin::getUsername() const {
    return username;
}

bool Admin::validateUserCredentials(const std::string& accNum,
                                    const std::string& password) const {
    return bank.validateCredentials(accNum, password);
}

bool Admin::userTransfer(const std::string& fromAcc,
                         const std::string& toAcc,
                         double             amount) {
    return bank.transfer(fromAcc, toAcc, amount);
}

std::string Admin::getUserAccountJson(const std::string& accNum) const {
    return bank.getAccountJson(accNum);
}
