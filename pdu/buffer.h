#ifndef buffer_h
#define buffer_h

#include <array>
#include <string.h>

#ifdef USE_BPSTD_SV
   #include "bpstd/string_view.h"
   using bpstd::basic_string_view;
   using string_view_t  = bpstd::string_view;
   using ustring_view_t = bpstd::basic_string_view<uint8_t>;
#else
   #include <string_view>
   using std::basic_string_view;
   using string_view_t  = std::string_view;
   using ustring_view_t = std::basic_string_view<uint8_t>;
#endif

#ifdef CUAP_DEBUG
   #include "mib/io/fmt-5"
#endif

#ifndef INDEX_T
   #define INDEX_T int32_t
#endif

#include "types.h"

using std::string;

namespace
{
   typedef  const char cchar;
   typedef  const double cdouble;
   typedef  const int cint;

   typedef  unsigned int uint;
   typedef  const unsigned int cuint;

   typedef  char int8;
   typedef  const char cint8;

   typedef  unsigned char uint8;
   typedef  const unsigned char cuint8;

   typedef  short int16;
   typedef  const short cint16;

   typedef  unsigned short uint16;
   typedef  const unsigned short cuint16;

   typedef  int int32;
   typedef  const int cint32;

   typedef  unsigned int uint32;
   typedef  const unsigned int cuint32;

   typedef  long long  int64;
   typedef  const long long cint64;

   typedef  unsigned long long uint64;
   typedef  const unsigned long long cuint64;

   typedef  unsigned long ulong;
   typedef  const unsigned long culong;


   typedef  long long lli;
   typedef  unsigned long long ulli;
   typedef  const unsigned long long culli;

   typedef unsigned char uchar;
   typedef unsigned const char ucchar;
}

namespace concepts
{
   using char_t   = char*;
   using cchar_t  = const char*;
   using uchar_t  = uint8_t*;
   using cuchar_t = const uint8_t*;
}

namespace misc
{
   using index_t   = INDEX_T;
   using size_type = int;

   template <class T, class T2, int32_t N1, int32_t N2>
   inline void copy_array(T(&src)[N1], T2(&dest)[N2])
   {
      if constexpr_t (N1 < N2) /// rhs > lhs
         std::copy(std::begin(src), std::end(src), std::begin(dest));
      else
         std::copy(std::begin(src), std::begin(src) + N2, std::begin(dest));
   }

   template <class T, class T2, int32_t N1, int32_t N2>
   inline void copy_array_cont(T(&src)[N1], T2(&dest)[N2])
   {
      if constexpr_t (N1 < N2) /// rhs > lhs
         std::copy(src.begin(), src.end(), std::begin(dest));
      else
         std::copy(src.begin(), src.end() + N2, std::begin(dest));
   }

   /// @file buffer.h
   /// @brief A buffer used for encoding PDUs.
   template <typename T, size_type SIZE>
   struct static_buffer
   {
      public:
         //using array_t = misc::array<T, SIZE>;
         using type = misc::static_buffer<T, SIZE>;

         using iterator        = T*;
         using const_iterator  = const T*;
         using reference       = T&;
         using const_reference = const T&;

         //constexpr_t static_buffer() = default;
         constexpr_t static_buffer() = default;

         template <typename T2, typename SZ>
         explicit constexpr_t static_buffer(T2* buf, SZ len)
         {
            for (SZ i = 0; i < SIZE && i < len; ++i)
               buffer[i] = buf[i];
         }

         constexpr_t static_buffer(const T& val) { fill_n(SIZE, val); }
         constexpr_t static_buffer(const_iterator b, const_iterator e);

         template <typename T2, int32_t N>
         explicit static_buffer(T2(&arr)[N])
         {
            if constexpr_t (N < SIZE)
            {
               std::copy(std::begin(arr), std::end(arr), buffer);
               offset = N;
            }
            else
            {
               std::copy(std::begin(arr), std::begin(arr) + SIZE, buffer);
               offset = SIZE;
            }
         }

         explicit static_buffer(const string &str)
         {
            if (str.size() < SIZE)
            {
               std::copy(std::begin(str), std::end(str), buffer);
               offset = str.size();
            }
            else
            {
               std::copy(std::begin(str), std::begin(str) + SIZE, buffer);
               offset = SIZE;
            }
         }

         explicit static_buffer(string_view_t str)
         {
            if (str.size() < SIZE)
            {
               std::copy(std::begin(str), std::end(str), buffer);
               offset = str.size();
            }
            else
            {
               std::copy(std::begin(str), std::begin(str) + SIZE, buffer);
               offset = SIZE;
            }
         }

