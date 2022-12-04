#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>

#define STORE_FILE "store/dumpFile"

std::mutex mtx; // mutex for critical section
std::string delimiter = ":"; // used to seperate key and value.

//Class template to implement node
template<typename K, typename V>
class Node {
private:
  /* data */
  K key;
  V value;

public:
  Node(){}
  Node(K k, V v, int );
  ~Node();
  K get_key() const;
  V get_value() const;
  void set_value(V);
  // Linear array to hold pointers to next node of different level
  Node<K, V> **forward; // an array holds next node pointers at different level. for example node->forward[i], node's next on level i.
  int node_level;
};

template<typename K, typename V>
Node<K, V>::Node(const K k, const V v, int level) {
  this->key = k;
  this->value = value;
  this->node_level = level;
  // level + 1, because array index is from 0 - level
  this->forward = new Node<K, V>* [level+1];
  // set forward array with 0(NULL)
  memset(this->forward, 0, sizeof(Node<K, V>*) * (level+1));
};

template<typename K, typename V>
Node<K, V>::~Node() {
  delete []forward;
};

template<typename K, typename V>
K Node<K, V>::get_key() const {
  return this->key;
};

template<typename K, typename V> 
V Node<K, V>::get_value() const {
  return value;
};

template<typename K, typename V> 
void Node<K, V>::set_value(V value) {
    this->value=value;
};

// Class template for Skip List
template<typename K, typename V>
class SkipList {
public:
  SkipList(int);
  ~SkipList();
  int get_random_level(); // deside to insert to how many levels of skip list.
  Node<K, V>* create_node(K, V, int);
  int insert_element(K, V);
  void display_list();
  bool search_element(K);
  void delete_element(K); 
  void dump_file(); // dump to file
  void load_file(); // load data from file
  int size(); // return size of skip list

private:
  void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);
  bool is_valid_string(const std::string& str);

private:
  int _max_level; // Maximum level of the skip list
  int _skip_list_level; // current level of skip list
  Node<K, V> *_header; // pointer of head of skiplist
  std::ofstream _file_writer; // file operator
  std::ifstream _file_reader;
  int _element_count; // skiplist current element count.
};

// create new node
template<typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(const K k, const V v, int level) {
  Node<K, V> *n = new Node<K, V>(k, v, level);
  return n;
}

// Insert given key and value in skip list 
// return 1 means element exists  
// return 0 means insert successfully
/* 
                           +------------+
                           |  insert 50 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |                      insert +----+
level 3         1+-------->10+---------------> | 50 |          70       100
                                               |    |
                                               |    |
level 2         1          10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 1         1    4     10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 0         1    4   9 10         30   40  | 50 |  60      70       100
                                               +----+
*/

template<typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value) {
  mtx.lock(); // when doing search we don't want it be removed or inserted. to avoid data racing. 
  Node<K, V> *current = this->_header;
  // create update array and initialize it
  // update is array which put node that the node-> forward[i] should be operated later
  Node<K, V> *update[_max_level+1];
  memset(update, 0, sizeof(Node<K, V>*) * (_max_level+1));

  // start from highest level of skip list
  for (int i = _skip_list_level; i >= 0; i--) { // 
    while (current->forward[i] != NULL && current->forward[i]->get_key() < key) {
      current = current->forward[i];
    } // go to right on same level
    update[i] = current;
  }

  // reached level 0 and forward pointer to right node, which is desired to insert key.
  current = current->forward[0]; // 60 in the example.

  // if current node have key equal to searched key, we get it
  if (current != NULL && current->get_key() == key) {
    std::cout << "key: " << key << ", exists" << std::endl;
    mtx.unlock();
    return 1;
  }
   // if current is NULL that means we have reached to end of the level 
    // if current's key is not equal to key that means we have to insert node between update[0] and current node .
  if (current == NULL || current->get_key() != key) {
    int random_level = get_random_level(); // Generate a random level for node. How many levels to insert. In case 
    // if random_level > _skip_list_level, initialize update value with pointer to header
    if (random_level > _skip_list_level) {
      for (int i = _skip_list_level; i < random_level+1; i++) {
        update[i] = _header;
      }
      _skip_list_level = random_level; // update skip list level to new level
    }

    // create node with generated random level.
    Node<K, V>* inserted_node = create_node(key, value, random_level);
  
    // insert node
    for (int i = 0; i <= random_level; i++) {
      inserted_node->forward[i] = update[i]->forward[i]; // the next node of inserted_node on each level
      update[i]->forward[i] = inserted_node;
    }
    std::cout << "Successfully inserted key:" << key << "< value:" << value << std::endl;
    _element_count++;
  }
  mtx.unlock();
  return 0;
}


