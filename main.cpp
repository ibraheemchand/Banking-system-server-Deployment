// main.cpp – Simple synchronous HTTP server (Winsock2 on Windows).
// Session tokens are stored in an in-memory map.

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
   typedef int socklen_t;
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <unistd.h>
#  define closesocket close
#endif

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <map>
#include <cstdlib>
#include <ctime>

#include "Admin.h"
#include "Bank.h"

// ─────────────────────────── Session management ─────────────────────────────

struct Session {
    std::string role;        // "admin" | "user"
    std::string accountNum;  // non-empty only for user sessions
};

static std::map<std::string, Session> sessions;

static std::string generateToken() {
    static bool seeded = false;
    if (!seeded) { srand((unsigned int)time(NULL)); seeded = true; }
    const char hex[] = "0123456789abcdef";
    std::string tok;
    tok.reserve(32);
    for (int i = 0; i < 32; ++i)
        tok += hex[rand() % 16];
    return tok;
}

static bool isAdminToken(const std::string& token) {
    auto it = sessions.find(token);
    return it != sessions.end() && it->second.role == "admin";
}

// Returns empty string if not a valid user token.
static std::string tokenToAccount(const std::string& token) {
    auto it = sessions.find(token);
    if (it == sessions.end() || it->second.role != "user") return {};
    return it->second.accountNum;
}

// ──────────────────────────── JSON helpers ───────────────────────────────────

// Escape a string value for embedding in JSON.
static std::string je(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (char c : s) {
        if      (c == '"')  r += "\\\"";
        else if (c == '\\') r += "\\\\";
        else if (c == '\n') r += "\\n";
        else if (c == '\r') r += "\\r";
        else                r += c;
    }
    return r;
}

// Extract a JSON string or number field from a flat object.
static std::string jf(const std::string& body, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    auto kp = body.find(needle);
    if (kp == std::string::npos) return {};
    auto cp = body.find(':', kp + needle.size());
    if (cp == std::string::npos) return {};
    size_t vs = cp + 1;
    while (vs < body.size() && (body[vs] == ' ' || body[vs] == '\t')) ++vs;
    if (vs >= body.size()) return {};
    if (body[vs] == '"') {
        size_t s = vs + 1, e = body.find('"', s);
        if (e == std::string::npos) return {};
        return body.substr(s, e - s);
    }
    size_t e = vs;
    while (e < body.size() &&
           (isdigit((unsigned char)body[e]) || body[e] == '.' || body[e] == '-'))
        ++e;
    return body.substr(vs, e - vs);
}

static std::string jsonOk(const std::string& msg) {
    return "{\"status\":\"ok\",\"message\":\"" + je(msg) + "\"}";
}
static std::string jsonError(const std::string& msg) {
    return "{\"status\":\"error\",\"message\":\"" + je(msg) + "\"}";
}

// ─────────────────────────── HTTP helpers ────────────────────────────────────

static std::string httpResp(int code, const std::string& codeStr,
                             const std::string& ct, const std::string& body) {
    std::ostringstream o;
    o << "HTTP/1.1 " << code << " " << codeStr          << "\r\n"
      << "Content-Type: "   << ct                        << "\r\n"
      << "Content-Length: " << body.size()               << "\r\n"
      << "Access-Control-Allow-Origin: *"                << "\r\n"
      << "Connection: close"                             << "\r\n"
      << "\r\n" << body;
    return o.str();
}

static std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    return { std::istreambuf_iterator<char>(f),
             std::istreambuf_iterator<char>() };
}

// ──────────────────────────── Request parsing ────────────────────────────────

struct Req {
    std::string method;
    std::string path;
    std::string token;  // from "Authorization: Bearer <token>"
    std::string body;
};

static Req parseRequest(const std::string& raw) {
    Req r;
    std::istringstream ss(raw);
    ss >> r.method >> r.path;

    const std::string authKey = "\r\nAuthorization: Bearer ";
    auto ap = raw.find(authKey);
    if (ap != std::string::npos) {
        size_t s = ap + authKey.size();
        size_t e = raw.find("\r\n", s);
        r.token = raw.substr(s, (e == std::string::npos) ? std::string::npos : e - s);
    }

    auto bp = raw.find("\r\n\r\n");
    if (bp != std::string::npos) r.body = raw.substr(bp + 4);
    return r;
}

// ──────────────────────────── Request handler ────────────────────────────────

