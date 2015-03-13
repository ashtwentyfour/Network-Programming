#include <iostream>
#include <string>

typedef std::string str;


/* template for a binary trie node */

class Routing {
 public:
 Routing() : destination_address("") , gateway_address(""), interface("") {          // constructor
    str s(32 ,'*');
    bit_address = s;
    table_entry = false;
    left = NULL;
    right = NULL;
    parent = NULL;
  }
  
 
  // member functions
  
  str getGateAddress(str& inter) const {inter = interface; return gateway_address;}   
  str getDest() const {return destination_address;}
  void setAddress(str dest, str gatewway, str inter);
  void setBitAddress(int i, char x) {if(i <= (31)) bit_address[i] = x;}
  void setBitAddress(str s) {bit_address = s;}
  str getBitAddress() const {return bit_address;}
  void make_table_entry() {table_entry = true;}                     // if a new routing table entry has been added successfully
  bool is_table_entry() const {return table_entry;}  
  

  // left, right and parent pointers
  
  Routing* left;
  Routing* right;
  Routing* parent;


 private:
 
  // data

  str destination_address;
  str gateway_address;
  str interface;
  str bit_address;
  bool table_entry;

};



long long int convert_to_int(str& dest, int& prefix_length);   // convert address from string to integer form 
long long int convert_to_int(str& dest);                       // for PDUs
