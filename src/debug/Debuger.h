
#include "../btree/BPlus.h"

class Debuger {
private: 
    std::shared_ptr<BufferManager> buffer_manager; 
public: 
    void traverse_tree(BPlus *tree); 

    Debuger(std::shared_ptr<BufferManager> buffer_manager_arg); 
};