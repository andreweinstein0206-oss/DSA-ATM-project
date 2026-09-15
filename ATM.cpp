
//ATM SIMULATION DATA STRUCTURE AND ALGORITHM PROJECT #1 

#include <iostream>
#include <string>
#include <iomanip>
#include <limits>
#include <fstream>

#define MAX 6
#define MAX_USERS 100
using namespace std;

struct Userinfo
{
    string name; 
    int pin, age, balance, withdraw, deposit, bday, contact, Ideposit;

};

class useraccount 
{
   private:
   Userinfo user[MAX_USERS];
   int userCount;
   int locateUser(string name, int pin);
   int balancecheck(int );


\\ hello brynt
