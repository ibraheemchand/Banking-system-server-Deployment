#include "Admin.h"

Admin::Admin(Bank& bank, const std::string& username)
    : bank(bank), username(username) {}

bool Admin::createAccount(const std::string& accNum,
                          const std::string& name,
                          double             initialBalance) {
    return bank.createAccount(accNum, name, initialBalance);
}

bool Admin::transferFunds(const std::string& from,
                          const std::string& to,
                          double             amount) {
    return bank.transfer(from, to, amount);
}

std::string Admin::getAccountsList() const {
    return bank.getAllAccountsJson();
}

std::string Admin::getUsername() const {
    return username;
}
