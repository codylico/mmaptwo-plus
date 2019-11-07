/*
 * \file mmapio.hpp
 * \brief Memory-mapped files
 * \author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_MMapIO_Plus_mmapIo_H_
#define hg_MMapIO_Plus_mmapIo_H_

#include <cstddef>

#ifdef MMAPIO_PLUS_WIN32_DLL
#  ifdef MMAPIO_PLUS_WIN32_DLL_INTERNAL
#    define MMAPIO_PLUS_API __declspec(dllexport)
#  else
#    define MMAPIO_PLUS_API __declspec(dllimport)
#  endif /*MMAPIO_PLUS_WIN32_DLL_INTERNAL*/
#else
#  define MMAPIO_PLUS_API
#endif /*MMAPIO_PLUS_WIN32_DLL*/

/**
 * \brief Memory-mapped files library
 */
namespace mmapio {
  using std::size_t;

  /**
   * \brief Operating system identifier.
   */
  enum os {
    os_none = 0,
    os_unix = 1,
    os_win32 = 2
  };

  /**
   * \brief File memory access modes.
   */
  enum mode {
    mode_read = 0x72,
    mode_write = 0x77,
    mode_end = 0x65,
    mode_private = 0x70,
    mode_bequeath = 0x71 /* TODO check API symbol */
  };

  /**
   * \brief Memory-mapped input-output interface.
   */
  MMAPIO_PLUS_API
  class mmapio_i {
  public:
    /**
     * \brief Destructor; closes the file and frees the space.
     */
    virtual ~mmapio_i(void) = 0;

    /**
     * \brief Acquire a lock to the space.
     * \return pointer to locked space on success, NULL otherwise
     */
    virtual void* acquire(void) = 0;

    /**
     * \brief Release a lock of the space.
     * \param p pointer of region to release
     */
    virtual void release(void* p) = 0;

    /**
     * \brief Check the length of the mapped area.
     * \return the length of the mapped region exposed by this interface
     */
    virtual size_t length(void) const = 0;
  };

  /* BEGIN configurations */
  /**
   * \brief Check the library's target backend.
   * \return a \link mmapio::os \endlink value
   */
  MMAPIO_PLUS_API
  int get_os(void);
  /* END   configurations */

  /* BEGIN open functions */
  /**
   * \brief Open a file using a narrow character name.
   * \param nm name of file to map
   * \param mode one of 'r' (for readonly) or 'w' (writeable),
   *   optionally followed by 'e' to extend map to end of file,
   *   optionally followed by 'p' to make write changes private
   * \param sz size in bytes of region to map
   * \param off file offset of region to map
   * \param throwing whether to pass on exceptions to the caller
   * \return an interface on success, NULL otherwise
   * \note On Windows, this function uses `CreateFileA` directly.
   * \note On Unix, this function uses the `open` system call directly.
   */
  MMAPIO_PLUS_API
  mmapio_i* open
    ( char const* nm, char const* mode, size_t sz, size_t off,
      bool throwing=true);

  /**
   * \brief Open a file using a UTF-8 encoded name.
   * \param nm name of file to map
   * \brief mode one of 'r' (for readonly) or 'w' (writeable),
   *   optionally followed by 'e' to extend map to end of file,
   *   optionally followed by 'p' to make write changes private
   * \param sz size in bytes of region to map
   * \param off file offset of region to map
   * \param throwing whether to pass on exceptions to the caller
   * \return an interface on success, NULL otherwise
   * \note On Windows, this function re-encodes the `nm` parameter from
   *   UTF-8 to UTF-16, then uses `CreateFileW` on the result.
   * \note On Unix, this function uses the `open` system call directly.
   */
  MMAPIO_PLUS_API
  mmapio_i* u8open
    ( unsigned char const* nm, char const* mode, size_t sz, size_t off,
      bool throwing=true);

  /**
   * \brief Open a file using a wide character name.
   * \param nm name of file to map
   * \brief mode one of 'r' (for readonly) or 'w' (writeable),
   *   optionally followed by 'e' to extend map to end of file,
   *   optionally followed by 'p' to make write changes private
   * \param sz size in bytes of region to map
   * \param off file offset of region to map
   * \param throwing whether to pass on exceptions to the caller
   * \return an interface on success, NULL otherwise
   * \note On Windows, this function uses `CreateFileW` directly.
   * \note On Unix, this function translates the wide string
   *   to a multibyte character string, then passes the result to
   *   the `open` system call. Use `setlocale` in advance if necessary.
   */
  MMAPIO_PLUS_API
  mmapio_i* wopen
    ( wchar_t const* nm, char const* mode, size_t sz, size_t off,
      bool throwing=true);
  /* END   open functions */
};

#endif /*hg_MMapIO_Plus_mmapIo_H_*/
