#include <iostream>


typedef std::string str;


/* NAT table entry */

class NAT {
 public:
 NAT(str address , long int pn) : address(address), pn(pn) {}       // constructor
  
  // member functions
  
 str getAddress() const {return address;}
 long int getpn() const {return pn;}
 void setpn (long int x) {pn = x;}
 
 private:
 
 // data 
  
  str address;
  long int pn;     // port number

};


bool operator<(const NAT& N1, const NAT& N2);         // overloading the < operator for the map 
