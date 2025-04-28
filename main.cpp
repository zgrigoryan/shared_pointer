// main.cpp
// -----------------------------------------------------------
// Simple test-drive for SharedPtr (see SharedPtr.h).
// Build examples:
//    g++ -std=c++17 -O2 main.cpp -o demo          # non-atomic counter
//    g++ -std=c++17 -O2 -DSHPTR_THREADSAFE main.cpp -o demo  # atomic counter
// -----------------------------------------------------------------------------
#include <iostream>
#include "SharedPtr.h"

struct Foo {
    int value;
    explicit Foo(int v) : value(v) {
        std::cout << "Foo(" << value << ") constructed\n";
    }
    ~Foo() {
        std::cout << "Foo(" << value << ") destroyed\n";
    }
};

void basic_lifecycle() {
    std::cout << "\n--- basic lifecycle ---\n";
    SharedPtr<Foo> p(new Foo{42});
    std::cout << "use_count = " << p.use_count() << "\n";

    {
        SharedPtr<Foo> q = p;
        std::cout << "after copy, use_count = " << p.use_count() << "\n";
        q->value = 99;
        std::cout << "p->value = " << p->value << "\n";
    }

    std::cout << "use_count after q dies = " << p.use_count() << "\n";
    p.reset();
}

void array_demo() {
    std::cout << "\n--- array demo ---\n";
    SharedPtr<int[]> arr(new int[5]{1,2,3,4,5});
    std::cout << "arr[2] = " << arr[2] << "\n";
}

void swap_and_move() {
    std::cout << "\n--- swap and move ---\n";
    SharedPtr<Foo> a(new Foo{1});
    SharedPtr<Foo> b(new Foo{2});

    swap(a, b); // ADL‑found
    std::cout << "after swap  a->value=" << a->value << " b->value=" << b->value << "\n";

    SharedPtr<Foo> m(new Foo{7});
    SharedPtr<Foo> n(std::move(m));
    std::cout << "m is " << (m?"not null":"null") << ", n.use_count=" << n.use_count() << "\n";
}

int main() {
#ifdef SHPTR_THREADSAFE
    std::cout << "*** Thread‑safe (atomic) build ***\n";
#else
    std::cout << "*** Non‑atomic build ***\n";
#endif

    basic_lifecycle();
    array_demo();
    swap_and_move();

    std::cout << "\nAll tests finished.\n" << std::endl;
}