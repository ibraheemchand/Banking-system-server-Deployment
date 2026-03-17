// ─────────────────────────────────────────────────────────────────────────────
// banking_system.cpp  –  Simple Console Banking System (C++17)
// Compile:  g++ -std=c++14 -Wall -o banking banking_system.cpp   (c++17 also works)
// Run:      ./banking
// ─────────────────────────────────────────────────────────────────────────────

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <limits>
#include <chrono>
#include <ctime>

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static string currentTimestamp()
{
    auto now  = chrono::system_clock::now();
    time_t t  = chrono::system_clock::to_time_t(now);
    tm* tmp   = localtime(&t);
    tm  ts    = tmp ? *tmp : tm{};
    ostringstream oss;
    oss << put_time(&ts, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Flush any leftover characters (including '\n') after a >> read.
static void flushInput()
{
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// Read a validated double >= minVal from stdin; returns false on bad input.
static bool readDouble(double& out, double minVal = 0.0)
{
    if (!(cin >> out) || out < minVal) {
        cin.clear();
        flushInput();
        return false;
    }
    flushInput();
    return true;
}

// Read a validated int from stdin; returns false on bad input.
static bool readInt(int& out)
{
    if (!(cin >> out)) {
        cin.clear();
        flushInput();
        return false;
    }
    flushInput();
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Transaction
// ─────────────────────────────────────────────────────────────────────────────

enum class TransactionType { DEPOSIT, TRANSFER_IN, TRANSFER_OUT };

static string txTypeLabel(TransactionType t)
{
    switch (t) {
        case TransactionType::DEPOSIT:      return "Deposit";
        case TransactionType::TRANSFER_IN:  return "Transfer In";
        case TransactionType::TRANSFER_OUT: return "Transfer Out";
    }
    return "Unknown";
}

struct Transaction {
    TransactionType type;
    double          amount;
    int             otherAccountID; // 0 = not applicable (e.g. initial deposit)
    string          timestamp;

    Transaction(TransactionType t, double amt, int other = 0)
        : type(t), amount(amt), otherAccountID(other),
          timestamp(currentTimestamp())
    {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Account
// ─────────────────────────────────────────────────────────────────────────────

class Account {
public:
    int                  accountID;
    string               name;
    double               balance;
    vector<Transaction>  transactions;

    // Default constructor needed by map
    Account() : accountID(0), balance(0.0) {}

    Account(int id, const string& holderName, double initialBalance)
        : accountID(id), name(holderName), balance(0.0)
    {
        if (initialBalance > 0.0) {
            balance = initialBalance;
            transactions.emplace_back(TransactionType::DEPOSIT, initialBalance);
        }
    }

    // Credit an incoming transfer.
    void credit(double amount, int fromID)
    {
        balance += amount;
        transactions.emplace_back(TransactionType::TRANSFER_IN, amount, fromID);
    }

    // Debit an outgoing transfer. Returns false if funds insufficient.
    bool debit(double amount, int toID)
    {
        if (amount > balance) return false;
        balance -= amount;
        transactions.emplace_back(TransactionType::TRANSFER_OUT, amount, toID);
        return true;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// BankSystem
// ─────────────────────────────────────────────────────────────────────────────

class BankSystem {
public:
    // ── Admin ──────────────────────────────────────────────────────────────

    void createAccount()
    {
        printDivider("Create New Account");

        cout << "  Holder name       : ";
        string name;
        getline(cin, name);
        if (name.empty()) {
            printError("Name cannot be empty.");
            return;
        }

        cout << "  Initial deposit $ : ";
        double initialBalance = 0.0;
        if (!readDouble(initialBalance, 0.0)) {
            printError("Invalid amount. Must be a non-negative number.");
            return;
        }

        int id = nextID_++;
        accounts_.emplace(id, Account(id, name, initialBalance));

        printOk("Account created successfully.");
        cout << "  Account ID : " << id         << "\n"
             << "  Name       : " << name       << "\n"
             << "  Balance    : " << fmtMoney(initialBalance) << "\n";
    }

    void listAllAccounts() const
    {
        printDivider("All Accounts");
        if (accounts_.empty()) {
            cout << "  No accounts found.\n";
            return;
        }
        for (const auto& kv : accounts_) {
            const int      id  = kv.first;
            const Account& acc = kv.second;
            cout << "  ID: " << setw(5) << id
                 << "  |  Name: " << left << setw(24) << acc.name
                 << "  |  Balance: " << right << fmtMoney(acc.balance) << "\n";
        }
    }

    // ── Account-Holder ─────────────────────────────────────────────────────

    void checkBalance(int id) const
    {
        const Account& acc = accounts_.at(id);
        printDivider("Balance");
        cout << "  Account : " << acc.name << "  (ID: " << id << ")\n"
             << "  Balance : " << fmtMoney(acc.balance)        << "\n";
    }

    void transferMoney(int fromID)
    {
        printDivider("Transfer Money");

        cout << "  Recipient account ID : ";
        int toID = 0;
        if (!readInt(toID))  { printError("Invalid account ID."); return; }
        if (toID == fromID)  { printError("Cannot transfer to your own account."); return; }
        if (!exists(toID))   { printError("Account " + to_string(toID) + " not found."); return; }

        cout << "  Amount to transfer $ : ";
        double amount = 0.0;
        if (!readDouble(amount, 0.01)) { printError("Invalid amount. Must be greater than 0."); return; }

        Account& from = accounts_.at(fromID);
        Account& to   = accounts_.at(toID);

        if (!from.debit(amount, toID)) {
            printError("Insufficient funds.  Available: " + fmtMoney(from.balance));
            return;
        }
        to.credit(amount, fromID);

        printOk("Transfer successful.");
        cout << "  Sent      : " << fmtMoney(amount)        << "\n"
             << "  To        : " << to.name
                 << "  (ID: " << toID << ")\n"
             << "  New balance: " << fmtMoney(from.balance) << "\n";
    }

    void showTransactionHistory(int id) const
    {
        const Account& acc = accounts_.at(id);
        printDivider("Transaction History - " + acc.name);

        if (acc.transactions.empty()) {
            cout << "  No transactions recorded.\n";
            return;
        }

        // Header
        cout << "  " << left
             << setw(4)  << "#"
             << setw(16) << "Type"
             << setw(14) << "Amount"
             << setw(16) << "Other Account"
             << "Timestamp\n";
        cout << "  " << string(72, '-') << "\n";

        int i = 1;
        for (const auto& tx : acc.transactions) {
            string other = (tx.otherAccountID != 0)
                               ? "ID " + to_string(tx.otherAccountID)
                               : "-";
            cout << "  " << left
                 << setw(4)  << i++
                 << setw(16) << txTypeLabel(tx.type)
                 << setw(14) << fmtMoney(tx.amount)
                 << setw(16) << other
                 << tx.timestamp << "\n";
        }
    }

    // ── Menus ──────────────────────────────────────────────────────────────

    void adminMenu()
    {
        // Simple PIN guard so casual users can't accidentally modify accounts.
        cout << "\n  Admin PIN : ";
        string pin;
        getline(cin, pin);
        if (pin != ADMIN_PIN_) {
            printError("Incorrect PIN.");
            return;
        }

        int choice = 0;
        while (true) {
            printBox("ADMIN PANEL", {
                "1. Create new account",
                "2. List all accounts",
                "3. Back to main menu"
            });
            cout << "  Choice: ";
            if (!readInt(choice)) { printError("Enter a number."); continue; }

            switch (choice) {
                case 1: createAccount();   break;
                case 2: listAllAccounts(); break;
                case 3: return;
                default: printError("Invalid option."); break;
            }
        }
    }

    void accountLoginMenu()
    {
        printDivider("Account Login");
        cout << "  Account ID : ";
        int id = 0;
        if (!readInt(id)) { printError("Invalid ID."); return; }
        if (!exists(id))  { printError("Account " + to_string(id) + " not found."); return; }

        cout << "  Welcome, " << accounts_.at(id).name << "!\n";

        int choice = 0;
        while (true) {
            printBox("ACCOUNT MENU", {
                "1. Check Balance",
                "2. Transfer Money",
                "3. View Transactions",
                "4. Logout"
            });
            cout << "  Choice: ";
            if (!readInt(choice)) { printError("Enter a number."); continue; }

            switch (choice) {
                case 1: checkBalance(id);           break;
                case 2: transferMoney(id);          break;
                case 3: showTransactionHistory(id); break;
                case 4:
                    printOk("Logged out.");
                    return;
                default: printError("Invalid option."); break;
            }
        }
    }

    void run()
    {
        int choice = 0;
        while (true) {
            printBox("C++ BANKING SYSTEM", {
                "1. Admin Panel",
                "2. Account Login",
                "3. Exit"
            });
            cout << "  Choice: ";
            if (!readInt(choice)) { printError("Enter a number."); continue; }

            switch (choice) {
                case 1: adminMenu();        break;
                case 2: accountLoginMenu(); break;
                case 3:
                    cout << "\n  Goodbye!\n\n";
                    return;
                default: printError("Invalid option."); break;
            }
        }
    }

private:
    map<int, Account> accounts_;
    int               nextID_   = 1001;
    // Change this PIN before deploying in any real environment.
    const string      ADMIN_PIN_ = "admin123";

    // ── Formatting helpers ─────────────────────────────────────────────────

    static string fmtMoney(double amount)
    {
        ostringstream oss;
        oss << "$" << fixed << setprecision(2) << amount;
        return oss.str();
    }

    static void printDivider(const string& title)
    {
        cout << "\n-- " << title << " ";
        int pad = 50 - static_cast<int>(title.size());
        if (pad > 0) cout << string(pad, '-');
        cout << "\n";
    }

    static void printOk(const string& msg)
    {
        cout << "  [OK]    " << msg << "\n";
    }

    static void printError(const string& msg)
    {
        cout << "  [ERROR] " << msg << "\n";
    }

    static void printBox(const string& title, const vector<string>& items)
    {
        const int width = 42;
        string border(width, '=');
        cout << "\n  +" << border << "+\n";

        // Centred title
        int pad = (width - static_cast<int>(title.size())) / 2;
        cout << "  |" << string(pad, ' ') << title
             << string(width - pad - static_cast<int>(title.size()), ' ')
             << "|\n";
        cout << "  +" << border << "+\n";

        for (const auto& item : items) {
            cout << "  |  " << left << setw(width - 2) << item << "|\n";
        }
        cout << "  +" << border << "+\n";
    }

    bool exists(int id) const
    {
        return accounts_.find(id) != accounts_.end();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main()
{
    BankSystem bank;
    bank.run();
    return 0;
}
