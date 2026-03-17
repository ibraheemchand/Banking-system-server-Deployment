// main.cpp  –  Simple single-threaded HTTP server using Winsock2 (Windows)
// or POSIX sockets (Linux/macOS).  No external libraries required.

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

#include "Admin.h"
#include "Bank.h"

// ───────────────────────────── helpers ──────────────────────────────────────

// Extract the value for a given key from a flat JSON object (string or number).
static std::string jsonField(const std::string& body, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    auto kp = body.find(needle);
    if (kp == std::string::npos) return {};

    auto cp = body.find(':', kp + needle.size());
    if (cp == std::string::npos) return {};

    size_t vs = cp + 1;
    while (vs < body.size() && (body[vs] == ' ' || body[vs] == '\t')) ++vs;
    if (vs >= body.size()) return {};

    if (body[vs] == '"') {                     // string value
        size_t start = vs + 1;
        size_t end   = body.find('"', start);
        if (end == std::string::npos) return {};
        return body.substr(start, end - start);
    }

    size_t end = vs;                           // numeric value
    while (end < body.size() &&
           (isdigit((unsigned char)body[end]) || body[end] == '.' || body[end] == '-'))
        ++end;
    return body.substr(vs, end - vs);
}

static std::string httpResponse(int                code,
                                const std::string& codeStr,
                                const std::string& contentType,
                                const std::string& body) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << code << " " << codeStr                      << "\r\n"
        << "Content-Type: "   << contentType                           << "\r\n"
        << "Content-Length: " << body.size()                           << "\r\n"
        << "Access-Control-Allow-Origin: *"                            << "\r\n"
        << "Connection: close"                                         << "\r\n"
        << "\r\n"
        << body;
    return oss.str();
}

static std::string jsonOk(const std::string& msg) {
    return "{\"status\":\"ok\",\"message\":\"" + msg + "\"}";
}
static std::string jsonError(const std::string& msg) {
    return "{\"status\":\"error\",\"message\":\"" + msg + "\"}";
}

static std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    return { std::istreambuf_iterator<char>(f),
             std::istreambuf_iterator<char>() };
}

// ─────────────────────────── request parsing ────────────────────────────────

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
};

static HttpRequest parseRequest(const std::string& raw) {
    HttpRequest req;
    std::istringstream ss(raw);
    ss >> req.method >> req.path;
    auto pos = raw.find("\r\n\r\n");
    if (pos != std::string::npos)
        req.body = raw.substr(pos + 4);
    return req;
}

// ──────────────────────────── request handler ───────────────────────────────

static void handleClient(int clientSock, Admin& admin) {
    char buf[16384]{};
    int  received = recv(clientSock, buf, sizeof(buf) - 1, 0);
    if (received <= 0) { closesocket(clientSock); return; }

    HttpRequest req = parseRequest(std::string(buf, received));
    std::string response;

    // GET /  or  /index.html  →  serve the SPA
    if (req.method == "GET" &&
        (req.path == "/" || req.path == "/index.html")) {
        std::string html = readFile("index.html");
        if (html.empty())
            response = httpResponse(404, "Not Found",
                                    "text/plain", "index.html not found");
        else
            response = httpResponse(200, "OK",
                                    "text/html; charset=utf-8", html);

    // GET /accounts  →  return JSON array of all accounts
    } else if (req.method == "GET" && req.path == "/accounts") {
        response = httpResponse(200, "OK",
                                "application/json", admin.getAccountsList());

    // POST /create-account
    } else if (req.method == "POST" && req.path == "/create-account") {
        std::string accNum  = jsonField(req.body, "accountNumber");
        std::string name    = jsonField(req.body, "holderName");
        std::string balStr  = jsonField(req.body, "initialBalance");

        if (accNum.empty() || name.empty() || balStr.empty()) {
            response = httpResponse(400, "Bad Request", "application/json",
                       jsonError("Missing fields: accountNumber, holderName, initialBalance"));
        } else {
            double bal = std::stod(balStr);
            if (admin.createAccount(accNum, name, bal)) {
                response = httpResponse(201, "Created", "application/json",
                           jsonOk("Account " + accNum + " created successfully"));
            } else {
                response = httpResponse(409, "Conflict", "application/json",
                           jsonError("Account number already exists or balance is invalid"));
            }
        }

    // POST /transfer
    } else if (req.method == "POST" && req.path == "/transfer") {
        std::string fromAcc = jsonField(req.body, "fromAccount");
        std::string toAcc   = jsonField(req.body, "toAccount");
        std::string amtStr  = jsonField(req.body, "amount");

        if (fromAcc.empty() || toAcc.empty() || amtStr.empty()) {
            response = httpResponse(400, "Bad Request", "application/json",
                       jsonError("Missing fields: fromAccount, toAccount, amount"));
        } else {
            double amount = std::stod(amtStr);
            if (admin.transferFunds(fromAcc, toAcc, amount)) {
                response = httpResponse(200, "OK", "application/json",
                           jsonOk("Transferred $" + amtStr +
                                  " from " + fromAcc + " to " + toAcc));
            } else {
                response = httpResponse(400, "Bad Request", "application/json",
                           jsonError("Transfer failed: verify account numbers "
                                     "and that sender has sufficient funds"));
            }
        }

    // OPTIONS pre-flight (CORS)
    } else if (req.method == "OPTIONS") {
        response =
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";

    // 404
    } else {
        response = httpResponse(404, "Not Found", "application/json",
                                jsonError("Endpoint not found"));
    }

    send(clientSock, response.c_str(), (int)response.size(), 0);
    closesocket(clientSock);
}

// ────────────────────────────────── main ────────────────────────────────────

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }
#endif

    Bank  bank;
    Admin admin(bank);

    // Pre-seed three demo accounts
    admin.createAccount("ACC001", "Alice Johnson",  5000.00);
    admin.createAccount("ACC002", "Bob Smith",      3200.00);
    admin.createAccount("ACC003", "Carol White",    8750.50);

    const int PORT = 8080;

    int serverSock = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        std::cerr << "socket() failed\n";
        return 1;
    }

    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if (bind(serverSock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind() failed – port " << PORT
                  << " may already be in use.\n";
        return 1;
    }
    listen(serverSock, 32);

    std::cout << "==============================================\n"
              << "   Bank Management System  -  HTTP Server    \n"
              << "==============================================\n"
              << "  Listening on  http://localhost:" << PORT << "\n"
              << "  Open that URL in your browser.\n"
              << "  Press Ctrl+C to stop the server.\n"
              << "==============================================\n";

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t   clientLen = sizeof(clientAddr);
        int clientSock = (int)accept(serverSock,
                                     (sockaddr*)&clientAddr, &clientLen);
        if (clientSock < 0) continue;

        // Handle the request on the same thread (synchronous – sufficient for local use)
        handleClient(clientSock, admin);
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
