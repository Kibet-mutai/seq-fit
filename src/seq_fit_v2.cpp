#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <string>

using U1 = unsigned char; // 1 byte
using U4 = unsigned long; // 4 bytes

/**
 * list element format
 *      |0   3|  |4  7| |  8  | |9  12|  |13 .. n|
 *      [ get_prev ] [get_next] [get_state] [ get_size ] [PAYLOAD]
 *         U4      U4     U1       U4        ?
 *
 * byte allocated/freed is address of first byte of payload
 * header = 13 bytes
 *
 * byte[0] is occupied by header data, so is always used, thus first
 * link has prev = 0 (0 indicates not used) last link has next = 0
 *
 * **/

class MemoryBlockManager {
protected:
    std::size_t RAM_SIZE;
    U1* ram;

public:
    MemoryBlockManager(std::size_t size)
        : RAM_SIZE(size)
    {
        ram = new U1[RAM_SIZE];
        memset(ram, 0, RAM_SIZE);
    }
    enum State : std::uint8_t { FREE = 0,
        OCCUPIED = 1 };

    std::uint32_t get_prev(std::size_t index) const
    {
        return *reinterpret_cast<const std::uint32_t*>(&ram[index - 13]);
    }

    std::uint32_t get_next(size_t index) const
    {
        return *reinterpret_cast<const std::uint32_t*>(&ram[index - 9]);
    }

    State get_state(size_t index) const
    {
        return static_cast<State>(*reinterpret_cast<const std::uint8_t*>(&ram[index - 5]));
    }

    std::uint32_t get_size(size_t index) const
    {
        return *reinterpret_cast<const std::uint32_t*>(&ram[index - 4]);
    }

    void set_prev(size_t index, std::uint32_t value)
    {
        *reinterpret_cast<std::uint32_t*>(&ram[index - 13]) = value;
    }

    void set_next(size_t index, std::uint32_t value)
    {
        *reinterpret_cast<std::uint32_t*>(&ram[index - 9]) = value;
    }

    void set_state(size_t index, State value)
    {
        *reinterpret_cast<std::uint8_t*>(&ram[index - 5]) = static_cast<std::uint8_t>(value);
    }

    void set_size(size_t index, std::uint32_t value)
    {
        *reinterpret_cast<std::uint32_t*>(&ram[index - 4]) = value;
    }
};

class SeqFitMemManager : public MemoryBlockManager {

private:
    // U1* ram; /*memory storage*/
    /*U4 size;*/
    static constexpr std::size_t HEADER_SIZE = 13;
    static constexpr std::size_t START = 13;

    void split(U4 addr, U4 n_bytes);
    void merge(U4 prev, U4 current, U4 next);

public:
    SeqFitMemManager(U4 n_bytes);
    ~SeqFitMemManager();

    void* allocate(U4 n_bytes);
    void deallocate(void* addr);
    void print_state();
};

template <class T, std::size_t S>
using ARRAY = std::array<T, S>;
SeqFitMemManager::SeqFitMemManager(U4 n_bytes)
    : MemoryBlockManager(n_bytes)
{
    if (n_bytes <= HEADER_SIZE) {
        std::cout << "SeqFitMemManager::SeqFitMemManager(): Not enough memory in the constructor\n";
        std::exit(1);
    }
    if (!ram) {
        std::cerr << "memory is too small to be allocated\n";
        std::exit(1);
    }
    set_prev(START, 0);
    set_next(START, 0);
    set_state(START, State::FREE);
    set_size(START, n_bytes - HEADER_SIZE);

    std::cout << "SeqFitMemManager::SeqFitMemManager(): bytes allocated = "
              << n_bytes
              << '\n';
}

SeqFitMemManager::~SeqFitMemManager()
{

    delete[] ram;
    std::cout << "SeqFitMemManager::~SeqFitMemManager(): size freed = " << RAM_SIZE << '\n';
}

/* U4 - number of bytes required
 * returns address of first byte of memory region allocated
 * or nullptr if cannot allocate a large enough block
 */
