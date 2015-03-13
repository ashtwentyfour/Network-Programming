#include <iostream>
#include <string>


typedef std::string str;

/* template for a PDU (Protocol Data Unit) object */

class PDU {
 public:
 PDU(str intr, str srce, str dest, double prot, int ttl, int spn, int dpn) : interface(intr), source_address(srce),    //constructor  
 destination_address(dest), protocol_number(prot), TTL(ttl), source_port_number(spn), destination_port_number(dpn) {}
 
 // member functions

 const str& getDestinationAddress() { return destination_address; } 


 const str& getSourceAddress() { return source_address; }
 

 void setDestinationAddress(str s) { destination_address = s; } 
  

 int getTTL() const { return TTL; }
  

 void decrementTTL() { TTL--; }
 

 int getDPN() const {return destination_port_number;}
 

 int getSPN() const {return source_port_number;}
 

 void setDPN(int x) {destination_port_number = x;} 


 private:

 //PDU Data

  str interface;
  str source_address;
  str destination_address;
  double protocol_number;
  int TTL;
  int source_port_number;
  int destination_port_number;

};
  
     
