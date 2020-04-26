#ifndef io_fmt_h
#define io_fmt_h

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/color.h>
//#include <fmt/printf.h>

using std::ostream;
using std::is_signed;
using std::is_unsigned;
using std::array;

namespace fmt
{
#if FMT_VERSION < 50000
   /// Like BasicMemoryWriter, except size comes in template arg
   template <typename Char, size_t SZ, typename Allocator = std::allocator<Char> >
   class basic_mem_writer : public BasicWriter<Char>
   {
      private:
         internal::MemoryBuffer<Char, SZ, Allocator> buffer_;

      public:
         explicit basic_mem_writer(const Allocator& alloc = Allocator())
            : BasicWriter<Char>(buffer_), buffer_(alloc) {}

         basic_mem_writer(basic_mem_writer &&other)
            : BasicWriter<Char>(buffer_), buffer_(std::move(other.buffer_))
         {
         }

         basic_mem_writer &operator=(basic_mem_writer &&other)
         {
            buffer_ = std::move(other.buffer_);
            return *this;
         }

         size_t max_size() { return SZ; }
   };

   template <class T, size_t N>
   using mem_writer_generic = basic_mem_writer<T, N>;
#elif FMT_VERSION >= 50000 && FMT_VERSION < 60000
   using namespace fmt::v5;
   template <class T, int SZ>
   using basic_mem_writer = fmt::v5::basic_memory_buffer<T, SZ>;

#endif

#if FMT_VERSION < 60000
   using mem_writer32_t = basic_mem_writer<char, 32>;
   using mem_writer40_t = basic_mem_writer<char, 40>;
   using mem_writer64_t = basic_mem_writer<char, 64>;

   using mem_writer128_t = basic_mem_writer<char, 128>;
   using mem_writer256_t = basic_mem_writer<char, 256>;
   using mem_writer512_t = basic_mem_writer<char, 512>;

   using mem_writer1KB_t = basic_mem_writer<char, 1024>;

   using umem_writer40_t  = basic_mem_writer<unsigned char, 40>;
   using umem_writer512_t = basic_mem_writer<unsigned char, 512>;
#endif
}

#ifdef ENABLE_DEPRECATED_COLORS
namespace fmt
{
   using namespace v5;
   using fmt::color;
   //using CStringRef = fmt::string_view;
//#ifdef
   template <class ...T>
   void print_blue(const char* frmt, T&&... args)
   {
      print_colored(color::blue, frmt, args...);
   }
   template <class ...T>
   void print_cyan(const char* frmt, T&&... args)
   {
      print_colored(color::cyan, frmt, args...);
   }

   template <class ...T>
   void print_green(const char* frmt, T&&... args)
   {
      print_colored(color::green, frmt, args...);
   }

   template <class ...T>
   inline void print_magenta(const char* frmt, T&&... args)
   {
      print_colored(color::magenta, frmt, args...);
   }

   template <class ...T>
   inline void print_red(const char* frmt, T&&... args)
   {
      print_colored(color::red, frmt, args...);
   }

   template <class ...T>
   inline void print_white(const char* frmt, T&&... args)
   {
      print_colored(color::white, frmt, args...);
   }

   template <class ...T>
   inline void print_yellow(const char* frmt, T&&... args)
   {
      print_colored(fmt::v5::color::yellow, frmt, args...);
   }

   template <class ...Args>
   void print_base(fmt::v5::string_view formt, Args&&...args)
   {
      print("{}", format(formt, args...));
   }

   template <class ...Args>
   void print_base_w_color(fmt::color color, fmt::v5::string_view formt, Args&&...args)
   {
      print_colored(color, "{}", format(formt, args...));
   }
}
#endif // ENABLE_DEPRECATED_COLORS

namespace fmt
{
   //using CStringRef = fmt::string_view;
#if FMT_VERSION >= 50000 && FMT_VERSION < 60000
   template <class ...T>
   void print_blue(const char* frmt, T&&... args)
   {
      print(fmt::fg(fmt::color::blue), frmt, args...);
   }

   template <class ...T>
   void print_cyan(const char* frmt, T&&... args)
   {
      fmt::print(fmt::fg(fmt::color::cyan), frmt, args...);
   }

   template <class ...T>
   void print_green(const char* frmt, T&&... args)
   {
      print(fmt::fg(fmt::color::green), frmt, args...);
   }

   template <class ...T>
   inline void print_magenta(const char* frmt, T&&... args)
   {
      print(fmt::v5::fg(fmt::color::magenta), frmt, args...);
   }

   template <class ...T>
   inline void print_red(const char* frmt, T&&... args)
   {
      print(fmt::fg(fmt::color::red), frmt, args...);
   }

   template <class ...T>
   inline void print_white(const char* frmt, T&&... args)
   {
      print(fmt::fg(fmt::color::white), frmt, args...);
   }

   template <class ...T>
   inline void print_yellow(const char* frmt, T&&... args)
   {
      print(fmt::fg(fmt::color::yellow), frmt, args...);
   }

   template <class ...Args>
   void print_base(fmt::v5::string_view formt, Args&&...args)
   {
      print("{}", format(formt, args...));
   }

   template <class ...Args>
   void print_base_w_style(fmt::text_style style, fmt::v5::string_view formt, Args&&...args)
   {
      print(style, "{}", format(formt, args...));
   }
#endif
}

namespace io_fmt = fmt;

#endif//io_fmt_h

 
