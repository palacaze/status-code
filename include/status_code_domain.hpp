/* Proposed SG14 status_code
(C) 2018 Niall Douglas <http://www.nedproductions.biz/> (5 commits)
File Created: Feb 2018


Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License in the accompanying file
Licence.txt or at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Distributed under the Boost Software License, Version 1.0.
(See accompanying file Licence.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef SYSTEM_ERROR2_STATUS_CODE_DOMAIN_HPP
#define SYSTEM_ERROR2_STATUS_CODE_DOMAIN_HPP

#include "config.hpp"

#include <cstring>  // for strchr

SYSTEM_ERROR2_NAMESPACE_BEGIN

/*! The main workhorse of the system_error2 library, can be typed (`status_code<DomainType>`), erased-immutable (`status_code<void>`) or erased-mutable (`status_code<erased<T>>`).

Be careful of placing these into containers! Equality and inequality operators are
*semantic* not exact. Therefore two distinct items will test true! To help prevent
surprise on this, `operator<` and `std::hash<>` are NOT implemented in order to
trap potential incorrectness. Define your own custom comparison functions for your
container which perform exact comparisons.
*/
template <class DomainType> class status_code;
class _generic_code_domain;
//! The generic code is a status code with the generic code domain, which is that of `errc` (POSIX).
using generic_code = status_code<_generic_code_domain>;

namespace detail
{
  template <class T> struct status_code_sizer
  {
    void *a;
    T b;
  };
  template <class To, class From> struct type_erasure_is_safe
  {
    static constexpr bool value = std::is_trivially_copyable<From>::value  //
                                  && (sizeof(status_code_sizer<From>) <= sizeof(status_code_sizer<To>));
  };
}  // namespace detail

/*! Abstract base class for a coding domain of a status code.
*/
class status_code_domain
{
  template <class DomainType> friend class status_code;

public:
  //! Type of the unique id for this domain.
  using unique_id_type = unsigned long long;
  /*! (Potentially thread safe) Reference to a message string.

  Be aware that you cannot add payload to implementations of this class.
  You get exactly the `void *[3]` array to keep state, this is usually
  sufficient for a `std::shared_ptr<>` or a `std::string`.

  You can install a handler to be called when this object is copied,
  moved and destructed. This takes the form of a C function pointer.
  */
  class string_ref
  {
  public:
    //! The value type
    using value_type = const char;
    //! The size type
    using size_type = size_t;
    //! The pointer type
    using pointer = const char *;
    //! The const pointer type
    using const_pointer = const char *;
    //! The iterator type
    using iterator = const char *;
    //! The const iterator type
    using const_iterator = const char *;

  protected:
    //! The operation occurring
    enum class _thunk_op
    {
      copy,
      move,
      destruct
    };
    //! The prototype of the handler function. Copies can throw, moves and destructs cannot.
    using _thunk_spec = void (*)(string_ref *dest, const string_ref *src, _thunk_op op);
#ifndef NDEBUG
  private:
    static void _checking_string_thunk(string_ref *dest, const string_ref *src, _thunk_op /*unused*/) noexcept
    {
      (void) dest;
      (void) src;
      assert(dest->_thunk == _checking_string_thunk);
      assert(src == nullptr || src->_thunk == _checking_string_thunk);
      // do nothing
    }

  protected:
#endif
    //! Pointers to beginning and end of character range
    pointer _begin{}, _end{};
    //! Three `void*` of state
    void *_state[3]{};  // at least the size of a shared_ptr
    //! Handler for when operations occur
    const _thunk_spec _thunk{nullptr};

    constexpr explicit string_ref(_thunk_spec thunk) noexcept : _thunk(thunk) {}

  public:
    //! Construct from a C string literal
    SYSTEM_ERROR2_CONSTEXPR14 explicit string_ref(const char *str, size_type len = static_cast<size_type>(-1), void *state0 = nullptr, void *state1 = nullptr, void *state2 = nullptr,
#ifndef NDEBUG
                                                  _thunk_spec thunk = _checking_string_thunk
#else
                                                  _thunk_spec thunk = nullptr
#endif
                                                  ) noexcept : _begin(str),
                                                               _end((len == static_cast<size_type>(-1)) ? (str + detail::cstrlen(str)) : (str + len)),  // NOLINT
                                                               _state{state0, state1, state2},
                                                               _thunk(thunk)
    {
    }
    //! Copy construct the derived implementation.
    string_ref(const string_ref &o)
        : _begin(o._begin)
        , _end(o._end)
        , _state{o._state[0], o._state[1], o._state[2]}
        , _thunk(o._thunk)
    {
      if(_thunk != nullptr)
      {
        _thunk(this, &o, _thunk_op::copy);
      }
    }
    //! Move construct the derived implementation.
    string_ref(string_ref &&o) noexcept : _begin(o._begin), _end(o._end), _state{o._state[0], o._state[1], o._state[2]}, _thunk(o._thunk)
    {
      if(_thunk != nullptr)
      {
        _thunk(this, &o, _thunk_op::move);
      }
    }
    //! Copy assignment
    string_ref &operator=(const string_ref &o)
    {
      this->~string_ref();
      new(this) string_ref(o);
      return *this;
    }
    //! Move assignment
    string_ref &operator=(string_ref &&o) noexcept
    {
      this->~string_ref();
      new(this) string_ref(static_cast<string_ref &&>(o));
      return *this;
    }
    //! Destruction
    ~string_ref()
    {
      if(_thunk != nullptr)
      {
        _thunk(this, nullptr, _thunk_op::destruct);
      }
      _begin = _end = nullptr;
    }

