#ifndef LONG_LIVED_VERSION_ENTITY_H
#define LONG_LIVED_VERSION_ENTITY_H


#include <type_traits>

namespace entt {


  class LongLivedVersionIdType {
    const LongLivedVersionIdType* prev;
    mutable LongLivedVersionIdType* next;
    using refcount_type = std::uint32_t;
    mutable refcount_type refcount;

    static LongLivedVersionIdType*& _getRootPtr() {
      static LongLivedVersionIdType* root(nullptr);
      return root;
    }

    static bool setRootReportIfSet(LongLivedVersionIdType* r) {
      LongLivedVersionIdType*& getRootPtr = _getRootPtr();
      bool set = (getRootPtr == nullptr);
      getRootPtr = r;
      return set;
    }
   public:

    //constructors should be private and only instantiable by friend entity_type
    LongLivedVersionIdType() : prev(nullptr), next(nullptr), refcount(0) {}
    LongLivedVersionIdType(const LongLivedVersionIdType* p) : prev(p), next(nullptr), refcount(0) {}

    struct RootUnset {};

    inline refcount_type get_refcount() const { return refcount; }

    static LongLivedVersionIdType* setIfUnSetAndGetRoot(LongLivedVersionIdType* r) {
      LongLivedVersionIdType*& getRootPtr = _getRootPtr();
      if (getRootPtr == nullptr) {
        getRootPtr = r;
      }
      return getRootPtr;
    }

    static LongLivedVersionIdType* getRoot() ENTT_NOEXCEPT {
      return _getRootPtr();
    }

    static LongLivedVersionIdType* getRootThrowIfUnset() {
      LongLivedVersionIdType* ptr = getRoot();
      if (nullptr == ptr)
        throw RootUnset();
      return ptr;
    }

    static LongLivedVersionIdType*& getHead() {
      static LongLivedVersionIdType* head;
      return head;
    }

    ~LongLivedVersionIdType() {
      //assert(refcount == 0);
      if (prev != nullptr)
        prev->adjust_next(next);
      if (next != nullptr)
        next->prev = prev;
      if (getRoot() == this) {
        _getRootPtr() = nullptr;
      }
    }

    void adjust_next(LongLivedVersionIdType* nnext) const {
      next = nnext;
    }

    void decref() {
      assert(refcount > 0);
      refcount--;
      //bool firstNode = (this->prev == nullptr);
      bool firstNode = (this == getRoot());
      if (0 == refcount && !firstNode)
        delete this;
    }

    void incref() {
      refcount++;
    }

    const LongLivedVersionIdType* id() const { return this; }
    inline bool operator==(const LongLivedVersionIdType* other) const {
      return this == other;
    };

    inline bool operator!=(const LongLivedVersionIdType* other) const {
      return this != other;
    }

    LongLivedVersionIdType* upgrade_basic() {
      LongLivedVersionIdType* pnext = this->next;
      if (pnext == nullptr) {
        pnext = new LongLivedVersionIdType(this);
        this->next = pnext;
        pnext->incref();
      }
      this->decref();
      return pnext;
    }

    LongLivedVersionIdType* upgrade_lookahead(int lookahead = 3) {
      int idx = 0;
      unsigned int maxRefCount = 0;
      LongLivedVersionIdType* maxRefCountPtr = nullptr;
      LongLivedVersionIdType* pthis = this;
      LongLivedVersionIdType* pprev = nullptr;
      do {
        pprev = pthis;
        pthis = pthis->next;
        if (pthis && pthis->refcount > maxRefCount) {
          maxRefCount = pthis->refcount;
          maxRefCountPtr = pthis;
        }
      } while (pthis && idx < lookahead);
      if (maxRefCountPtr == nullptr) {
        pthis = new LongLivedVersionIdType(pprev);
        pprev->next = pthis;
        pthis->incref();
      } else {
        pthis = maxRefCountPtr;
        maxRefCountPtr->incref();
      }
      this->decref();
      return pthis;
    }
  };


