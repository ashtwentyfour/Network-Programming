#include "binary_trie.h"
#include "nat_table_entry.h"
#include <vector>
#include <cstdlib>
#include <cmath>




// =========================================================================================================



bool operator<(const NAT& N1, const NAT& N2) {            // compares 2 NAT objects
  return N1.getAddress() < N2.getAddress();
}



long long int convert_to_int(str& dest, int& prefix_length) {    // converts a routing table IP address to int and also returns the prefix length
  long long int address_w = 0;
  if (dest == "") {
    prefix_length = 0;
    return 0;
  }

  str temp;
  str blank = "";
  temp = blank;
  int i = 0;
  std::vector<long long int> net_add_parts;

  while(net_add_parts.size() != 4) {
    while(dest[i] != '.' && i < dest.size()) {
      if(dest[i] == '/') break;
      temp = temp + dest[i];
      i++;
    }
    net_add_parts.push_back(atoi(temp.c_str()));
    i++;
    temp = blank;
  }

  address_w = net_add_parts[0]*pow(2 , 24) + net_add_parts[1]*pow(2 , 16) + net_add_parts[2]*pow(2 , 8) + net_add_parts[3];
  temp = blank;
  while(i != dest.size()) {
    if(dest[i] != '/') temp = temp + dest[i];
    i++;
  }

  prefix_length = atoi(temp.c_str());
  return address_w;

}




long long int convert_to_int(str& dest) {      // converts a PDU destination address to int 
  long long int address_w = 0;
  
  if (dest == "") {
    return 0;
  }
 
  str temp;
  str blank = "";
  temp = blank;
  int i = 0;
  std::vector<long long int> net_add_parts; 
  while(net_add_parts.size() != 4) {
    while(dest[i] != '.' && i < dest.size()) {
      temp = temp + dest[i];
      i++;
    }
    net_add_parts.push_back(atoi(temp.c_str()));
    i++;
    temp = blank;
  }
  address_w = net_add_parts[0]*pow(2 , 24) + net_add_parts[1]*pow(2 , 16) + net_add_parts[2]*pow(2 , 8) + net_add_parts[3];
  return address_w;
}




void Routing::setAddress(str dest, str gateway, str inter) {          // set/initialize the paremeters for a node
  destination_address = dest;
  gateway_address = gateway;
  interface = inter;
} 




bool BinaryTrie::insert(str gateway, str dest, str inter) {         // insert a new node into the trie
  if(size_ == 0) {                                                  // if the size is 0 create a new root node
    root_ = new Routing;
  }
  
  
  int prefix_length = 0;
  long long int address = convert_to_int(dest, prefix_length);        // obtain address value
  
  Routing* temp = root_;                                            
  
  if(prefix_length == 0 && root_ -> left) {                         // if a default address is encountered make the update 
    root_ -> left -> setAddress(dest, gateway, inter);
    default_ = root_ -> left;
    size_++;
    return true;
  }
 
  else if(prefix_length == 0 && !root_ -> left) {
    root_ -> left = new Routing;
    root_ -> left -> setAddress(dest, gateway, inter);
    default_ = root_ -> left;
    size_++;
    return true;
  }
 
  int i;
  for(i = 0; i < prefix_length; i++) {                            // traverse through the trie  
    if(((address >> (31 - i)) & 1) == 1) {
      if(temp -> right == NULL) break;
      temp = temp -> right;
    }
    if(((address >> (31 - i)) & 1) == 0) {
      if(temp -> left == NULL) break;
      temp = temp -> left;
    }
  }
  
  if(i == prefix_length && prefix_length != 0) {
    return false;
  }
  
 
  while(i < prefix_length && prefix_length != 0) {          // add new nodes along the path until depth = prefix length
    if(((address >> (31 - i)) & 1) == 1) {
      temp -> right = new Routing;
      temp -> right -> parent = temp;
      temp -> right -> setBitAddress(temp -> getBitAddress());
      temp = temp -> right;
      temp -> setBitAddress(i , '1');
    }
     
    if(((address >> (31 - i)) & 1) == 0) {
      temp -> left = new Routing;
      temp -> left -> parent = temp;
      temp -> left -> setBitAddress(temp -> getBitAddress());
      temp = temp -> left;
      temp -> setBitAddress(i , '0');
    }
    i++;
  }
     
  temp->setAddress(dest, gateway, inter);    // set node parameters 

  temp -> make_table_entry();                // register it as a table entry
       
  size_++;                                  // increment the size after adding a new node

  return true;           

}



str BinaryTrie::find(str dest, str& inter) const {                   // lookup an address
  long long int address = convert_to_int(dest);                      // get the address value 
  
  Routing* temp = root_;
  Routing* current_table_entry = default_;                           // if no match is found the default entry is used
  
  int i = 0;
  while(1) {                                                         // start at the root 1 - right, 0 - left
    if(((address >> (31 - i)) & 1) == 1) { 
      if(temp -> is_table_entry()) current_table_entry = temp;
      if(temp -> right == NULL) break;                               // break when you hit a NULL pointer
      temp = temp -> right;
    }
    if(((address >> (31 - i)) & 1) == 0) {
      if(temp -> is_table_entry()) current_table_entry = temp;
      if(temp -> left == NULL) break;
      temp = temp -> left;
    }
    i++;
  }
  
  return current_table_entry -> getGateAddress(inter);              // return the gatewau address of the latest node encountered
}




void BinaryTrie::destroy_trie(Routing* p) {          // delete tree recursively
  if(!p) return;
  destroy_trie(p -> left);
  destroy_trie(p -> right);
  delete p;
  return;
}


// =====================================================================================================================

   
    
    
  