    //! Returns whether the reference is empty or not
    bool empty() const noexcept { return _begin == _end; }
    //! Returns the size of the string
    size_type size() const noexcept { return _end - _begin; }
    //! Returns a null terminated C string
    value_type *c_str() const noexcept { return _begin; }
    //! Returns the beginning of the string
    iterator begin() noexcept { return _begin; }
    //! Returns the beginning of the string
    const_iterator begin() const noexcept { return _begin; }
    //! Returns the beginning of the string
    const_iterator cbegin() const noexcept { return _begin; }
    //! Returns the end of the string
    iterator end() noexcept { return _end; }
    //! Returns the end of the string
    const_iterator end() const noexcept { return _end; }
    //! Returns the end of the string
    const_iterator cend() const noexcept { return _end; }
  };

  /*! A reference counted, threadsafe reference to a message string.
  */
  class atomic_refcounted_string_ref : public string_ref
  {
    struct _allocated_msg
    {
      mutable std::atomic<unsigned> count;
    };
    _allocated_msg *&_msg() noexcept { return reinterpret_cast<_allocated_msg *&>(this->_state[0]); }                  // NOLINT
    const _allocated_msg *_msg() const noexcept { return reinterpret_cast<const _allocated_msg *>(this->_state[0]); }  // NOLINT

    static void _refcounted_string_thunk(string_ref *_dest, const string_ref *_src, _thunk_op op) noexcept
    {
      atomic_refcounted_string_ref *dest = static_cast<atomic_refcounted_string_ref *>(_dest);
      const atomic_refcounted_string_ref *src = static_cast<const atomic_refcounted_string_ref *>(_src);
      (void) src;
      assert(dest->_thunk == _refcounted_string_thunk);
      assert(src == nullptr || src->_thunk == _refcounted_string_thunk);
      switch(op)
      {
      case _thunk_op::copy:
      case _thunk_op::move:
      {
        if(dest->_msg() != nullptr)
        {
          auto count = dest->_msg()->count.fetch_add(1);
          assert(count != 0);
        }
        return;
      }
      case _thunk_op::destruct:
      {
        if(dest->_msg() != nullptr)
        {
          auto count = dest->_msg()->count.fetch_sub(1);
          if(count == 1)
          {
            free((void *) dest->_begin);  // NOLINT
            delete dest->_msg();
          }
        }
      }
      }
    }

  public:
    //! Construct from a C string literal
    explicit atomic_refcounted_string_ref(const char *str, size_type len = static_cast<size_type>(-1), void *state1 = nullptr, void *state2 = nullptr) noexcept : string_ref(str, len, nullptr, state1, state2)
    {
      _msg() = static_cast<_allocated_msg *>(calloc(1, sizeof(_allocated_msg)));  // NOLINT
      if(_msg() == nullptr)
      {
        free((void *) this->_begin);  // NOLINT
        _msg() = nullptr;             // disabled
        this->_begin = "failed to get message from system";
        this->_end = strchr(this->_begin, 0);
        return;
      }
      ++_msg()->count;
    }
  };

private:
  unique_id_type _id;

protected:
  /*! Use [https://www.random.org/cgi-bin/randbyte?nbytes=8&format=h](https://www.random.org/cgi-bin/randbyte?nbytes=8&format=h) to get a random 64 bit id.

  Do NOT make up your own value. Do NOT use zero.
  */
  constexpr explicit status_code_domain(unique_id_type id) noexcept : _id(id) {}
  //! No public copying at type erased level
  status_code_domain(const status_code_domain &) = default;
  //! No public moving at type erased level
  status_code_domain(status_code_domain &&) = default;
  //! No public assignment at type erased level
  status_code_domain &operator=(const status_code_domain &) = default;
  //! No public assignment at type erased level
  status_code_domain &operator=(status_code_domain &&) = default;
  //! No public destruction at type erased level
  ~status_code_domain() = default;

public:
  //! True if the unique ids match.
  constexpr bool operator==(const status_code_domain &o) const noexcept { return _id == o._id; }
  //! True if the unique ids do not match.
  constexpr bool operator!=(const status_code_domain &o) const noexcept { return _id != o._id; }
  //! True if this unique is lower than the other's unique id.
  constexpr bool operator<(const status_code_domain &o) const noexcept { return _id < o._id; }

  //! Returns the unique id used to identify identical category instances.
  constexpr unique_id_type id() const noexcept { return _id; }
  //! Name of this category.
  virtual string_ref name() const noexcept = 0;

protected:
  //! True if code means failure.
  virtual bool _failure(const status_code<void> &code) const noexcept = 0;
  //! True if code is (potentially non-transitively) equivalent to another code in another domain.
  virtual bool _equivalent(const status_code<void> &code1, const status_code<void> &code2) const noexcept = 0;
  //! Returns the generic code closest to this code, if any.
  virtual generic_code _generic_code(const status_code<void> &code) const noexcept = 0;
  //! Return a reference to a string textually representing a code.
  virtual string_ref _message(const status_code<void> &code) const noexcept = 0;
#if defined(_CPPUNWIND) || defined(__EXCEPTIONS) || defined(STANDARDESE_IS_IN_THE_HOUSE)
  //! Throw a code as a C++ exception.
  virtual void _throw_exception(const status_code<void> &code) const = 0;
#else
  // Keep a vtable slot for binary compatibility
  virtual void _throw_exception(const status_code<void> &code) const final { abort(); }
#endif
};

SYSTEM_ERROR2_NAMESPACE_END

#endif