  struct LongLivedVersionIdRef {
    LongLivedVersionIdType* ptr_id;


    constexpr LongLivedVersionIdRef() : ptr_id(nullptr) {}
    LongLivedVersionIdRef(int numeric_id) : ptr_id(nullptr) {
      if (numeric_id-- > 0) {
        ptr_id = LongLivedVersionIdType::getRoot();
        ptr_id->incref();
      }
      while (numeric_id-- > 0) {
        ptr_id = ptr_id->upgrade_basic();
      }
    }

    LongLivedVersionIdRef(const LongLivedVersionIdRef& other) : ptr_id(other.ptr_id) {
      if (ptr_id != nullptr)
        ptr_id->incref();
    }

    LongLivedVersionIdRef(LongLivedVersionIdRef&& other) : ptr_id(other.ptr_id) {
      other.ptr_id = nullptr;
    }

    ~LongLivedVersionIdRef() {
      if (ptr_id != nullptr)
        ptr_id->decref();
    }

    inline LongLivedVersionIdRef& operator=(const LongLivedVersionIdRef& other) {
      if (ptr_id != other.ptr_id) {
        if (ptr_id != nullptr)
          ptr_id->decref();

        ptr_id = other.ptr_id;

        if (ptr_id != nullptr)
          ptr_id->incref();
      }
      return *this;
    }

    inline LongLivedVersionIdRef& operator+(std::uint32_t num_offset) {
      return upgrade_lookahead(num_offset);
    }

    inline bool operator==(const LongLivedVersionIdRef& other) const {
      return ptr_id == other.ptr_id;
    }
    inline bool operator!=(const LongLivedVersionIdRef& other) const {
      return ptr_id != other.ptr_id;
    }
    inline bool operator<(const LongLivedVersionIdRef& other) const {
      return ptr_id < other.ptr_id;
    }

    inline LongLivedVersionIdRef& upgrade_lookahead(int lookahead=3) {
      if (ptr_id == nullptr && lookahead-- > 0) {
        ptr_id = LongLivedVersionIdType::getRoot();
        ptr_id->incref();
      }
      if (lookahead > 0)
        ptr_id = ptr_id->upgrade_lookahead(lookahead);
      return *this;
    }
  };

  struct EntTypeWithLongTermVersionId {
    using entity_type = std::uint32_t;
    using version_type = LongLivedVersionIdRef;

    entity_type entity_id;
    version_type version_id;
    //static constexpr version_type version_mask = version_type();
    static const version_type default_version() {
      //return LongLivedVersionIdType::getRoot();
      return version_type(1);
    }

    EntTypeWithLongTermVersionId(): entity_id(), version_id() {}
    EntTypeWithLongTermVersionId(std::uint32_t l) : entity_id(l), version_id() {}
    EntTypeWithLongTermVersionId(const entity_type e_id, const version_type v_id) : entity_id(e_id), version_id(v_id) {}
    EntTypeWithLongTermVersionId(const EntTypeWithLongTermVersionId& other) : entity_id(other.entity_id), version_id(other.version_id) {}
    EntTypeWithLongTermVersionId(EntTypeWithLongTermVersionId&& other) : entity_id(other.entity_id), version_id(other.version_id) {}

    inline bool operator==(const EntTypeWithLongTermVersionId& other) const {
      return entity_id == other.entity_id && version_id == other.version_id;
    }
    inline bool operator!=(const EntTypeWithLongTermVersionId& other) const {
      return entity_id != other.entity_id || version_id != other.version_id;
    }
    inline bool operator<(const EntTypeWithLongTermVersionId& other) const {
      return (entity_id < other.entity_id) || (version_id < other.version_id);
    }
    inline EntTypeWithLongTermVersionId& operator=(const EntTypeWithLongTermVersionId& other) {
      entity_id = other.entity_id;
      version_id = other.version_id;
      return *this;
    }
  };


}

#endif