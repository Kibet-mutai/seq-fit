#ifndef SEQ_FIT
#define SEQ_FIT

struct Node {
    unsigned long size;
    bool is_free;
    Node* next;
    Node* prev;
    Node(unsigned long size, bool is_free)
        : size(size)
        , is_free(is_free)
        , next(nullptr)
        , prev(nullptr)
    {
    }
};

class SeqFit {
private:
    Node* head;

public:
    SeqFit(unsigned long n_bytes);
    void* allocate(unsigned long n_bytes);
    void deallocate(void* ptr);
    void display();
    void print_h();
    ~SeqFit();
};
#endif
