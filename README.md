## Sequential Fit allocation

- This technique involves orginizing memory into doubly-linked list of free and reserved
  regions
- When an allocation is requested, the memory manager moves sequentially through the list
  untils it finds a free block of memory that can fit the requested size (first-fit allocation).

### Example

```cpp
#include "seq_fit_v2.cpp"


int main()
{
    SeqFitMemManager* mmptr = new SeqFitMemManager(1024);
    void* ptr = mmptr->allocate(sizeof(int));
    mmptr->print_state();
}
```
