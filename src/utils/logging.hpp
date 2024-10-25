#include <cassert>
#include <Geode/loader/Log.hpp>

namespace log
{
  struct FmtStrWithLocation
  {
    std::string_view str;
#ifndef NDEBUG
    const std::source_location location;
#endif

#ifndef NDEBUG
    FmtStrWithLocation(const char* str, const std::source_location& location =
                                            std::source_location::current())
        : str(str), location(location)
    {
    }
#else
    FmtStrWithLocation(const char* str) : str(str) {}
#endif

    template <typename Fn, typename... Args>
    void logTo(Fn&& func, Args&&... args)
    {
#ifndef NDEBUG
      func("file: {}({}:{}) `{}`: {}", location.file_name(), location.line(),
           location.column(), location.function_name(),
           fmt::vformat(str, fmt::make_format_args(args...)));
#else
      func(str, std::forward<Args>(args)...);
#endif
    }
  };

#define __gdmx_gen_log_wrapper_impl(logname, argstype, argsvar, formatvar)     \
  []<typename... argstype>(geode::log::impl::FmtStr<argstype...> formatvar,    \
                           argstype&&... argsvar)                              \
  { geode::log::logname(formatvar, std::forward<argstype>(argsvar)...); }

#define __gdmx_gen_log_wrapper(logname)                                        \
  __gdmx_gen_log_wrapper_impl(logname, GEODE_CONCAT(Args, __LINE__),           \
                              GEODE_CONCAT(args, __LINE__),                    \
                              GEODE_CONCAT(format, __LINE__))

  template <typename... Args>
  void error(FmtStrWithLocation format, Args&&... args)
  {
    format.logTo(__gdmx_gen_log_wrapper(error), std::forward<Args>(args)...);
  }

  template <typename... Args>
  void warn(FmtStrWithLocation format, Args&&... args)
  {
    format.logTo(__gdmx_gen_log_wrapper(warn), std::forward<Args>(args)...);
  }

  template <typename... Args>
  void debug(FmtStrWithLocation format, Args&&... args)
  {
    format.logTo(__gdmx_gen_log_wrapper(debug), std::forward<Args>(args)...);
  }

#undef __gdmx_gen_log_wrapper_impl
#undef __gdmx_gen_log_wrapper
} // namespace log
