#include "routing.h"


/* Binary Trie Data Structure */

class BinaryTrie {
 public:
 BinaryTrie() : root_(NULL) , default_(NULL), size_(0) {}             // default constructor
 
  // member functions 
 
  bool insert(str gate, str dest, str inter);         // adds a node to the trie
  unsigned int size() const {return size_;}          
  str find(str dest, str& inter) const;
  ~BinaryTrie() {this -> destroy_trie(root_);}       // destructor which frees the dynamically allocated memory
 private:

  // data 

  Routing* root_;
  Routing* default_;              // pointer to the default gateway node
  unsigned int size_;
  void destroy_trie(Routing* p);

};
 
