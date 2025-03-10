#include "mem-mgr.h"
#include <iostream>
#include <vector>

SeqFit::SeqFit(unsigned long n_bytes)
{
    head = new Node(n_bytes, true);
}

SeqFit::~SeqFit()
{
    Node* current = head;
    while (current) {
        Node* next = current->next;
        delete current;
        current = next;
    }
}
void* SeqFit::allocate(unsigned long n_bytes)
{
    Node* current = head;
    while (current != nullptr) {
        // check if current size is available and is greater or equal to the requested size
        if (current->is_free && current->size >= n_bytes) {
            current->is_free = false;
            // split if current->size is greater than requested size
            if (current->size > n_bytes) {
                Node* new_block = new Node(current->size - n_bytes, true);
                new_block->next = current->next;
                if (new_block->next) {
                    new_block->next->prev = new_block;
                }
                current->size = n_bytes;
                current->next = new_block;
                new_block->prev = current;
            }
            return reinterpret_cast<void*>(reinterpret_cast<char*>(current) + sizeof(Node));
        }
        current = current->next;
    }
    return nullptr;
}

void SeqFit::deallocate(void* ptr)
{
    if (!ptr)
        return;

    Node* target = reinterpret_cast<Node*>(reinterpret_cast<char*>(ptr) - sizeof(Node));
    target->is_free = true;

    // Merge with next block if free
    if (target->next && target->next->is_free) {
        target->size += sizeof(Node) + target->next->size;
        Node* to_delete = target->next;
        target->next = to_delete->next;
        if (target->next) {
            target->next->prev = target;
        }
        delete to_delete;
    }

    // Merge with previous block if free
    if (target->prev && target->prev->is_free) {
        target->prev->size += sizeof(Node) + target->size;
        target->prev->next = target->next;
        if (target->next) {
            target->next->prev = target->prev;
        }
        delete target;
    }
}

void SeqFit::display()
{
    Node* curr = head;
    std::cout << "Memory Blocks:\n";
    while (curr != nullptr) {
        std::cout << "[Addr: " << curr << " | Size: " << curr->size
                  << " | Free: " << std::boolalpha << curr->is_free << "]\n";
        curr = curr->next;
    }
}

int main(int argc, char* argv[])
{
    SeqFit seq_fit(80);

    seq_fit.display();
    std::cout << "-------------------\n";
    seq_fit.display();
    return 0;
}