void* SeqFitMemManager::allocate(unsigned long bytes)
{
    U4 current;

    if (bytes == 0) {
        std::cout << "SeqFitMemManager::allocate(): zero bytes requested\n";
        return nullptr;
    }

    // traverse the linked list starting with the first element - first-fit allocation
    current = START;

    while (current < RAM_SIZE && get_next(current) != 0) {
        if ((get_size(current) >= bytes) && (get_state(current) == State::FREE)) {
            split(current, bytes);
            return reinterpret_cast<void*>(&ram[current]);
        }
        current = get_next(current);
    }

    // handle the last block (which has get_next(current) == 0)
    if ((get_size(current) >= bytes) && (get_state(current) == State::FREE)) {
        split(current, bytes);
        return reinterpret_cast<void*>(&ram[current]);
    }

    return nullptr;
}

void SeqFitMemManager::split(U4 address, U4 n_bytes)
{
    /* the payload to have enough room for
     * n_bytes = size of the request;
     * HEADER_SIZE = header for the new region
     * HEADER_SIZE = payload for the new region (arbitrary 13 bytes)
     */

    if (get_size(address) >= n_bytes + HEADER_SIZE + HEADER_SIZE) {
        U4 old_next;
        U4 old_prev;
        U4 old_size;

        U4 new_addr;

        old_next = get_next(address);
        old_prev = get_prev(address);
        old_size = get_size(address);

        new_addr = address + n_bytes + HEADER_SIZE;

        // update the current block to allocated state
        set_next(address, new_addr);
        set_prev(address, old_prev);
        set_state(address, State::OCCUPIED);
        set_size(address, n_bytes);

        // set new block with remaining free space
        set_next(new_addr, old_next);
        set_prev(new_addr, old_prev);
        set_state(new_addr, State::FREE);
        set_size(new_addr, old_size - n_bytes - HEADER_SIZE);

    } else {
        std::cout << "SeqFitMemManager::split(): Fail to split\n";
        set_state(address, State::OCCUPIED);
    }
    return;
}

void SeqFitMemManager::deallocate(void* address)
{
    U4 free; // index into ram[]

    if (address == nullptr) {
        std::cerr << "SeqFitMemManager::deallocate(): null pointer\n";
        return;
    }

    // check if address exist
    if (address >= reinterpret_cast<void*>(&ram[RAM_SIZE])
        || address < reinterpret_cast<void*>(&ram[0])) {
        std::cerr << "SeqFitMemManager::deallocate(): address out of bounds\n";
        return;
    }

    // translate void* address to index in ram[]
    free = static_cast<U4>((reinterpret_cast<U1*>(address) - &ram[0]));

    std::cout << "SeqFitMemManager::deallocate(): address resolves to index: "
              << free << '\n';

    // header always occupies first 13 bytes of storage

    if (free < 13) {
        std::cerr << "SeqFitMemManager::deallocate(): address header\n";
        return;
    }

    if (get_state(free) != State::OCCUPIED || get_prev(free) >= free || get_next(free) >= RAM_SIZE || get_size(free) >= RAM_SIZE || get_size(free) == 0) {
        std::cerr << "SeqFitMemManager::deallocate(): referencing invalid region\n";
        return;
    }
    merge(get_prev(free), free, get_next(free));

    return;
}

/*
 *      4 cases (F = free, O = occupied)
 *      FOF   -> [F]
 *      OOF   -> 0[F]
 *      FOO   -> [F]O
 *      OOO   -> OFO
 */
void SeqFitMemManager::merge(U4 prev, U4 current, U4 next)