static void handleClient(int sock, Admin& admin) {
    char buf[16384]{};
    int  n = recv(sock, buf, sizeof(buf) - 1, 0);
    if (n <= 0) { closesocket(sock); return; }

    Req         req  = parseRequest(std::string(buf, n));
    std::string resp;

    // ── Serve frontend ────────────────────────────────────────────────────────
    if (req.method == "GET" &&
        (req.path == "/" || req.path == "/index.html")) {
        std::string html = readFile("index.html");
        resp = html.empty()
               ? httpResp(404, "Not Found",  "text/plain", "index.html not found")
               : httpResp(200, "OK", "text/html; charset=utf-8", html);

    // ── POST /login/admin ─────────────────────────────────────────────────────
    } else if (req.method == "POST" && req.path == "/login/admin") {
        std::string pw = jf(req.body, "password");
        if (admin.validateAdminPassword(pw)) {
            std::string tok = generateToken();
            sessions[tok]   = { "admin", "" };
            resp = httpResp(200, "OK", "application/json",
                            "{\"status\":\"ok\",\"token\":\"" + tok + "\"}");
        } else {
            resp = httpResp(401, "Unauthorized", "application/json",
                            jsonError("Invalid admin password"));
        }

    // ── POST /login/user ──────────────────────────────────────────────────────
    } else if (req.method == "POST" && req.path == "/login/user") {
        std::string accNum = jf(req.body, "accountNumber");
        std::string pw     = jf(req.body, "password");
        if (admin.validateUserCredentials(accNum, pw)) {
            std::string tok   = generateToken();
            sessions[tok]     = { "user", accNum };
            std::string accJs = admin.getUserAccountJson(accNum);
            resp = httpResp(200, "OK", "application/json",
                            "{\"status\":\"ok\",\"token\":\"" + tok +
                            "\",\"account\":" + accJs + "}");
        } else {
            resp = httpResp(401, "Unauthorized", "application/json",
                            jsonError("Invalid account number or password"));
        }

    // ── POST /logout ──────────────────────────────────────────────────────────
    } else if (req.method == "POST" && req.path == "/logout") {
        sessions.erase(req.token);
        resp = httpResp(200, "OK", "application/json", jsonOk("Logged out"));

    // ── GET /accounts  (admin or user) ────────────────────────────────────────
    } else if (req.method == "GET" && req.path == "/accounts") {
        if (!isAdminToken(req.token) && tokenToAccount(req.token).empty()) {
            resp = httpResp(401, "Unauthorized", "application/json",
                            jsonError("Authentication required"));
        } else {
            resp = httpResp(200, "OK", "application/json", admin.getAccountsList());
        }

    // ── POST /create-account  (admin-only) ────────────────────────────────────
    } else if (req.method == "POST" && req.path == "/create-account") {
        if (!isAdminToken(req.token)) {
            resp = httpResp(401, "Unauthorized", "application/json",
                            jsonError("Admin authentication required"));
        } else {
            std::string accNum  = jf(req.body, "accountNumber");
            std::string name    = jf(req.body, "holderName");
            std::string balStr  = jf(req.body, "initialBalance");
            std::string pw      = jf(req.body, "password");

            if (accNum.empty() || name.empty() || balStr.empty() || pw.empty()) {
                resp = httpResp(400, "Bad Request", "application/json",
                                jsonError("Missing fields: accountNumber, holderName, "
                                          "initialBalance, password"));
            } else {
                double bal = std::stod(balStr);
                if (admin.createAccount(accNum, name, bal, pw)) {
                    resp = httpResp(201, "Created", "application/json",
                                    jsonOk("Account " + accNum + " created successfully"));
                } else {
                    resp = httpResp(409, "Conflict", "application/json",
                                    jsonError("Account number already exists or data is invalid"));
                }
            }
        }

    // ── POST /transfer  (admin-only) ──────────────────────────────────────────
    } else if (req.method == "POST" && req.path == "/transfer") {
        if (!isAdminToken(req.token)) {
            resp = httpResp(401, "Unauthorized", "application/json",
                            jsonError("Admin authentication required"));
        } else {
            std::string from   = jf(req.body, "fromAccount");
            std::string to     = jf(req.body, "toAccount");
            std::string amtStr = jf(req.body, "amount");

            if (from.empty() || to.empty() || amtStr.empty()) {
                resp = httpResp(400, "Bad Request", "application/json",
                                jsonError("Missing fields: fromAccount, toAccount, amount"));
            } else {
                double amt = std::stod(amtStr);
                if (admin.transferFunds(from, to, amt)) {
                    resp = httpResp(200, "OK", "application/json",
                                    jsonOk("Transferred $" + amtStr +
                                           " from " + from + " to " + to));
                } else {
                    resp = httpResp(400, "Bad Request", "application/json",
                                    jsonError("Transfer failed: check accounts and balance"));
                }
            }
        }

    // ── POST /change-password  (admin-only) ───────────────────────────────────
    } else if (req.method == "POST" && req.path == "/change-password") {
        if (!isAdminToken(req.token)) {
            resp = httpResp(401, "Unauthorized", "application/json",
                            jsonError("Admin authentication required"));
        } else {
            std::string accNum  = jf(req.body, "accountNumber");
            std::string newPass = jf(req.body, "newPassword");

            if (accNum.empty() || newPass.empty()) {
                resp = httpResp(400, "Bad Request", "application/json",
                                jsonError("Missing fields: accountNumber, newPassword"));
            } else if (admin.changeUserPassword(accNum, newPass)) {
                resp = httpResp(200, "OK", "application/json",
                                jsonOk("Password changed for account " + accNum));
            } else {
                resp = httpResp(404, "Not Found", "application/json",
                                jsonError("Account not found"));
            }
        }

    // ── GET /my-account  (user-only) ──────────────────────────────────────────
    } else if (req.method == "GET" && req.path == "/my-account") {
        std::string accNum = tokenToAccount(req.token);
        if (accNum.empty()) {
            resp = httpResp(401, "Unauthorized", "application/json",
                            jsonError("User authentication required"));
        } else {
            resp = httpResp(200, "OK", "application/json",
                            admin.getUserAccountJson(accNum));
        }

    // ── POST /user-transfer  (user-only) ──────────────────────────────────────
    } else if (req.method == "POST" && req.path == "/user-transfer") {
        std::string fromAcc = tokenToAccount(req.token);
        if (fromAcc.empty()) {
            resp = httpResp(401, "Unauthorized", "application/json",
                            jsonError("User authentication required"));
        } else {
            std::string toAcc  = jf(req.body, "toAccount");
            std::string amtStr = jf(req.body, "amount");

            if (toAcc.empty() || amtStr.empty()) {
                resp = httpResp(400, "Bad Request", "application/json",
                                jsonError("Missing fields: toAccount, amount"));
            } else {
                double amt = std::stod(amtStr);
                if (admin.userTransfer(fromAcc, toAcc, amt)) {
                    resp = httpResp(200, "OK", "application/json",
                                    jsonOk("Transferred $" + amtStr + " to " + toAcc));
                } else {
                    resp = httpResp(400, "Bad Request", "application/json",
                                    jsonError("Transfer failed: check recipient account "
                                              "and your balance"));
                }
            }
        }

    // ── CORS pre-flight ───────────────────────────────────────────────────────
    } else if (req.method == "OPTIONS") {
        resp = "HTTP/1.1 204 No Content\r\n"
               "Access-Control-Allow-Origin: *\r\n"
               "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
               "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
               "Content-Length: 0\r\n"
               "Connection: close\r\n\r\n";

    // ── 404 ───────────────────────────────────────────────────────────────────
    } else {
        resp = httpResp(404, "Not Found", "application/json",
                        jsonError("Endpoint not found"));
    }

    send(sock, resp.c_str(), (int)resp.size(), 0);
    closesocket(sock);
}

