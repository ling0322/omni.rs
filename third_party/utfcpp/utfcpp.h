// Copyright 2006-2016 Nemanja Trifunovic

/*
Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:
The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef UTF8_FOR_CPP_CHECKED_H_2675DCD0_9480_4c0c_B92A_CC14C027B731
#define UTF8_FOR_CPP_CHECKED_H_2675DCD0_9480_4c0c_B92A_CC14C027B731

#include <stdint.h>

#include <iterator>
#include <string>

#include "lutil/error.h"

namespace lut {
namespace utf8 {
// Helper code - not intended to be directly called by the library users. May be changed at any time
namespace internal {
// Unicode constants
// Leading (high) surrogates: 0xd800 - 0xdbff
// Trailing (low) surrogates: 0xdc00 - 0xdfff
const uint16_t LEAD_SURROGATE_MIN = 0xd800u;
const uint16_t LEAD_SURROGATE_MAX = 0xdbffu;
const uint16_t TRAIL_SURROGATE_MIN = 0xdc00u;
const uint16_t TRAIL_SURROGATE_MAX = 0xdfffu;
const uint16_t LEAD_OFFSET = 0xd7c0u;  // LEAD_SURROGATE_MIN - (0x10000 >> 10)
const uint32_t
    SURROGATE_OFFSET = 0xfca02400u;  // 0x10000u - (LEAD_SURROGATE_MIN << 10) - TRAIL_SURROGATE_MIN

// Maximum valid value for a Unicode code point
const uint32_t CODE_POINT_MAX = 0x0010ffffu;

template<typename octet_type>
inline uint8_t mask8(octet_type oc) {
  return static_cast<uint8_t>(0xff & oc);
}
template<typename u16_type>
inline uint16_t mask16(u16_type oc) {
  return static_cast<uint16_t>(0xffff & oc);
}
template<typename octet_type>
inline bool is_trail(octet_type oc) {
  return ((utf8::internal::mask8(oc) >> 6) == 0x2);
}

template<typename u16>
inline bool is_lead_surrogate(u16 cp) {
  return (cp >= LEAD_SURROGATE_MIN && cp <= LEAD_SURROGATE_MAX);
}

template<typename u16>
inline bool is_trail_surrogate(u16 cp) {
  return (cp >= TRAIL_SURROGATE_MIN && cp <= TRAIL_SURROGATE_MAX);
}

template<typename u16>
inline bool is_surrogate(u16 cp) {
  return (cp >= LEAD_SURROGATE_MIN && cp <= TRAIL_SURROGATE_MAX);
}

template<typename u32>
inline bool is_code_point_valid(u32 cp) {
  return (cp <= CODE_POINT_MAX && !utf8::internal::is_surrogate(cp));
}

template<typename octet_iterator>
inline typename std::iterator_traits<octet_iterator>::difference_type sequence_length(
    octet_iterator lead_it) {
  uint8_t lead = utf8::internal::mask8(*lead_it);
  if (lead < 0x80)
    return 1;
  else if ((lead >> 5) == 0x6)
    return 2;
  else if ((lead >> 4) == 0xe)
    return 3;
  else if ((lead >> 3) == 0x1e)
    return 4;
  else
    return 0;
}

template<typename octet_difference_type>
inline bool is_overlong_sequence(uint32_t cp, octet_difference_type length) {
  if (cp < 0x80) {
    if (length != 1) return true;
  } else if (cp < 0x800) {
    if (length != 2) return true;
  } else if (cp < 0x10000) {
    if (length != 3) return true;
  }

  return false;
}

enum utf_error {
  UTF8_OK,
  NOT_ENOUGH_ROOM,
  INVALID_LEAD,
  INCOMPLETE_SEQUENCE,
  OVERLONG_SEQUENCE,
  INVALID_CODE_POINT
};

/// Helper for get_sequence_x
template<typename octet_iterator>
utf_error increase_safely(octet_iterator& it, octet_iterator end) {
  if (++it == end) return NOT_ENOUGH_ROOM;

  if (!utf8::internal::is_trail(*it)) return INCOMPLETE_SEQUENCE;

  return UTF8_OK;
}

#define UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(IT, END) \
  {                                                    \
    utf_error ret = increase_safely(IT, END);          \
    if (ret != UTF8_OK) return ret;                    \
  }

/// get_sequence_x functions decode utf-8 sequences of the length x
template<typename octet_iterator>
utf_error get_sequence_1(octet_iterator& it, octet_iterator end, uint32_t& code_point) {
  if (it == end) return NOT_ENOUGH_ROOM;

  code_point = utf8::internal::mask8(*it);

  return UTF8_OK;
}

template<typename octet_iterator>
utf_error get_sequence_2(octet_iterator& it, octet_iterator end, uint32_t& code_point) {
  if (it == end) return NOT_ENOUGH_ROOM;

  code_point = utf8::internal::mask8(*it);

  UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

  code_point = ((code_point << 6) & 0x7ff) + ((*it) & 0x3f);

  return UTF8_OK;
}

template<typename octet_iterator>
utf_error get_sequence_3(octet_iterator& it, octet_iterator end, uint32_t& code_point) {
  if (it == end) return NOT_ENOUGH_ROOM;

  code_point = utf8::internal::mask8(*it);

  UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

  code_point = ((code_point << 12) & 0xffff) + ((utf8::internal::mask8(*it) << 6) & 0xfff);

  UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

  code_point += (*it) & 0x3f;

  return UTF8_OK;
}

template<typename octet_iterator>
utf_error get_sequence_4(octet_iterator& it, octet_iterator end, uint32_t& code_point) {
  if (it == end) return NOT_ENOUGH_ROOM;

  code_point = utf8::internal::mask8(*it);

  UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

  code_point = ((code_point << 18) & 0x1fffff) + ((utf8::internal::mask8(*it) << 12) & 0x3ffff);

  UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

  code_point += (utf8::internal::mask8(*it) << 6) & 0xfff;

  UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

  code_point += (*it) & 0x3f;

  return UTF8_OK;
}

#undef UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR

template<typename octet_iterator>
utf_error validate_next(octet_iterator& it, octet_iterator end, uint32_t& code_point) {
  if (it == end) return NOT_ENOUGH_ROOM;

  // Save the original value of it so we can go back in case of failure
  // Of course, it does not make much sense with i.e. stream iterators
  octet_iterator original_it = it;

  uint32_t cp = 0;
  // Determine the sequence length based on the lead octet
  typedef typename std::iterator_traits<octet_iterator>::difference_type octet_difference_type;
  const octet_difference_type length = utf8::internal::sequence_length(it);

  // Get trail octets and calculate the code point
  utf_error err = UTF8_OK;
  switch (length) {
    case 0:
      return INVALID_LEAD;
    case 1:
      err = utf8::internal::get_sequence_1(it, end, cp);
      break;
    case 2:
      err = utf8::internal::get_sequence_2(it, end, cp);
      break;
    case 3:
      err = utf8::internal::get_sequence_3(it, end, cp);
      break;
    case 4:
      err = utf8::internal::get_sequence_4(it, end, cp);
      break;
  }

  if (err == UTF8_OK) {
    // Decoding succeeded. Now, security checks...
    if (utf8::internal::is_code_point_valid(cp)) {
      if (!utf8::internal::is_overlong_sequence(cp, length)) {
        // Passed! Return here.
        code_point = cp;
        ++it;
        return UTF8_OK;
      } else
        err = OVERLONG_SEQUENCE;
    } else
      err = INVALID_CODE_POINT;
  }

  // Failure branch - restore the original value of the iterator
  it = original_it;
  return err;
}

template<typename octet_iterator>
inline utf_error validate_next(octet_iterator& it, octet_iterator end) {
  uint32_t ignored;
  return utf8::internal::validate_next(it, end, ignored);
}

// Internal implementation of both checked and unchecked append() function
// This function will be invoked by the overloads below, as they will know
// the octet_type.
template<typename octet_iterator, typename octet_type>
octet_iterator append(uint32_t cp, octet_iterator result) {
  if (cp < 0x80)  // one octet
    *(result++) = static_cast<octet_type>(cp);
  else if (cp < 0x800) {  // two octets
    *(result++) = static_cast<octet_type>((cp >> 6) | 0xc0);
    *(result++) = static_cast<octet_type>((cp & 0x3f) | 0x80);
  } else if (cp < 0x10000) {  // three octets
    *(result++) = static_cast<octet_type>((cp >> 12) | 0xe0);
    *(result++) = static_cast<octet_type>(((cp >> 6) & 0x3f) | 0x80);
    *(result++) = static_cast<octet_type>((cp & 0x3f) | 0x80);
  } else {  // four octets
    *(result++) = static_cast<octet_type>((cp >> 18) | 0xf0);
    *(result++) = static_cast<octet_type>(((cp >> 12) & 0x3f) | 0x80);
    *(result++) = static_cast<octet_type>(((cp >> 6) & 0x3f) | 0x80);
    *(result++) = static_cast<octet_type>((cp & 0x3f) | 0x80);
  }
  return result;
}

// One of the following overloads will be invoked from the API calls

// A simple (but dangerous) case: the caller appends byte(s) to a char array
inline char* append(uint32_t cp, char* result) {
  return append<char*, char>(cp, result);
}

// Hopefully, most common case: the caller uses back_inserter
// i.e. append(cp, std::back_inserter(str));
template<typename container_type>
std::back_insert_iterator<container_type> append(
    uint32_t cp,
    std::back_insert_iterator<container_type> result) {
  return append<std::back_insert_iterator<container_type>, typename container_type::value_type>(
      cp,
      result);
}

// The caller uses some other kind of output operator - not covered above
// Note that in this case we are not able to determine octet_type
// so we assume it's uint_8; that can cause a conversion warning if we are wrong.
template<typename octet_iterator>
octet_iterator append(uint32_t cp, octet_iterator result) {
  return append<octet_iterator, uint8_t>(cp, result);
}

}  // namespace internal
}  // namespace utf8

namespace utf8 {
/// The library API - functions intended to be called by the users

template<typename octet_iterator>
octet_iterator append(uint32_t cp, octet_iterator result) {
  if (!utf8::internal::is_code_point_valid(cp)) {
    throw AbortedError(sprintf("invalid code point %d", cp));
  }

  return internal::append(cp, result);
}

template<typename octet_iterator>
uint32_t next(octet_iterator& it, octet_iterator end) {
  uint32_t cp = 0;
  internal::utf_error err_code = utf8::internal::validate_next(it, end, cp);
  switch (err_code) {
    case internal::UTF8_OK:
      break;
    case internal::NOT_ENOUGH_ROOM:
      throw AbortedError("not enough space");
    case internal::INVALID_LEAD:
    case internal::INCOMPLETE_SEQUENCE:
    case internal::OVERLONG_SEQUENCE:
      throw AbortedError(sprintf("invalid utf-8 %d", int(*it)));
    case internal::INVALID_CODE_POINT:
      throw AbortedError(sprintf("invalid code point %d", cp));
  }
  return cp;
}

template<typename u16bit_iterator, typename octet_iterator>
void utf16to8(u16bit_iterator start, u16bit_iterator end, octet_iterator result) {
  u16bit_iterator it = start;
  while (it != end) {
    uint32_t cp = utf8::internal::mask16(*it++);
    // Take care of surrogate pairs first
    if (utf8::internal::is_lead_surrogate(cp)) {
      if (it != end) {
        uint32_t trail_surrogate = utf8::internal::mask16(*it++);
        if (utf8::internal::is_trail_surrogate(trail_surrogate)) {
          cp = (cp << 10) + trail_surrogate + internal::SURROGATE_OFFSET;
        } else {
          throw AbortedError(
              lut::sprintf(
                  "invalid utf16 trail surrogate %d in position %d",
                  trail_surrogate,
                  it - start));
        }
      } else {
        throw AbortedError(
            lut::sprintf("invalid utf16 lead surrogate %d in position %d", cp, it - start));
      }
    } else if (utf8::internal::is_trail_surrogate(cp)) {
      throw AbortedError(lut::sprintf("lone trail surrogate %d in position %d", cp, it - start));
    }

    result = utf8::append(cp, result);
  }
}

template<typename u16bit_iterator, typename octet_iterator>
void utf8to16(octet_iterator start, octet_iterator end, u16bit_iterator result) {
  while (start < end) {
    uint32_t cp = utf8::next(start, end);
    if (cp > 0xffff) {  // make a surrogate pair
      *result++ = static_cast<uint16_t>((cp >> 10) + internal::LEAD_OFFSET);
      *result++ = static_cast<uint16_t>((cp & 0x3ff) + internal::TRAIL_SURROGATE_MIN);
    } else
      *result++ = static_cast<uint16_t>(cp);
  }
}

template<typename octet_iterator, typename u32bit_iterator>
void utf32to8(u32bit_iterator start, u32bit_iterator end, octet_iterator result) {
  while (start != end) {
    result = utf8::append(*(start++), result);
  }
}

template<typename octet_iterator, typename u32bit_iterator>
void utf8to32(octet_iterator start, octet_iterator end, u32bit_iterator result) {
  for (; start < end; ++result) {
    uint32_t c = utf8::next(start, end);
    *result = c;
  }
}

}  // namespace utf8
}  // namespace lut

#endif  // header guard
