#pragma once
#include <string>

class Account {
private:
    std::string accountNumber;
    std::string holderName;
    double balance;

public:
    Account(const std::string& accNum, const std::string& name, double initialBalance);

    std::string getAccountNumber() const;
    std::string getHolderName() const;
    double getBalance() const;

    void deposit(double amount);
    bool withdraw(double amount);

    std::string toJson() const;
};
