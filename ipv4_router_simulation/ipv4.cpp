#include <iostream>
#include <utility>
#include <vector>
#include <cstdlib>
#include <string>
#include <fstream>
#include <map>
#include "pdu.h"
#include "binary_trie.h"
#include "nat_table_entry.h"



typedef std::map<std::string , std::string> ARP_MAP; 
typedef std::ifstream FILE_INPUT;
typedef std::string str;
typedef std::vector<PDU> VECTOR;
typedef std::ofstream FILE_OUTPUT;
typedef std::map<NAT , NAT> NAT_TABLE;



/* Function which does the routing for with NAT Rules */

void execute_with_nat_rules(PDU& P, const BinaryTrie& IPv4_Trie, ARP_MAP& arp_map, NAT_TABLE& nat_table, const str& NATed_IP, const str& NAted_Interface) {
  P.decrementTTL();                               // decrement the TTL first
   
  if(P.getTTL() <= 0) {                           // if TTL = 0 don't bother proceeding
    std::cout<<P.getSourceAddress()<<":"<<P.getSPN()<<"->"<<
      P.getDestinationAddress()<<":"<<P.getDPN()<<" discarded (TTL expired)"<<std::endl;
    return;
  }
  
  
  if(P.getDestinationAddress() == NATed_IP) {    // apply reverse translation if the PDUs destination is the NATed IP address
    for(NAT_TABLE::iterator itr = nat_table.begin(); itr != nat_table.end(); itr++) {
      if((itr -> second).getpn() == P.getDPN()) {
        P.setDestinationAddress((itr -> first).getAddress()); 
        P.setDPN((itr -> first).getpn()); 
        break;
      }
    }
  }
  
  str inter;                                                            
  str gateway = IPv4_Trie.find(P.getDestinationAddress() , inter);   // routing table lookup using the binary trie
  
  if(gateway == "0.0.0.0") {                                         // default gateway indicating a direct connection
    std::cout<<P.getSourceAddress()<<":"<<P.getSPN()<<"->"<<P.getDestinationAddress()<<":"<<
      P.getDPN()<<" directly connected ("<<inter<<"-"<<arp_map[P.getDestinationAddress()]<<") ttl "<<P.getTTL()<<std::endl;
    return;
  }
  
  else if(gateway != "0.0.0.0" && inter == NAted_Interface) {        // if the interface is the NATed interface (ppp0 in this case)
    NAT ORIG(P.getSourceAddress(), P.getSPN());                      // create a new key for the NAT Table
    NAT TRANS(NATed_IP, P.getSPN());                                 // create corresponding translated address + port number pair 
 
    NAT_TABLE::iterator itr_2;                                       // move through the table to see if a translated address is already using the same port number
    for(itr_2 = nat_table.begin(); itr_2 != nat_table.end() && nat_table.size() > 0; itr_2++) {
      if((itr_2 -> second).getpn() == P.getSPN()) TRANS.setpn(P.getSPN() + 1);     // choose a different port number 
    }

    nat_table.insert(std::make_pair<NAT , NAT> (ORIG , TRANS));                      // add the address and it's translated address to the NAT table
   
    std::cout<<TRANS.getAddress()<<":"<<TRANS.getpn()<<"->"<<P.getDestinationAddress()<<":"<<
      P.getDPN()<<" via "<<gateway<<"(ppp0) ttl "<<P.getTTL()<<std::endl;
  
    return;
  }
  
  else {                        
    std::cout<<P.getSourceAddress()<<":"<<P.getSPN()<<"->"<<P.getDestinationAddress()<<":"<<
      P.getDPN()<<" via "<<gateway<<"("<<inter<<"-"<<arp_map[gateway]<<") ttl "<<P.getTTL()<<std::endl;
    return;
  }


}



/* Function for regular routing (No NAT) */

void execute(PDU& P, const BinaryTrie& IPv4_Trie, ARP_MAP& arp_map) {
  P.decrementTTL();      // first decrement the TTL of the PDU

  if(P.getTTL() <= 0) {   // proceed only if the TTL > 0
    std::cout<<P.getSourceAddress()<<":"<<P.getSPN()<<"->"<<
    P.getDestinationAddress()<<":"<<P.getDPN()<<" discarded (TTL expired)"<<std::endl;    
    return;
  }

  str inter;
  str gateway = IPv4_Trie.find(P.getDestinationAddress() , inter);   // binary trie lookup

  if(gateway == "0.0.0.0") {          // if default gateway -> directly connected
    std::cout<<P.getSourceAddress()<<":"<<P.getSPN()<<"->"<<P.getDestinationAddress()<<":"<<
      P.getDPN()<<" directly connected ("<<inter<<"-"<<arp_map[P.getDestinationAddress()]<<") ttl "<<P.getTTL()<<std::endl;    
    return;
  }

  else if(gateway != "0.0.0.0" && inter == "ppp0") {      
    std::cout<<P.getSourceAddress()<<":"<<P.getSPN()<<"->"<<P.getDestinationAddress()<<":"<<
      P.getDPN()<<" via "<<gateway<<"(ppp0) ttl "<<P.getTTL()<<std::endl;
    return; 
 }

 else {
   std::cout<<P.getSourceAddress()<<":"<<P.getSPN()<<"->"<<P.getDestinationAddress()<<":"<<
   P.getDPN()<<" via "<<gateway<<"("<<inter<<"-"<<arp_map[gateway]<<") ttl "<<P.getTTL()<<std::endl;
   return;
 }

}
  
  

