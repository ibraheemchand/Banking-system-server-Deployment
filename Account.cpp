#include "Account.h"
#include <sstream>
#include <iomanip>

Account::Account(const std::string& accNum,
                 const std::string& name,
                 double             initialBalance,
                 const std::string& password)
    : accountNumber(accNum), holderName(name),
      balance(initialBalance), password(password) {}

std::string Account::getAccountNumber() const { return accountNumber; }
std::string Account::getHolderName()    const { return holderName; }
double      Account::getBalance()       const { return balance; }
std::string Account::getPassword()      const { return password; }

void Account::setPassword(const std::string& newPass) { password = newPass; }

void Account::deposit(double amount) {
    if (amount > 0) balance += amount;
}

bool Account::withdraw(double amount) {
    if (amount > 0 && balance >= amount) {
        balance -= amount;
        return true;
    }
    return false;
}

std::string Account::toJson() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "{"
        << "\"accountNumber\":\"" << accountNumber << "\","
        << "\"holderName\":\""    << holderName    << "\","
        << "\"balance\":"         << balance
        << "}";
    return oss.str();
}
