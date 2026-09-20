#include <iostream>
#include <string>
#include <iomanip>
#include <limits>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cctype>


#ifdef _WIN32
#define CLEAR "cls"
#include <windows.h>
#include <tlhelp32.h>
#else
#define CLEAR "clear"
#include <sys/stat.h>
#endif

using namespace std;

const int MAX_USERS = 100;
const int PIN_SHIFT = 3;
const string CSV_FILENAME = "accountinfo.csv";


//  HELPERS

void printCentered(const string& text) {
    int width = 120;
    int len = static_cast<int>(text.length());
    int spaces = (width - len) / 2;

    if (spaces > 0) {
        cout << string(spaces, ' ');
    }
    cout << text << "\n";
}

void printCenteredNoNewline(const string& text) {
    int width = 104;
    int spaces = (width - static_cast<int>(text.length())) / 2;
    if (spaces > 0) cout << string(spaces, ' ');
    cout << text;
}

void verticalCenter(int lines) {
    for (int i = 0; i < lines; i++) {
        cout << "\n";
    }
}

void clearScreen() {
    system(CLEAR);
}


// SECTION 1: STRUCTURES

struct Transaction {
    string type;          //withdraw, deposit, tranfer
    float amount;
    float balanceAfter;
    Transaction* next;
};

//One ATM account 
struct Userinfo {
    string accNum;           
    string name;
    string birthday;            
    string contactNo;
    string pinCode;             
    float balance;
    Transaction* historyHead;   
    bool active;                 

    Userinfo() : balance(0), historyHead(nullptr), active(false) {}
};


class UserAccountList {
private:

    Userinfo users[MAX_USERS];
    int userCount;

public:
    UserAccountList() : userCount(0) {}

    ~UserAccountList() {
        for (int i = 0; i < MAX_USERS; i++) {
            Transaction* current = users[i].historyHead;
            while (current != nullptr) {
                Transaction* toDelete = current;
                current = current->next;
                delete toDelete;
            }
        }
    }

    int searchByAccNum(const string& accNum) const {

        for (int i = 0; i < userCount; i++) {
            if (users[i].active && users[i].accNum == accNum) {
                return i;
            }
        }
        return -1;
    }

    int searchByContact(const string& contactNo) const {
        for (int i = 0; i < userCount; i++) {
            if (users[i].active && users[i].contactNo == contactNo) {
                return i;
            }
        }
        return -1;
    }

    bool accountNumberExists(const string& accNum) const {
        return searchByAccNum(accNum) != -1;
    }

    bool insertUser(const Userinfo& newUser) {
        if (userCount >= MAX_USERS) {
            return false;  // array is full
        }
        users[userCount] = newUser;
        users[userCount].active = true;
        userCount++;
        return true;
    }

    bool updateBalance(const string& accNum, float newBalance) {
        int idx = searchByAccNum(accNum);
        if (idx == -1) return false;
        users[idx].balance = newBalance;
        return true;
    }

    bool updatePin(const string& accNum, const string& newEncryptedPin) {
        int idx = searchByAccNum(accNum);
        if (idx == -1) return false;
        users[idx].pinCode = newEncryptedPin;
        return true;
    }

    void addTransaction(const string& accNum, const string& type, float amount) {
        int idx = searchByAccNum(accNum);
        if (idx == -1) return;

        Transaction* newNode = new Transaction;
        newNode->type = type;
        newNode->amount = amount;
        newNode->balanceAfter = users[idx].balance;
        newNode->next = users[idx].historyHead;
        users[idx].historyHead = newNode;
    }

    void printHistory(const string& accNum) const {
        int idx = searchByAccNum(accNum);
        if (idx == -1) {
            cout << "Account not found.\n";
            return;
        }

        Transaction* current = users[idx].historyHead;
        if (current == nullptr) {
            cout << "No transactions yet.\n";
            return;
        }

        cout << fixed << setprecision(2);
        cout << "----- Transaction History -----\n";
        while (current != nullptr) {
            cout << setw(15) << left << current->type
                 << "Amount: " << setw(10) << current->amount
                 << "Balance after: " << current->balanceAfter << "\n";
            current = current->next;
        }
    }

