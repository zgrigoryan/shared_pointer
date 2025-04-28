#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include <cstddef>      // std::nullptr_t, std::size_t
#include <utility>      // std::swap, std::move
#include <cassert>
#ifdef SHPTR_THREADSAFE
  #include <atomic>
  using ref_count_t = std::atomic<std::size_t>;
#else
  using ref_count_t = std::size_t;
#endif

// =========================== Control‑block ===========================
namespace detail {
    template<class P>
    struct ControlBlock {
        P              ptr;
        ref_count_t    ref_cnt;
        explicit ControlBlock(P p) noexcept : ptr(p), ref_cnt{1} {}
    };
}

// ======================= Primary template (objects) =================

template<class T>
class SharedPtr {
public:
    //‑‑ ctors ‑‑//
    constexpr SharedPtr() noexcept : cb_(nullptr) {}
    constexpr SharedPtr(std::nullptr_t) noexcept : cb_(nullptr) {}
    explicit SharedPtr(T* p) : cb_(p ? new detail::ControlBlock<T*>(p) : nullptr) {}

    SharedPtr(const SharedPtr& o)  noexcept : cb_(o.cb_) { inc(); }
    SharedPtr(SharedPtr&&  o)  noexcept : cb_(o.cb_) { o.cb_=nullptr; }

    //‑‑ dtor ‑‑//
    ~SharedPtr() { dec(delete_object); }

    //‑‑ assignment ‑‑//
    SharedPtr& operator=(const SharedPtr& rhs) noexcept { assign(rhs); return *this; }
    SharedPtr& operator=(SharedPtr&&  rhs) noexcept { move_assign(std::move(rhs)); return *this; }
    SharedPtr& operator=(T* raw) { reset(raw); return *this; }

    //‑‑ observers ‑‑//
    T* get()                 const noexcept { return cb_?cb_->ptr:nullptr; }
    std::size_t use_count()  const noexcept { return cb_?cb_->ref_cnt:0; }
    bool unique()            const noexcept { return use_count()==1; }
    explicit operator bool() const noexcept { return get()!=nullptr; }

    //‑‑ access ‑‑//
    T&  operator*()  const { assert(get()); return *get(); }
    T*  operator->() const noexcept { return get(); }

    //‑‑ modifiers ‑‑//
    void reset()      noexcept { dec(delete_object); cb_=nullptr; }
    void reset(T* p)            { if(get()!=p){ dec(delete_object); cb_=p?new detail::ControlBlock<T*>(p):nullptr; }}
    void swap(SharedPtr& o) noexcept { std::swap(cb_, o.cb_); }

private:
    detail::ControlBlock<T*>* cb_;

    static void delete_object(T* p){ delete p; }

    void inc() noexcept { if(cb_) ++cb_->ref_cnt; }
    template<class D> void dec(D del) noexcept {
        if(!cb_) return; if(--cb_->ref_cnt==0){ del(cb_->ptr); delete cb_; }
    }
    void assign(const SharedPtr& r) noexcept { if(this==&r) return; dec(delete_object); cb_=r.cb_; inc(); }
    void move_assign(SharedPtr&& r) noexcept { if(this==&r) return; dec(delete_object); cb_=r.cb_; r.cb_=nullptr; }
};

// ===================== Partial specialization (arrays) ===============

template<class T>
class SharedPtr<T[]> {
public:
    constexpr SharedPtr() noexcept : cb_(nullptr) {}
    constexpr SharedPtr(std::nullptr_t) noexcept : cb_(nullptr) {}
    explicit SharedPtr(T* p) : cb_(p ? new detail::ControlBlock<T*>(p) : nullptr) {}

    SharedPtr(const SharedPtr& o) noexcept : cb_(o.cb_) { inc(); }
    SharedPtr(SharedPtr&&  o) noexcept : cb_(o.cb_) { o.cb_=nullptr; }
    ~SharedPtr() { dec(delete_array); }

    SharedPtr& operator=(const SharedPtr& r) noexcept { assign(r); return *this; }
    SharedPtr& operator=(SharedPtr&&  r) noexcept { move_assign(std::move(r)); return *this; }
    SharedPtr& operator=(T* raw) { reset(raw); return *this; }

    // observers
    T* get()                 const noexcept { return cb_?cb_->ptr:nullptr; }
    std::size_t use_count()  const noexcept { return cb_?cb_->ref_cnt:0; }
    bool unique()            const noexcept { return use_count()==1; }
    explicit operator bool() const noexcept { return get()!=nullptr; }

    // element access
    T& operator[](std::size_t i) const { assert(get()); return get()[i]; }

    // modifiers
    void reset()      noexcept { dec(delete_array); cb_=nullptr; }
    void reset(T* p)            { if(get()!=p){ dec(delete_array); cb_=p?new detail::ControlBlock<T*>(p):nullptr; }}
    void swap(SharedPtr& o) noexcept { std::swap(cb_, o.cb_); }

private:
    detail::ControlBlock<T*>* cb_;
    static void delete_array(T* p){ delete[] p; }
    void inc() noexcept { if(cb_) ++cb_->ref_cnt; }
    template<class D> void dec(D del) noexcept { if(!cb_) return; if(--cb_->ref_cnt==0){ del(cb_->ptr); delete cb_; }}
    void assign(const SharedPtr& r) noexcept { if(this==&r) return; dec(delete_array); cb_=r.cb_; inc(); }
    void move_assign(SharedPtr&& r) noexcept { if(this==&r) return; dec(delete_array); cb_=r.cb_; r.cb_=nullptr; }
};

// =========================== free swap (ADL) =========================

template<class T> inline void swap(SharedPtr<T>& a, SharedPtr<T>& b) noexcept { a.swap(b); }
template<class T> inline void swap(SharedPtr<T[]>& a, SharedPtr<T[]>& b) noexcept { a.swap(b); }

#endif // SHARED_PTR_H
