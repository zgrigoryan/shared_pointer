#include <iostream>
#include <utility>
#include <cstddef>
#include <stdexcept>
#include <memory>
#include <atomic>
#include <cassert>

// Non-templated control block base for type erasure
typedef void (*DestroyFn)(void*);

struct ControlBlockBase {
    std::atomic<size_t> count{1};
    DestroyFn destroy_fn;
    explicit ControlBlockBase(DestroyFn fn) : destroy_fn(fn) {}
    void destroy(void* ptr) noexcept {
        destroy_fn(ptr);
        delete this;
    }
};

// Control block for objects with custom deleter
template<typename U, typename Deleter>
struct ControlBlockImpl : ControlBlockBase {
    Deleter deleter;
    ControlBlockImpl(U* ptr, Deleter d)
      : ControlBlockBase(+[](void* p){ /*no-op*/ }), deleter(std::move(d)) {
        // override destroy_fn to call deleter
        destroy_fn = [](void* p) noexcept {
            Deleter del = static_cast<ControlBlockImpl*>(
                             static_cast<ControlBlockBase*>(nullptr)
                          )->deleter;
            del(static_cast<U*>(p));
        };
    }
};

// Helper to create control block with correct destroy function
template<typename U, typename Deleter>
ControlBlockBase* make_control_block(U* ptr, Deleter d) {
    struct Cb : ControlBlockBase {
        Deleter deleter;
        Cb(U* p, Deleter del)
         : ControlBlockBase(nullptr), deleter(std::move(del)) {
            destroy_fn = [](void* p) noexcept {
                Cb* self = static_cast<Cb*>(static_cast<ControlBlockBase*>(nullptr));
                self->deleter(static_cast<U*>(p));
            };
        }
    };    
    return nullptr;
}

// SharedPtr for single objects
template<typename T>
class SharedPtr {
public:
    SharedPtr() noexcept : ptr_(nullptr), cb_(nullptr) {}

    template<typename Deleter = std::default_delete<T>>
    explicit SharedPtr(T* ptr, Deleter d = Deleter()) {
        if (ptr) {
            cb_ = new ControlBlockImpl<T, Deleter>(ptr, std::move(d));
            ptr_ = ptr;
        }
    }

    SharedPtr(const SharedPtr& other) noexcept
      : ptr_(other.ptr_), cb_(other.cb_) {
        add_ref();
    }
    SharedPtr(SharedPtr&& other) noexcept
      : ptr_(other.ptr_), cb_(other.cb_) {
        other.ptr_ = nullptr;
        other.cb_  = nullptr;
    }
    ~SharedPtr() noexcept { release(); }

    SharedPtr& operator=(SharedPtr other) noexcept {
        swap(other);
        return *this;
    }

    void reset(T* ptr = nullptr) { SharedPtr(ptr).swap(*this); }
    template<typename Deleter>
    void reset(T* ptr, Deleter d) { SharedPtr(ptr, std::move(d)).swap(*this); }

    void swap(SharedPtr& other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(cb_,  other.cb_);
    }

    T* get() const noexcept            { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }
    T& operator*() const               { if (!ptr_) throw std::runtime_error("Null ptr"); return *ptr_; }
    T* operator->() const              { if (!ptr_) throw std::runtime_error("Null ptr"); return ptr_; }
    size_t use_count() const noexcept  { return cb_ ? cb_->count.load() : 0; }
    bool   unique() const noexcept     { return use_count() == 1; }

    friend bool operator==(const SharedPtr& a, const SharedPtr& b) noexcept {
        return a.ptr_ == b.ptr_;
    }
    friend bool operator!=(const SharedPtr& a, const SharedPtr& b) noexcept {
        return !(a == b);
    }

private:
    void add_ref() noexcept {
        if (cb_) cb_->count.fetch_add(1, std::memory_order_relaxed);
    }
    void release() noexcept {
        if (!cb_) return;
        if (cb_->count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            cb_->destroy(ptr_);
        }
        ptr_ = nullptr;
        cb_  = nullptr;
    }

    T* ptr_;
    ControlBlockBase* cb_;
};


int main() {
    SharedPtr<int> p1(new int(10));
    SharedPtr<int> p2 = p1;
    std::cout << "p1: " << *p1 << std::endl;
    std::cout << "p2: " << *p2 << std::endl;
    std::cout << "p1 use count: " << p1.use_count() << std::endl;
    std::cout << "p2 use count: " << p2.use_count() << std::endl;
    SharedPtr<int> p3(new int(20), [](int* p) { delete p; });
    SharedPtr<int> p4 = p3;
    std::cout << "p3: " << *p3 << std::endl;
    std::cout << "p4: " << *p4 << std::endl;
    std::cout << "p3 use count: " << p3.use_count() << std::endl;
    std::cout << "p4 use count: " << p4.use_count() << std::endl;
    SharedPtr<int> p5;
    p5 = std::move(p4);
    std::cout << "p5: " << *p5 << std::endl;
    std::cout << "p4: " << (p4 ? "not null" : "null") << std::endl;
    std::cout << "p5 use count: " << p5.use_count() << std::endl;
    std::cout << "p4 use count: " << p4.use_count() << std::endl;
    SharedPtr<int> p6(new int(30), [](int* p) { delete p; });
    std::cout << "p6: " << *p6 << std::endl;
    std::cout << "p6 use count: " << p6.use_count() << std::endl;
    SharedPtr<int> p7 = p6;
    std::cout << "p7: " << *p7 << std::endl;
    std::cout << "p6 use count: " << p6.use_count() << std::endl;
    std::cout << "p7 use count: " << p7.use_count() << std::endl;
    SharedPtr<int> p8 = std::move(p7);
    std::cout << "p8: " << *p8 << std::endl;
    std::cout << "p7: " << (p7 ? "not null" : "null") << std::endl;
    std::cout << "p8 use count: " << p8.use_count() << std::endl;
    std::cout << "p7 use count: " << p7.use_count() << std::endl;
    SharedPtr<int> p9(new int(40), [](int* p) { delete p; });
    std::cout << "p9: " << *p9 << std::endl;
    std::cout << "p9 use count: " << p9.use_count() << std::endl;
    SharedPtr<int> p10 = p9;
    SharedPtr<int> p11 = std::move(p10);
    std::cout << "p11: " << *p11 << std::endl;
    std::cout << "p10: " << (p10 ? "not null" : "null") << std::endl;
    std::cout << "p11 use count: " << p11.use_count() << std::endl;
    std::cout << "p10 use count: " << p10.use_count() << std::endl;
    SharedPtr<int> p12(new int(50), [](int* p) { delete p; });
    std::cout << "p12: " << *p12 << std::endl;
    std::cout << "p12 use count: " << p12.use_count() << std::endl;
    p12.reset(new int(60), [](int* p) { delete p; });
    std::cout << "p12 after reset: " << *p12 << std::endl;
    std::cout << "p12 use count: " << p12.use_count() << std::endl;
    return 0;
}