    int getUserCount() const { return userCount; }
    Userinfo& getUserAt(int index) { return users[index]; }
    const Userinfo& getUserAt(int index) const { return users[index]; }
};

// SECTION 2: PIN ENCRYPTION


string encryptPin(const string& plainPin) {
    string result = plainPin;
    for (char& c : result) {
        if (isdigit(static_cast<unsigned char>(c))) {
            int d = (c - '0' + PIN_SHIFT) % 10;
            c = static_cast<char>('0' + d);
        }
    }
    return result;
}

string decryptPin(const string& encryptedPin) {
    string result = encryptedPin;
    for (char& c : result) {
        if (isdigit(static_cast<unsigned char>(c))) {
            int d = (c - '0' - PIN_SHIFT + 10) % 10;
            c = static_cast<char>('0' + d);
        }
    }
    return result;
}


// SECTION 3: SAFE INPUT HELPERS


void clearInputBuffer() {
    cin.clear();
    cin.ignore((numeric_limits<streamsize>::max)(), '\n');
}

void readLine(string& out) {
    if (!getline(cin, out)) {
        cout << "\n[Input closed -- exiting.]\n";
        exit(0);
    }
}

void exitIfInputClosed() {
    if (cin.eof()) {
        cout << "\n[Input closed -- exiting.]\n";
        exit(0);
    }
}

// SECTION 4: ATM CARD (USB FLASH DRIVE) SIMULATION

string findUSBDrive() {
#ifdef _WIN32

    char drive[4] = "A:\\";
    for (char letter = 'C'; letter <= 'Z'; letter++) {
        drive[0] = letter;
        if (GetDriveTypeA(drive) == DRIVE_REMOVABLE) {
        return string(1, letter) + ":\\";
        }
    }
    return "";
#else
    struct stat info;
    if (stat("./usb_card", &info) == 0 && (info.st_mode & S_IFDIR)) {
    return "./usb_card/";
    }
    return "";
#endif
}

bool isCardInserted() {
    return !findUSBDrive().empty();
}

void waitForCardRemoval() {
    while (isCardInserted()) {
        cout << "\n";
        printCentered("Please remove your card, then press ENTER...");
        string dummy;
        readLine(dummy);
    }
}

bool cardFileExists() {
    string path = findUSBDrive() + "card.dat";
    ifstream file(path);
    return file.good();
}

bool writeCardFile(const string& accNum, const string& encryptedPin) {
    string path = findUSBDrive() + "card.dat";
    ofstream file(path);
    if (!file) return false;
    file << accNum << "\n" << encryptedPin << "\n";
    return true;  // the ofstream destructor flushes and closes the file
}

bool readCardFile(string& accNum, string& encryptedPin) {
    string path = findUSBDrive() + "card.dat";
    ifstream file(path);
    if (!file) return false;
    getline(file, accNum);
    getline(file, encryptedPin);
    return true;
}


// SECTION 5: CSV PERSISTENCE


bool loadFromCSV(UserAccountList& accounts, const string& filename = CSV_FILENAME) {
    ifstream file(filename);
    if (!file) {
        return false;  
    }

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string name, birthday, contactNo, accNum, pinCode, balanceStr;

        getline(ss, name, ',');
        getline(ss, birthday, ',');
        getline(ss, contactNo, ',');
        getline(ss, accNum, ',');
        getline(ss, pinCode, ',');
        getline(ss, balanceStr, ',');

        Userinfo u;
        u.name = name;
        u.birthday = birthday;
        u.contactNo = contactNo;
        u.accNum = accNum;
        u.pinCode = pinCode;        
        u.balance = stof(balanceStr);
        accounts.insertUser(u);

    }
    return true;
}

