#pragma once
#include <string>

class Account {
private:
    std::string accountNumber;
    std::string holderName;
    double      balance;
    std::string password;

public:
    Account(const std::string& accNum,
            const std::string& name,
            double             initialBalance,
            const std::string& password);

    std::string getAccountNumber() const;
    std::string getHolderName()    const;
    double      getBalance()       const;
    std::string getPassword()      const;

    void setPassword(const std::string& newPass);
    void deposit(double amount);
    bool withdraw(double amount);

    // JSON representation (never includes password)
    std::string toJson() const;
};
