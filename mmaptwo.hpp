/*
 * \file mmaptwo.hpp
 * \brief Memory-mapped files
 * \author Cody Licorish (svgmovement@gmail.com)
 */
#ifndef hg_MMapTwo_Plus_mmapTwo_H_
#define hg_MMapTwo_Plus_mmapTwo_H_

#include <cstddef>

#ifdef MMAPTWO_PLUS_WIN32_DLL
#  ifdef MMAPTWO_PLUS_WIN32_DLL_INTERNAL
#    define MMAPTWO_PLUS_API __declspec(dllexport)
#  else
#    define MMAPTWO_PLUS_API __declspec(dllimport)
#  endif /*MMAPTWO_PLUS_WIN32_DLL_INTERNAL*/
#else
#  define MMAPTWO_PLUS_API
#endif /*MMAPTWO_PLUS_WIN32_DLL*/

/**
 * \brief Memory-mapped files library
 */
namespace mmaptwo {
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

    /**
     * \note If not using bequeath, the caller of
     *   \link mmaptwo::open \endlink, \link mmaptwo::u8open \endlink or
     *   \link mmaptwo::wopen \endlink must give time for the function
     *   to return. Otherwise, the file descriptor of the mapped file
     *   may leak.
     */
    mode_bequeath = 0x71
  };

  /**
   * \brief Memory reading part of memory-mapped input-output interface.
   */
  MMAPTWO_PLUS_API
  class page_i {
  public:
    /**
     * \brief Destructor; frees the space.
     * \note The source map instance, which holds the file descriptor,
     *   remains unaffected by this function.
     */
    virtual ~page_i(void) = 0;

    /**
     * \brief Get a pointer to the space.
     * \return pointer to space on success, NULL otherwise
     */
    virtual void* get(void) = 0;

    /**
     * \brief Get a pointer to the space.
     * \return pointer to space on success, NULL otherwise
     */
    virtual void const* get(void) const = 0;

    /**
     * \brief Check the length of the mapped area.
     * \return the length of the mapped region exposed by this interface
     */
    virtual size_t length(void) const = 0;

    /**
     * \brief Check the offset of the mapped area.
     * \return the offset of the mapped region exposed by this interface
     * \note Offset is measured from start of source mappable.
     */
    virtual size_t offset(void) const = 0;
  };

  /**
   * \brief File acquisition part of memory-mapped input-output interface.
   */
  MMAPTWO_PLUS_API
  class mmaptwo_i {
  public:
    /**
     * \brief Destructor; closes the file.
     * \note Will not free any acquired pages!
     */
    virtual ~mmaptwo_i(void) = 0;

    /**
     * \brief Acquire a mapping to the space.
     * \param siz size of the map to acquire
     * \param off offset into the file data
     * \return pointer to a page interface on success, NULL otherwise
     */
    virtual page_i* acquire(size_t siz, size_t off) = 0;

    /**
     * \brief Check the length of the mappable area.
     * \return the length of the mappable region exposed by this interface
     */
    virtual size_t length(void) const = 0;

    /**
     * \brief Check the length of the mappable area.
     * \return the length of the mappable region exposed by this interface
     */
    virtual size_t offset(void) const = 0;
  };


  /* BEGIN configurations */
  /**
   * \brief Check the library's target backend.
   * \return a \link mmaptwo::os \endlink value
   */
  MMAPTWO_PLUS_API
  int get_os(void);

  /**
   * \brief Check whether the library can handle possible race conditions
   *   involving file bequeath prevention. Such prevention may be necessary
   *   when starting child processes.
   * \return nonzero if file bequeath prevention is race-proof, zero
   *   otherwise
   */
  MMAPTWO_PLUS_API
  bool check_bequeath_stop(void);

  /**
   * \brief Check what this library thinks the page size is.
   * \return a page size
   * \note Users of this library should not need this value
   *   to use the library.
   */
  MMAPTWO_PLUS_API
  size_t get_page_size(void);
  /* END   configurations */

  /* BEGIN open functions */
  /**
   * \brief Open a file using a narrow character name.
   * \param nm name of file to map
   * \param mode one of 'r' (for readonly) or 'w' (writeable),
   *   optionally followed by 'e' to extend map to end of file,
   *   optionally followed by 'p' to make write changes private
   * \param sz size in bytes of region to provide for mapping
   * \param off file offset of region to provide for mapping
   * \param throwing whether to pass on exceptions to the caller
   * \return an interface on success, NULL otherwise
   * \note On Windows, this function uses `CreateFileA` directly.
   * \note On Unix, this function uses the `open` system call directly.
   */
  MMAPTWO_PLUS_API
  mmaptwo_i* open
    ( char const* nm, char const* mode, size_t sz, size_t off,
      bool throwing=true);

  /**
   * \brief Open a file using a UTF-8 encoded name.
   * \param nm name of file to map
   * \brief mode one of 'r' (for readonly) or 'w' (writeable),
   *   optionally followed by 'e' to extend map to end of file,
   *   optionally followed by 'p' to make write changes private
   * \param sz size in bytes of region to provide for mapping
   * \param off file offset of region to provide for mapping
   * \param throwing whether to pass on exceptions to the caller
   * \return an interface on success, NULL otherwise
   * \note On Windows, this function re-encodes the `nm` parameter from
   *   UTF-8 to UTF-16, then uses `CreateFileW` on the result.
   * \note On Unix, this function uses the `open` system call directly.
   */
  MMAPTWO_PLUS_API
  mmaptwo_i* u8open
    ( unsigned char const* nm, char const* mode, size_t sz, size_t off,
      bool throwing=true);

  /**
   * \brief Open a file using a wide character name.
   * \param nm name of file to map
   * \brief mode one of 'r' (for readonly) or 'w' (writeable),
   *   optionally followed by 'e' to extend map to end of file,
   *   optionally followed by 'p' to make write changes private
   * \param sz size in bytes of region to provide for mapping
   * \param off file offset of region to provide for mapping
   * \param throwing whether to pass on exceptions to the caller
   * \return an interface on success, NULL otherwise
   * \note On Windows, this function uses `CreateFileW` directly.
   * \note On Unix, this function translates the wide string
   *   to a multibyte character string, then passes the result to
   *   the `open` system call. Use `setlocale` in advance if necessary.
   */
  MMAPTWO_PLUS_API
  mmaptwo_i* wopen
    ( wchar_t const* nm, char const* mode, size_t sz, size_t off,
      bool throwing=true);
  /* END   open functions */
};

#endif /*hg_MMapTwo_Plus_mmapTwo_H_*/