bool saveToCSV(const UserAccountList& accounts, const string& filename = CSV_FILENAME) {
    ofstream file(filename);
    if (!file) return false;

    for (int i = 0; i < accounts.getUserCount(); i++) {
        const Userinfo& u = accounts.getUserAt(i);
        if (!u.active) continue;
        file << u.name << ","
        << u.birthday << ","
        << u.contactNo << ","
        << u.accNum << ","
        << u.pinCode << ","
        << u.balance << "\n";
    }
    return true;
}


// SECTION 6: REGISTRATION MODULE

bool runRegistration(UserAccountList& accounts) {
    clearScreen();
    verticalCenter(5);
    Userinfo newUser;

    printCentered("=== REGISTRATION MODULE ===");
    cout << "\n";
    printCenteredNoNewline("Enter Name: ");
    readLine(newUser.name);

    printCenteredNoNewline("Enter Birthday (YYYY-MM-DD): ");
    readLine(newUser.birthday);

    // ---- Contact number: must not already belong to another account ----
    while (true) {
        printCenteredNoNewline("Enter Contact Number: ");
        readLine(newUser.contactNo);
        if (accounts.searchByContact(newUser.contactNo) == -1) {
            break;
        }
        printCentered("Contact no. Exist, please try again.");
    }

    // ---- 5-digit account number, checked for collisions ----
    while (true) {
        printCenteredNoNewline("Add 5-digit Account Number: ");
        readLine(newUser.accNum);
        if (newUser.accNum.length() != 5) {
            printCentered("Account number must be exactly 5 digits.");
            continue;
        }
        if (!accounts.accountNumberExists(newUser.accNum)) {
            break;
        }
        printCentered("Acc no. Exist, please try again.");
    }

    // ---- PIN entry + confirmation, up to 3 attempts ----
    string pin, confirmPin;
    bool pinConfirmed = false;
    for (int attempt = 1; attempt <= 3; attempt++) {
        printCenteredNoNewline("Input 4-6 digit PIN Code: ");
        readLine(pin);
        printCenteredNoNewline("Confirm PIN Code: ");
        readLine(confirmPin);

        if (pin == confirmPin && pin.length() >= 4 && pin.length() <= 6) {
            pinConfirmed = true;
            break;
        }
        printCentered(to_string(attempt) + " attempt failed.");
    }

    if (!pinConfirmed) {
        printCentered("Registration cancelled. Returning to main menu.");
        return false;
    }
    newUser.pinCode = encryptPin(pin);  

    // ---- Initial deposit: must be 5000 or more ----
    float deposit;
    while (true) {
        printCenteredNoNewline("Enter deposit (initial is 5000): ");
        cin >> deposit;
        exitIfInputClosed();
        if (cin.fail()) {
            clearInputBuffer();
            printCentered("Invalid amount, must be 5000 and above.");
            continue;
        }
        clearInputBuffer();
        if (deposit >= 5000) {
            break;
        }
        printCentered("INVALID, must be 5000 and above. Enter to try again.");
    }
    newUser.balance = deposit;

    // 
    accounts.insertUser(newUser);
    saveToCSV(accounts);
    bool cardWritten = writeCardFile(newUser.accNum, newUser.pinCode);

    cout << "\n";
    printCentered("Successfully added your initial deposit balance.");
    cout << "\n";
    printCentered("You are now successfully registered!");
    cout << "\n\n";

    printCentered("Name: " + newUser.name);
    printCentered("Birthdate: " + newUser.birthday);
    printCentered("Contact no.: " + newUser.contactNo);
    printCentered("Account Number: " + newUser.accNum);
    printCentered("Pin code: (hidden)");
    ostringstream balanceText;
    balanceText << fixed << setprecision(2) << newUser.balance;
    printCentered("Current Balance: " + balanceText.str());

    if (!cardWritten) {
        cout << "\n";
        printCentered("WARNING: could not write card.dat to the USB drive.");
        printCentered("Your account was saved, but you'll need to reinsert the");
        printCentered("drive before you can log in with it.");
    }

    cout << "\n";
    printCenteredNoNewline("Press ENTER to go back to the main menu...");
    cin.get();
    return true;
}