/* This function builds a binary trie by using the routing table entries */

void read_routing_table(BinaryTrie& IPv4_Trie, FILE_INPUT& routing_table_input) {
  str dest_address;
  str interface;
  str gateway;
  bool success = false;
  while(routing_table_input >> dest_address) {              // take the input values from the file
    routing_table_input >> gateway >> interface;
    success = IPv4_Trie.insert(gateway, dest_address, interface);    // adding a new node to the tree
    if(!success) {
      std::cerr<<"Error adding node with destination address: "<<dest_address<<std::endl;            // indicate error if the node cannot be added to the trie
    }
  }
}  



/* This function uses a map to create an ARP table */

void create_arp_table(ARP_MAP& arp_map, FILE_INPUT& arp_input) {
  str address;
  str layer_2_address;
  while(arp_input >> address) {
    arp_input >> layer_2_address;
    arp_map[address] = layer_2_address;                // adding a new key to the ARP map
  }
  return;
}



/* Function to read all the pdus into a vector - (for non-NAT part only) */

void read_pdus(VECTOR& pdus, FILE_INPUT& pdu_input) {
  str inter, source_add, destination_add;
  double prot_number;
  int ttl, spn, dpn;
  while(pdu_input >> inter) {
    pdu_input >> source_add >> destination_add >> prot_number >> ttl >> spn >> dpn;
    PDU Packet(inter, source_add, destination_add, prot_number, ttl, spn, dpn);      // constructor creates a new PDU object 
    pdus.push_back(Packet);                                                          // inserted into the vector
  }
  return;
}



int main(int argc , char* argv[]) {
  
  
  if(argv[1] && atoi(argv[1]) == 1) {            // running part 1 where NAT rules are absent
   
   FILE_INPUT arp_input("arp.txt");              // streams to read in ARP, PDU and ROUTING table data 
   FILE_INPUT pdu_input("pdus.txt");
   FILE_INPUT routing_table_input("routes.txt");
   
   if(!arp_input || !pdu_input || !routing_table_input) {
     std::cerr<<"Bad Input"<<std::endl;
     exit(0);
   }
  
   ARP_MAP arp_map;                              //creating ARP map
   create_arp_table(arp_map , arp_input);
  
   BinaryTrie IPv4_Trie;                         // building binary trie using the routing table data
   read_routing_table(IPv4_Trie , routing_table_input);
   
   VECTOR pdus;                                  // creating a vector of pdus to be processed
   read_pdus(pdus, pdu_input);
  
   for(int i = 0; i < pdus.size(); i++) execute(pdus[i], IPv4_Trie, arp_map);   // calling the function which carries out the routing
 
 
 }
 
  else {                                        // part 2 with NAT rules
    
    FILE_INPUT arp_input("arp.txt");            // file input using ifstream
    FILE_INPUT pdu_input("pdus.txt");
    FILE_INPUT routing_table_input("routes.txt");
    FILE_INPUT nat("nat.txt");
    
    if(!arp_input || !pdu_input || !routing_table_input || !nat) {
      std::cerr<<"Bad Input"<<std::endl;
      exit(0);
    }
     
    ARP_MAP arp_map;                          //create ARP map
    create_arp_table(arp_map , arp_input);

    BinaryTrie IPv4_Trie;                     // create binary trie
    read_routing_table(IPv4_Trie , routing_table_input);

    NAT_TABLE nat_table;                      // NAT table (map) of address -> translated address                     
    str NAted_Interface;                      // NATed interface (read from the nat.txt file)
    str NAted_IP;                             // NATed IP address
    nat >> NAted_Interface >> NAted_IP;

    str inter, source_add, destination_add;  // beginning the process where PDUs are read from the file one by one
    double prot_number;
    int ttl, spn, dpn;
   
    while(std::cin >> inter) {              
      std::cin >> source_add >> destination_add >> prot_number >> ttl >> spn >> dpn;
      PDU P(inter, source_add, destination_add, prot_number, ttl, spn, dpn);
      execute_with_nat_rules(P , IPv4_Trie , arp_map, nat_table, NAted_IP, NAted_Interface);   // pass the pdu to the function which carries out routing 
    }  
      
  }
      
  return 0;

}