         template <class T2, size_type N>
         constexpr_t static_buffer(std::array<T2, N>& init) { for (T2 v : init) memcpy_n_incr(v); }

         constexpr_t static_buffer(static_buffer&& rhs) : offset(rhs.offset) {
            copy_array(rhs.buffer, buffer);
         }

         constexpr_t static_buffer(const static_buffer& rhs) : offset(rhs.offset) {
            copy_array(rhs.buffer, buffer);
         }

         constexpr_t static_buffer& operator=(static_buffer&& rhs) {
            offset = rhs.offset;
            copy_array(rhs.buffer, buffer);
            return *this;
         }

         constexpr_t static_buffer& operator=(const static_buffer& rhs) {
            offset = rhs.offset;
            copy_array(rhs.buffer, buffer);
            return *this;
         }
         //constexpr_t static_buffer(T&& val) { this->fill(val); }

         template <typename T2, size_type N>
         static_buffer& operator=(T2(&arr)[N])
         {
            for (T&& v : arr)
               memcpy_n_incr(v);

            return *this;
         }

         template <class T2, size_type N>
         static_buffer& operator=(std::array<T2, N>&&) noexcept;

         template <class T2, size_type N>
         static_buffer& operator=(std::array<T2, N>& );

         static_buffer& append(const uint8_t val)   { *this += val; return *this; }
         static_buffer& append(const uint16_t val)  { return memcpy_n_incr(val); }
         static_buffer& append(const uint32_t val)  { return memcpy_n_incr(val); }
         static_buffer& append_sv(const string_view_t);

         /** Erases the given range, then copy values in @src to the range in @buffer */
         template <class T2 = std::make_unsigned_t<T>>
         void assign(const T* src, size_t b, size_t e)
         {
            erase(b, e);
            for (size_t i = 0; b < e; ++b, ++i)
               buffer[b] = src[i];
         }

         /// Assigns the value of src to the offset location in buffer
         /// @param \t src: value to be assigned
         /// @param \t _offset: start location in buffer
         void assign(uint8_t src, size_t _offset) { buffer[_offset] = src; }
         void assign(uint16_t src, size_t _offset) { memcpy(&buffer[_offset], &src, sizeof(uint16_t)); }
         void assign(uint32_t src, size_t _offset) { memcpy(&buffer[_offset], &src, sizeof(uint32_t)); }

         /** Erases the given range in buffer, then copy values in @src to the range in @buffer */
         template <class T2 = std::make_unsigned_t<T>>
         void assign_n(T2* src, size_t sz, size_t b, size_t e)
         {
            erase(b, e);
            for (size_t i = 0; ((i < sz) && (b < e)); ++b, ++i)
               buffer[b] = src[i];
         }

         iterator begin() { return &buffer[0]; }
         const_iterator begin()  const  { return &buffer[0]; }
         const_iterator cbegin() const  { return &buffer[0]; }

         iterator end() { return &buffer[offset]; }
         const_iterator end()  const  { return &buffer[offset]; }
         const_iterator cend() const  { return &buffer[offset]; }

         reference operator[](size_t i) { return buffer[i]; }
         const_reference operator[](size_t i) const { return buffer[i]; }

         void fill(T value) { this->fill_n(SIZE, value); }
         //void fill_n(size_t sz, T value) { std::fill_n(begin(), sz, value); }
         void fill_n(size_t sz, T value) { memset(buffer, value, sz * sizeof(T)); }

         void clear() { fill(0); offset = 0; }
         bool empty() const { return (SIZE == 0); }

         void erase(size_t b, size_t e)
         {
            for (; b < e; ++b) buffer[b] = 0;
         }

         void erase(iterator b, iterator e)
         {
            for (; b < e;)
               *b++ = 0;
         }

         template <class T2, size_type N = SIZE>
         T2 get(size_type _offset) const
         {
            T2 ret = 0;
            memcpy(&ret, &buffer[_offset], sizeof(T2));
            return ret;
         }

         constexpr_t uint16_t get_u16(size_type _offset) { return get<uint16_t, sizeof(uint16_t)>(_offset); }
         constexpr_t uint32_t get_u32(size_type _offset) const { return get<uint32_t, sizeof(uint32_t)>(_offset); }

