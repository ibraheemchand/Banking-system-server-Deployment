#pragma once
#include "Bank.h"
#include <string>

// Admin uses composition over Bank. Acts as the single privileged entity.
class Admin {
private:
    Bank&       bank;
    std::string username;
    std::string adminPassword;

public:
    explicit Admin(Bank&              bank,
                   const std::string& username      = "admin",
                   const std::string& adminPassword = "admin123");

    // ── Admin authentication ─────────────────────────────────────────────────
    bool validateAdminPassword(const std::string& password) const;

    // ── Account management (admin-only) ──────────────────────────────────────
    bool createAccount(const std::string& accNum,
                       const std::string& name,
                       double             initialBalance,
                       const std::string& password);

    bool transferFunds(const std::string& from,
                       const std::string& to,
                       double             amount);

    bool changeUserPassword(const std::string& accNum,
                            const std::string& newPass);

    std::string getAccountsList() const;   // all accounts JSON
    std::string getUsername()     const;

    // ── User-level operations ─────────────────────────────────────────────────
    bool validateUserCredentials(const std::string& accNum,
                                 const std::string& password) const;

    bool userTransfer(const std::string& fromAcc,
                      const std::string& toAcc,
                      double             amount);

    std::string getUserAccountJson(const std::string& accNum) const;
};