{
    /* handle special cases of region at the end(s)
     * prev = 0              low end
     * next = 0               high end
     * prev = 0 && next = 0   only 1 list element
     */

    if (prev == 0) {
        if (next == 0) {
            set_state(current, State::FREE);
        } else if (get_state(next) == State::OCCUPIED) {
            set_state(current, State::FREE);
        } else if (get_state(next) == State::FREE) {
            U4 temp;

            std::cout << "SeqFitMemManager::merge(): merging to the get_next\n";

            set_state(current, State::FREE);
            set_size(current, get_size(next) + HEADER_SIZE);
            set_next(current, get_next(next));

            temp = get_next(next);
            set_prev(temp, current);
        }
    } else if (next == 0) {
        if (get_state(prev) == State::OCCUPIED) {
            set_state(current, State::FREE);
        } else if (get_state(prev) == State::FREE) {
            std::cout << "SeqFitMemManager::merge(): merging to get_prevIOUS\n";
            set_size(prev, get_size(prev) + get_size(current) + HEADER_SIZE);
            set_next(prev, get_next(current));
        }
    } /* handle the 4 cases*/
    else if (get_state(prev) == State::OCCUPIED && get_state(next) == State::OCCUPIED) {
        set_state(current, State::FREE);
    } else if (get_state(prev) == State::OCCUPIED && get_state(next) == State::FREE) {
        U4 temp;
        set_state(current, State::FREE);
        set_size(current, get_size(current) + get_size(next) + HEADER_SIZE);
        set_next(current, get_next(next));

        temp = get_next(next);

        if (temp) {
            set_prev(temp, current);
        }
    } else if (get_state(prev) == State::FREE && get_state(next) == State::OCCUPIED) {
        set_size(prev, get_size(prev) + get_size(current) + HEADER_SIZE);
        set_next(prev, get_next(current));
        set_prev(next, prev);
    } else if (get_state(prev) == State::FREE && get_state(next) == State::FREE) {
        U4 temp;
        std::cout << "SeqFitMemManager::merge(): merging both sides\n";
        set_size(prev, get_size(prev) + get_size(current) + HEADER_SIZE + get_size(next) + HEADER_SIZE);

        set_next(prev, get_next(next));

        temp = get_next(next);
        if (temp) {
            set_prev(temp, prev);
        }
    }
    return;
}

void SeqFitMemManager::print_state()
{
    U4 index = 0, current = START;

    while (get_next(current) != 0) {
        if (current < 13 || current >= RAM_SIZE) {
            std::cerr << "Invalid memory access at 0x" << std::hex << current << '\n';
            break;
        }
        std::cout << "get_prevIOUS = " << std::hex << "0x" << get_prev(current) << " ";
        std::cout << "ADDRESS = "
                  << "0x" << current << " ";
        std::cout << (get_state(current) == State::FREE ? "FREE" : "OCCUPIED") << " ";
        std::cout << "get_size = " << get_size(current) << " ";
        std::cout << "get_next = " << std::hex << "0x" << get_next(current);
        std::cout << '\n';

        if (get_next(current) == 0)
            break;
        current = get_next(current);
        ++index;
    }

    // print the last list element
    std::cout << "----LAST ELEMENT------\n";
    std::cout << "get_prevIOUS = "
              << " index = " << index
              << ", " << std::hex << "0x" << get_prev(current) << " ";
    std::cout << "ADDRESS = " << std::hex << "0x" << current << " ";
    std::cout << (get_state(current) == State::FREE ? "FREE" : "OCCUPIED") << " ";
    std::cout << "get_size = " << std::dec << get_size(current) << " ";
    std::cout << "get_next = " << get_next(current);
    std::cout << '\n';
}

int main(int argc, char* argv[])
{
    SeqFitMemManager* mmptr = new SeqFitMemManager(1024);

    int* ptr = reinterpret_cast<int*>(mmptr->allocate(sizeof(int)));
    *ptr = 200;
    if (ptr == nullptr) {
        std::cerr << "Fail to allocate mem\n";
        std::exit(1);
    }
    std::cout << *ptr << " -- " << ptr << '\n';
    long* l_ptr = reinterpret_cast<long*>(mmptr->allocate(sizeof(long)));
    if (l_ptr == nullptr) {
        std::cerr << "Fail to allocate mem\n";
        std::exit(1);
    }
    mmptr->print_state();

    delete mmptr;
    return 0;
}