//Delete element from skip list
template<typename K, typename V>
void SkipList<K, V>::delete_element(K key) {
  mtx.lock();
  Node<K, V> *current = this->_header;
  Node<K, V> *update[_max_level+1];
  memset(update, 0, sizeof(Node<K, V>*) * (_max_level+1)); // initiate update with all 0.

  // start from highest level of skip list
  for (int i = _skip_list_level; i >= 0; i--) {
    while (current->forward[i] != NULL && current->forward[i]->get_key() < key) {
      current = current->forward[i];
    }
    update[i] = current; // update[] keep the address of node before delete_element 
  }

  current = current->forward[0]; 
  if (current != NULL && current->get_key() == key) {
    // start for lowest level and delete the current node of each level
    for (int i = 0; i < _skip_list_level; i++) {
       // if at level i, next node is not target node, break the loop. // no target node up this level
      if (update[i]->forward[i] != current)
        break;
      update[i]->forward[i] = current->forward[i];
    }
    
    // Remove levels which have no elements
    while (_skip_list_level > 0 && _header->forward[_skip_list_level] == 0) {
      _skip_list_level--;
    }
    std::cout << "Successfully deleted key" << key << std::endl;
    _element_count--;
  }
  mtx.unlock();
  return;
}

// Search for element in skip list
/*
                           +------------+
                           |  select 60 |
                           +------------+
level 4     +-->1+                                                      100    |--> a column is a node  
                 |
                 |
level 3         1+-------->10+------------------>50+           70       100    |
                                                   |
                                                   |
level 2         1          10         30         50|           70       100    |
                                                   |
                                                   |
level 1         1    4     10         30         50|           70       100    |
                                                   |
                                                   |
level 0         1    4   9 10         30   40    50+-->60      70       100    |
*/
template<typename K, typename V>
bool SkipList<K, V>::search_element(K key) {
  std::cout << "------search_element-------" << std::endl;
  Node<K, V> *current = _header;

  // start from highest level of skip list
  for (int i = _skip_list_level; i >= 0; i--) {
    while (current->forward[i] && current->forward[i]->get_key() < key) {
      current = current->forward[i];
    }
  }
  //reached level 0 and advance pointer to right node, which we search
  current = current->forward[0];

  // if current node have key equal to searched key, we get it
  if (current and current->get_key() == key) {
    std::cout << "Found key: " << key <<", value: " << current->get_value() << std::endl;
    return true;
  }

  std::cout << "Not Found Key: " << key << std::endl;
  return false;
}

// Construct skip list
template<typename K, typename V>
SkipList<K, V>::SkipList(int max_level) {
  this->_max_level = max_level;
  this->_skip_list_level = 0;
  this->_element_count = 0;

  // create header node and initialize key and value to null
  K k;
  V v;
  this->_header = new Node<K, V>(k, v, _max_level);
};

template<typename K, typename V> 
SkipList<K, V>::~SkipList() {

    if (_file_writer.is_open()) {
        _file_writer.close();
    }
    if (_file_reader.is_open()) {
        _file_reader.close();
    }
    Node<K, V>* node = _header->forward[0];
    Node<K, V>* next;
    while(node) {
      next = node->forward[0];
      delete node;
      node = next; 
    }
    delete _header;
}

template<typename K, typename V>
int SkipList<K, V>::get_random_level(){

    int k = 1;
    while (rand() % 2) { // the new node have 0.5 probability to inset to up level
        k++;
    }
    k = (k < _max_level) ? k : _max_level;
    return k;
};

// Display skip list
template<typename K, typename V>
void SkipList<K, V>::display_list() {
  std::cout << "\n******Skip LIst*******" << "\n";
  for (int i = 0; i <= _skip_list_level; i++) {
    Node<K, V> *node = this->_header->forward[i];
    std::cout << "Level" << i << ":";
    while (node != NULL) {
      std::cout << node->get_key() << ":" << node->get_value() << ";";
      node = node->forward[i];
    }
    std::cout << std::endl;
  }
}

// Dump data in memory to file
template<typename K, typename V>
void SkipList<K, V>::dump_file() { // write all nodes on level 0 to file.
  std::cout << "------dump_file------" << std::endl;
  _file_writer.open(STORE_FILE); // std::ofstream
  Node<K, V> *node = this->_header->forward[0];

  while (node != NULL) {
    _file_writer << node->get_key() << ":" << node->get_value() << "\n";
    std::cout << node->get_key() << ":" << node->get_value() << ";\n";
    node = node->forward[0];
  }
  _file_writer.flush();
  _file_writer.close();
  return ;
}

// Load data from disk
template<typename K, typename V>
void SkipList<K, V>::load_file() {
  _file_reader.open(STORE_FILE); // std::ifsream
  std::cout << "-----load_file-----" << std::endl;
  std::string line;
  std::string* key = new std::string();
  std::string* value = new std::string();
  while (getline(_file_reader, line)) {
    get_key_value_from_string(line, key, value); // get the data from file
    if (key->empty() || value->empty()) {
      continue;
    }
    insert_element(*key, *value);
    std::cout << "key:" << *key << "value:" << *value << std::endl;
  }
  _file_reader.close();
}

// Get current SkipList size 
template<typename K, typename V> 
int SkipList<K, V>::size() { 
    return _element_count;
}

template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) {
  if (!is_valid_string(str)) {
    return;
  }
  *key = str.substr(0, str.find(delimiter)); // substring before ":"
  *value = str.substr(str.find(delimiter)+1, str.length());
}

template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {
  if (str.empty()) {
    return false;
  }
  if (str.find(delimiter) == std::string::npos) {
    return false;
  }
  return true;
}