         static_buffer& operator+=(const uint8_t p);
         static_buffer& operator+=(const uint16_t p) { return memcpy_n_incr(p); }
         static_buffer& operator+=(const uint32_t p) { return memcpy_n_incr(p); }
         static_buffer& operator+=(string_view_t& p)   { return append_sv(p); }

         operator T*() { return buffer;}
         operator const T*() const { return buffer;}
         operator basic_string_view<T> () { return buffer;}

         const char* c_str() const { return reinterpret_cast<const char*>(buffer); }

         static_buffer& add_octet_array(const uint32_t& length, const uint8_t* arr);

         size_type buffer_offset() const { return offset; }
         T* data() { return buffer; }
         const T* data() const { return buffer; }

         /// Returns Maximum size of the buffer
         constexpr_t size_type capacity() const { return SIZE; }
         constexpr_t size_type max_size() const { return SIZE; }

         /// Returns the number of elements in the buffer
         size_type size() const
         {
            size_type i;
            for (i = capacity() - 1; i > 0; --i)
            {
               if (buffer[i] != 0)
               {
                  return i + 1;
               }
            }
            return i;
         }

         /// Returns the number of elements in the buffer
         size_type length()  const { return offset; }

         void set_values(std::initializer_list<T> lst) {  for (T v : lst) memcpy_n_incr(static_cast<T>(v)); }

      public:
         size_type offset = 0;
         T buffer[SIZE]{0};
         //uint32_t offset = 0;

      public:
         template <class TVal>
         static_buffer& memcpy_n_incr(const TVal val)
         {
            ::memcpy(&buffer[offset], &val, sizeof(val));
            offset += sizeof(val);
            return *this;
         }
   };

   template <typename T, size_type SIZE>
   constexpr_t static_buffer<T, SIZE>::static_buffer(const_iterator b, const_iterator e)
   {
      for (; b != e;)
         memcpy_n_incr(*b++);
   }

   /*template <typename T, size_t SIZE>
   template <typename T2>
   constexpr_t static_buffer<T, SIZE>::static_buffer(const std::initializer_list<T2>& init)
   {
   #ifdef CUAP_DEBUG

      fmt::print("Using initializer_list \n");
      ///fmt::print("initializer_list Size: {}\n", sizeof(init));
      ///fmt::print("initializer_list elements: {}\n", init.size());
   #endif // CUAP_DEBUG
      for (T&& v : init)
         memcpy_n_incr(v);
   }*/

   template <typename T, size_type SIZE>
   template <class T2, size_type N>
   static_buffer<T, SIZE>& static_buffer<T, SIZE>::operator=(std::array<T2, N>&& arr) noexcept
   {
#ifdef CUAP_DEBUG
      fmt::print("Array&& Size: {}\n", sizeof(arr));
#endif // CUAP_DEBUG
      for (T&& v : arr)
         memcpy_n_incr(v);

      return *this;
   }

   template <typename T, size_type SIZE>
   template <class T2, size_type N>
   static_buffer<T, SIZE>& static_buffer<T, SIZE>::operator=(std::array<T2, N>& arr)
   {
#ifdef CUAP_DEBUG
      fmt::print("Array Size: {}\n", sizeof(arr));
#endif
      for (T&& v : arr)
         memcpy_n_incr(v);

      return *this;
   }

   template <typename T, size_type SIZE>
   inline static_buffer<T, SIZE>& static_buffer<T, SIZE>::operator+=(const uint8_t p)
   {
      buffer[offset++] = p;
      return *this;
   }

   template <typename T, size_type SIZE>
   inline static_buffer<T, SIZE>& static_buffer<T, SIZE>::append_sv(const string_view_t p)
   {
      std::copy(p.begin(), p.end(), buffer + offset);
      offset += p.length();
      //buffer[offset - 1] = 0; // Null terminator
      return *this;
   }

   template <typename T, size_type SIZE>
   class static_buffer_char;
}

namespace misc
{

   /// @file buffer.h
   /// @brief A buffer used for encoding PDUs.
   template <typename T, size_type SIZE = 32>
   struct dynammic_buffer
   {
      public:
         using type = misc::dynammic_buffer<T, SIZE>;

         using iterator        = T*;
         using const_iterator  = const T*;
         using reference       = T&;
         using const_reference = const T&;


      public:
         size_type offset = 0;
         std::basic_string<uint8_t> buffer;

      public:
         template <class TVal>
         dynammic_buffer& memcpy_n_incr(const TVal val)
         {
            ::memcpy(&buffer[offset], &val, sizeof(val));
            offset += sizeof(val);
            return *this;
         }
   };

}

#endif//buffer_h