// ─────────────────────────────── main ────────────────────────────────────────

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }
#endif

    Bank  bank;
    Admin admin(bank);  // admin password is "admin123"

    // Pre-seed demo accounts (account number / name / balance / password)
    admin.createAccount("ACC001", "Alice Johnson", 5000.00, "alice123");
    admin.createAccount("ACC002", "Bob Smith",     3200.00, "bob123");
    admin.createAccount("ACC003", "Carol White",   8750.50, "carol123");

    const int PORT = 8080;

    int serverSock = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) { std::cerr << "socket() failed\n"; return 1; }

    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if (bind(serverSock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind() failed – port " << PORT << " may be in use.\n";
        return 1;
    }
    listen(serverSock, 32);

    std::cout
        << "==============================================\n"
        << "   Bank Management System  -  HTTP Server    \n"
        << "==============================================\n"
        << "  URL  :  http://localhost:" << PORT           << "\n"
        << "  Admin password  : admin123\n"
        << "  Demo accounts   : ACC001/alice123\n"
        << "                    ACC002/bob123\n"
        << "                    ACC003/carol123\n"
        << "  Press Ctrl+C to stop.\n"
        << "==============================================\n";

    while (true) {
        sockaddr_in ca{};
        socklen_t   cl = sizeof(ca);
        int client = (int)accept(serverSock, (sockaddr*)&ca, &cl);
        if (client < 0) continue;
        handleClient(client, admin);
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