// SECTION 7: TRANSACTION MODULE


#ifdef _WIN32
void closeConsoleWindow() {
    HWND console = GetConsoleWindow();
    if (console != NULL) {
        PostMessage(console, WM_CLOSE, 0, 0);
    }
}
#endif

#ifdef _WIN32
void closeParentShell() {
    DWORD myPid = GetCurrentProcessId();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    DWORD parentPid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (pe.th32ProcessID == myPid) {
                parentPid = pe.th32ParentProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }


    if (parentPid != 0 && Process32FirstW(snap, &pe)) {
        do {
            if (pe.th32ProcessID == parentPid) {
                bool isShell = _wcsicmp(pe.szExeFile, L"cmd.exe") == 0 ||
                        _wcsicmp(pe.szExeFile, L"powershell.exe") == 0 ||
                        _wcsicmp(pe.szExeFile, L"pwsh.exe") == 0;
                        if (isShell) {
                        HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, parentPid);
                        if (h != NULL) {
                        TerminateProcess(h, 0);
                        CloseHandle(h);
                    }
                }
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
}
#endif

void closeProgram() {
    this_thread::sleep_for(chrono::seconds(3));
#ifdef _WIN32
    closeParentShell();     
    closeConsoleWindow();  
#endif
    exit(0);
}

void closingMessage() {
    cout << "\nThankyou for transacting with us!\n";
    cout << "Automatically closing the program in 3 seconds...\n";
    closeProgram();
}


bool askAnotherTransaction() {
    cout << "\n1. Do another transaction\n2. Exit and eject card\n> ";
    int choice;
    cin >> choice;
    exitIfInputClosed();
    clearInputBuffer();
    return choice == 1;
}

void showBalance(UserAccountList& accounts, const string& accNum) {
    int idx = accounts.searchByAccNum(accNum);
    cout << fixed << setprecision(2);
    cout << "\nAvailable balance: " << accounts.getUserAt(idx).balance << "\n";
}

// BALANCE INQUIRY
void doBalanceInquiry(UserAccountList& accounts, const string& accNum) {
    cout << "\n--- Balance Inquiry ---\n";
    cout << "1. Show available balance\n2. Show transaction history\n> ";
    int choice;
    cin >> choice;
    exitIfInputClosed();
    clearInputBuffer();
    if (choice == 1) {
        showBalance(accounts, accNum);
    } else {
        accounts.printHistory(accNum);
    }
}

// WITHDRAW
void doWithdraw(UserAccountList& accounts, const string& accNum) {
    while (true) {
        cout << "\n**Enter amount to withdraw: ";
        float amount;
        cin >> amount;
        exitIfInputClosed();
        if (cin.fail() || amount <= 0) {
            clearInputBuffer();
            cout << "\"INVALID AMOUNT, PLEASE TRY AGAIN\"\n";
            continue;
        }
        clearInputBuffer();

        int idx = accounts.searchByAccNum(accNum);
        float balance = accounts.getUserAt(idx).balance;

        
        if (amount <= balance) {
            accounts.updateBalance(accNum, balance - amount);
            accounts.addTransaction(accNum, "Withdraw", amount);
            saveToCSV(accounts);
            cout << "Withdraw successful.\n";
            showBalance(accounts, accNum);
            return;
        }

        // entered amount > balance > insufficient funds
        cout << "Your desired amount exceeds your balance...\n";
        showBalance(accounts, accNum);
        cout << "1. Try again\n2. Back to Main Menu\n3. Eject card\n> ";
        int choice;
        cin >> choice;
        exitIfInputClosed();
        clearInputBuffer();
        if (choice == 1) continue;   
        if (choice == 2) return;    
        closingMessage();
        exit(0);                     
    }
}

// DEPOSIT
void doDeposit(UserAccountList& accounts, const string& accNum) {
    while (true) {
        cout << "\nEnter Amount to deposit: ";
        float amount;
        cin >> amount;
        exitIfInputClosed();
        if (cin.fail() || amount <= 0) {
            clearInputBuffer();
            cout << "\"INVALID AMOUNT, PLEASE TRY AGAIN\"\n";
            continue;
        }
        clearInputBuffer();

        int idx = accounts.searchByAccNum(accNum);
        float balance = accounts.getUserAt(idx).balance;
        accounts.updateBalance(accNum, balance + amount);
        accounts.addTransaction(accNum, "Deposit", amount);
        saveToCSV(accounts);
        cout << "Deposit accepted to balance.\n";
        showBalance(accounts, accNum);
        return;
    }
}

// TRANSFER
void doTransfer(UserAccountList& accounts, const string& accNum) {
    while (true) {
        cout << "\nEnter Destination Acc no.: ";
        string destAcc;
        readLine(destAcc);

        if (destAcc == accNum) {
            cout << "You cannot transfer to your own account.\n";
            continue;
        }
        if (accounts.searchByAccNum(destAcc) == -1) {
            cout << "\"Account is not exist, please try again.\"\n";
            continue;  // loop back to "Enter Destination Acc no."
        }

        while (true) {
            cout << "Enter Amount: ";
            float amount;
            cin >> amount;
            exitIfInputClosed();
            if (cin.fail() || amount <= 0) {
                clearInputBuffer();
                cout << "\"INVALID AMOUNT, PLEASE TRY AGAIN\"\n";
                continue;
            }
            clearInputBuffer();

            int srcIdx = accounts.searchByAccNum(accNum);
            float srcBalance = accounts.getUserAt(srcIdx).balance;

            // Transfer amount <= balance -> proceed
            if (amount <= srcBalance) {
                int destIdx = accounts.searchByAccNum(destAcc);
                float destBalance = accounts.getUserAt(destIdx).balance;
                accounts.updateBalance(accNum, srcBalance - amount);
                accounts.updateBalance(destAcc, destBalance + amount);
                accounts.addTransaction(accNum, "Transfer Out", amount);
                accounts.addTransaction(destAcc, "Transfer In", amount);
                saveToCSV(accounts);
                cout << "Transfer successful.\n";
                showBalance(accounts, accNum);
                return;
            }

            // Transfer amount > balance -> insufficient funds
            cout << "\"Your currently balance is too low for your desired "
            << "amount, please try again\"\n";
            showBalance(accounts, accNum);
        } 
    }      
}         

// CHANGE PIN

void doChangePin(UserAccountList& accounts, const string& accNum) {
    bool currentPinOk = false;
    for (int attempt = 1; attempt <= 3; attempt++) {
        cout << "\nEnter Current PIN: ";
        string entered;
        readLine(entered);
        int idx = accounts.searchByAccNum(accNum);
        if (encryptPin(entered) == accounts.getUserAt(idx).pinCode) {
            currentPinOk = true;
            break;
        }
        cout << "INCORRECT PIN\n";
    }
    if (!currentPinOk) {
        cout << "\"3 Attempt failed, PLEASE TRY AGAIN\"\n";
        return;  
    }

    string newPin, confirmPin;
    cout << "ENTER NEW PIN: ";
    readLine(newPin);
    cout << "CONFIRM PIN: ";
    readLine(confirmPin);

    if (newPin != confirmPin) {
        cout << "Your two new PIN entries didn't match.\n";
        return;
    }

    string encryptedNew = encryptPin(newPin);
    accounts.updatePin(accNum, encryptedNew);
    saveToCSV(accounts);
    writeCardFile(accNum, encryptedNew);  
    cout << "PIN CHANGED SUCCESSFULLY! Thankyou for using!\n";
}

//  Main Transaction Module 
void runTransactionModule(UserAccountList& accounts, const string& accNum) {
    while (true) {
        clearScreen();
        verticalCenter(5);
        printCentered("======== TRANSACTION MODULE ========");
        printCentered( "\n");
        printCentered("1. Balance Inquiry"); 
        printCentered("2. Withdraw");
        printCentered("3. Deposit");
        printCentered("4. Transfer");
        printCentered("5. Change PIN");
        printCentered("6. Exit and eject card");
        printCentered( "\n");
        printCenteredNoNewline("Enter choice: ");

        int choice;
        cin >> choice;
        exitIfInputClosed();
        clearInputBuffer();
        switch (choice) {
            case 1: doBalanceInquiry(accounts, accNum); break;
            case 2: doWithdraw(accounts, accNum); break;
            case 3: doDeposit(accounts, accNum); break;
            case 4: doTransfer(accounts, accNum); break;
            case 5: doChangePin(accounts, accNum); break;
            case 6:
                closingMessage();
                return;
            default:
                cout << "Invalid option.\n";
                this_thread::sleep_for(chrono::seconds(1));
                continue;
        }

        if (!askAnotherTransaction()) {
            closingMessage();
            return;
        }
    }
}


// SECTION 8: MAIN PROGRAM FLOW


int main() {
    UserAccountList accounts;
    loadFromCSV(accounts);

    while (true) {
        // ---------------- "Please insert your card...." ----------------
        while (!isCardInserted()) {
            clearScreen();
            verticalCenter(5);
            printCentered("=============== WELCOME TO A&B CAPITAL BANK ===============");
            cout << "\n\n";
            printCentered("Please insert your ATM card....");
            cout << "\n";
            printCentered("(Press ENTER once the flash drive is plugged in)");
            printCentered("(type 3 to exit)");
            printCentered( "\n");
            cout << string(58, ' ');
            string input;
            readLine (input);
            if (input == "3") { 
                printCentered("Thankyou for using A&B CAPITAL! exitt program in 3 seconds");
                closeProgram();
            }
        }


        // ---- (1) UNREGISTERED CARD ----
        if (!cardFileExists()) {
            clearScreen();
            verticalCenter(3);
            printCentered("=== (1) UNREGISTERED CARD ===");
            runRegistration(accounts);
            waitForCardRemoval();
            continue;
        }

        // ---- (2) REGISTERED CARD ----
        string cardAccNum, cardEncPin;
        readCardFile(cardAccNum, cardEncPin);

        int idx = accounts.searchByAccNum(cardAccNum);
        if (idx == -1 || accounts.getUserAt(idx).pinCode != cardEncPin) {
            clearScreen();
            verticalCenter(5);
            printCentered("\"PIN NOT LOCATED\"");
            waitForCardRemoval();
            continue;
        }

        clearScreen();
        verticalCenter(5);
        printCentered("=== (2) REGISTERED CARD ===");
        cout << "\n";

        bool pinOk = false;
        for (int attempt = 1; attempt <= 3; attempt++) {
            verticalCenter(3);
            printCenteredNoNewline("INPUT PIN: ");
            string entered;
            readLine(entered);
            if (encryptPin(entered) == accounts.getUserAt(idx).pinCode) {
                pinOk = true;
                break;
            }
            verticalCenter(2);
            printCentered("********** INVALID PIN **********");
        }

        if (!pinOk) {
            printCentered("more than 3 attempts.\n");
            printCentered("Ejecting card, please try again for security purposes.\n");
            closeProgram();
            return 0;  // back to "please insert your card"
        }

        cout << "\"VALID PIN\"\n";
        runTransactionModule(accounts, cardAccNum);
        return 0; 
    }   
}      