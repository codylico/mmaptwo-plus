/*
 * \file mmaptwo.cpp
 * \brief Memory-mapped files
 * \author Cody Licorish (svgmovement@gmail.com)
 */
#define MMAPTWO_PLUS_WIN32_DLL_INTERNAL
#define _POSIX_C_SOURCE 200809L
#include "mmaptwo.hpp"
#include <cstdlib>
#include <stdexcept>

namespace mmaptwo {
  /**
   * \brief Mode tag for mmaptwo interface, holding various
   *   mapping configuration values.
   */
  struct mode_tag {
    /** \brief access mode (readonly, read-write) */
    char mode;
    /** \brief map-to-end-of-file flag */
    char end;
    /** \brief flag for activating private mapping */
    char privy;
    /** \brief flag for enabling access from child processes */
    char bequeath;
  };

  /**
   * \brief Extract a mmaptwo mode tag from a mode text.
   * \param mmode the text to parse
   * \return a mmaptwo mode tag
   */
  static struct mode_tag mode_parse(char const* mmode);
};

#define MMAPTWO_OS_UNIX 1
#define MMAPTWO_OS_WIN32 2

/*
 * inspired by https://stackoverflow.com/a/30971057
 * and https://stackoverflow.com/a/11351171
 */
#ifndef MMAPTWO_PLUS_OS
#  if (defined _WIN32)
#    define MMAPTWO_PLUS_OS MMAPTWO_OS_WIN32
#  elif (defined __unix__) || (defined(__APPLE__)&&defined(__MACH__))
#    define MMAPTWO_PLUS_OS MMAPTWO_OS_UNIX
#  else
#    define MMAPTWO_PLUS_OS 0
#  endif
#endif /*MMAPTWO_PLUS_OS*/

#if MMAPTWO_PLUS_OS == MMAPTWO_OS_UNIX
#  include <unistd.h>
#  if (defined __cplusplus) && (__cplusplus >= 201103L)
#    include <cwchar>
#    include <cstring>
#  endif /*__cplusplus*/
#  include <fcntl.h>
#  include <sys/mman.h>
#  include <sys/stat.h>
#  include <cerrno>
#  include <limits>

namespace mmaptwo {
  class mmaptwo_unix;

  /**
   * \brief Page handler structure for POSIX mmaptwo implementation.
   */
  MMAPTWO_PLUS_API
  class page_unix : public page_i {
  private:
    /** \brief `mmap` pointer */
    void* ptr;
    /** \brief length of space */
    size_t len;
    /** \brief offset from `ptr` to start of user-requested space */
    size_t shift;
    /** \brief offset from start of source to user-requested space */
    size_t offnum;

    page_unix(page_unix const& ) = delete;
    page_unix& operator=(page_unix const& ) = delete;
    page_unix(void) = delete;

  public:
    /**
     * \brief Complete acquisition of a page of space.
     * \param fd file descriptor
     * \param mt mode tags
     * \param sz size of page instance to request
     * \param off offset of page from start of file
     * \param pre_off nominal offset
     * \throws `std::length_error` and `std::runtime_error`
     */
    page_unix
      (int fd, struct mode_tag mt, size_t sz, size_t off, size_t pre_off);

    /**
     * \brief Destructor; frees the space.
     * \note The source map instance, which holds the file descriptor,
     *   remains unaffected by this function.
     */
    ~page_unix(void) override;

    /**
     * \brief Get a pointer to the space.
     * \return pointer to space
     */
    void* get(void) override;

    /**
     * \brief Get a pointer to the space.
     * \return pointer to space
     */
    void const* get(void) const override;

    /**
     * \brief Check the length of the mapped area.
     * \return the length of the mapped region exposed by this interface
     */
    size_t length(void) const override;

    /**
     * \brief Check the offset of the mapped area.
     * \return the offset of the mapped region exposed by this interface
     * \note Offset is measured from start of source mappable.
     */
    size_t offset(void) const override;
  };

  /**
   * \brief File handler structure for POSIX mmaptwo implementation.
   */
  MMAPTWO_PLUS_API
  class mmaptwo_unix : public mmaptwo_i {
  private:
    /** \brief length of space */
    size_t len;
    /** \brief offset from start of file to user-requested source */
    size_t offnum;
    /** \brief file descriptor */
    int fd;
    /** \brief open attributes */
    struct mode_tag mt;

    mmaptwo_unix(mmaptwo_unix const& ) = delete;
    mmaptwo_unix& operator=(mmaptwo_unix const& ) = delete;
    mmaptwo_unix(void) = delete;

  public:
    /**
     * \brief Finish preparing a memory map interface.
     * \param fd file descriptor
     * \param mmode mode tag
     * \param sz size of range to map
     * \param off offset from start of file
     * \throws `std::runtime_error`
     */
    mmaptwo_unix
      (int fd, struct mode_tag const mmode, size_t sz, size_t off);

    /**
     * \brief Destructor; closes the file.
     * \note Closing the file should not affect active pages.
     */
    ~mmaptwo_unix(void) override;

  public:
    /**
     * \brief Acquire a mapping to the space.
     * \param siz size of the map to acquire
     * \param off offset into the file data
     * \return pointer to a page interface on success, `nullptr` otherwise
     * \throws `std::invalid_argument`, `std::length_error`,
     *   and `std::runtime_error`
     */
    page_i* acquire(size_t siz, size_t off) override;

    /**
     * \brief Check the length of the mappable area.
     * \return the length of the mappable region exposed by this interface
     */
    size_t length(void) const override;

    /**
     * \brief Check the offset of the mappable area.
     * \return the offset of the mappable region from start of file
     */
    size_t offset(void) const override;
  };

  /**
   * \brief Convert a wide string to a multibyte string.
   * \param nm the string to convert
   * \return a multibyte string on success, `nullptr` otherwise
   */
  static char* wctomb(wchar_t const* nm);

  /**
   * \brief Convert a mmaptwo mode character to a POSIX `open` flag.
   * \param mmode the character to convert
   * \return an `open` flag on success, zero otherwise
   */
  static int mode_rw_cvt(int mmode);

  /**
   * \brief Convert a mmaptwo mode character to a POSIX `mmap`
   *   protection flag.
   * \param mmode the character to convert
   * \return an `mmap` protection flag on success, zero otherwise
   */
  static int mode_prot_cvt(int mmode);

  /**
   * \brief Convert a mmaptwo mode character to a POSIX `mmap` others' flag.
   * \param mprivy the private flag to convert
   * \return an `mmap` others' flag on success, zero otherwise
   */
  static int mode_flag_cvt(int mprivy);

  /**
   * \brief Fetch a file size from a file descriptor.
   * \param fd target file descriptor
   * \return a file size, or zero on failure
   */
  static size_t file_size_e(int fd);
};

#elif MMAPTWO_PLUS_OS == MMAPTWO_OS_WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <climits>
#  include <cerrno>
#  include <cstring>

namespace mmaptwo {
  /**
   * \brief File handler structure for Win32 API mmaptwo implementation.
   */
  MMAPTWO_PLUS_API
  class mmaptwo_win32 : public mmaptwo_i {
  private:
    /** \brief length of space, including unrequested leading bytes */
    size_t len;
    /** \brief file mapping handle */
    HANDLE fmd;
    /** \brief file handle */
    HANDLE fd;
    /** \brief user-requested offset from start of file */
    size_t offnum;
    /** \brief open attributes */
    struct mode_tag mt;

  public:
    /**
     * \brief Finish preparing a memory map interface.
     * \param fd file handle
     * \param mmode mode text
     * \param sz size of range to map
     * \param off offset from start of file
     * \throws `std::invalid_argument`, `std::range_error`,
     *   and `std::runtime_error`
     */
    mmaptwo_win32
      (HANDLE fd, struct mode_tag const mmode, size_t sz, size_t off);

    /**
     * \brief Destructor; closes the file.
     * \note This destructor does not destroy, and should not invalidate,
     *   any active pages.
     */
    ~mmaptwo_win32(void) override;

  public:
    /**
     * \brief Acquire a mapping to the space.
     * \param siz size of the map to acquire
     * \param off offset into the file data
     * \return pointer to a page interface on success, `nullptr` otherwise
     * \throws `std::invalid_argument`, `std::runtime_error`,
     *   and `std::length_error`
     */
    page_i* acquire(size_t siz, size_t off) override;

    /**
     * \brief Check the length of the mapped area.
     * \param m map instance
     * \return the length of the mapped region exposed by this interface
     */
    size_t length(void) const override;

    /**
     * \brief Check the length of the mappable area.
     * \return the length of the mappable region exposed by this interface
     */
    size_t offset(void) const override;
  };

  /**
   * \brief Page handler structure for Win32 API mmaptwo implementation.
   */
  MMAPTWO_PLUS_API
  class page_win32 : public page_i {
  private:
    /** \brief `MapViewOfFile` pointer */
    void* ptr;
    /** \brief length of space */
    size_t len;
    /** \brief offset from `ptr` to start of user-requested space */
    size_t shift;
    /** \brief user-requested offset from start of source */
    size_t offnum;

    page_win32(page_win32 const& ) = delete;
    page_win32& operator=(page_win32 const& ) = delete;
    page_win32(void) = delete;

  public:
    /**
     * \brief Finish acquiring a page of the space.
     * \param fmd file module descriptor
     * \param mt mode tags
     * \param sz size of page instance to request
     * \param off offset of page from start of file
     * \param pre_off nominal offset
     * \throws `std::length_error` and `std::runtime_error`
     */
    page_win32
      (HANDLE fmd, struct mode_tag mt, size_t sz, size_t off, size_t pre_off);

    /**
     * \brief Destructor; frees the space.
     * \note The source map instance, which holds the file descriptor,
     *   remains unaffected by this function.
     */
    ~page_win32(void) override;

    /**
     * \brief Get a pointer to the space.
     * \return pointer to space on success
     */
    void* get(void) override;

    /**
     * \brief Get a pointer to the space.
     * \return pointer to space on success
     */
    void const* get(void) const override;

    /**
     * \brief Check the length of the mapped area.
     * \return the length of the mapped region exposed by this interface
     */
    size_t length(void) const override;

    /**
     * \brief Check the offset of the mapped area.
     * \return the offset of the mapped region exposed by this interface
     * \note Offset is measured from start of source mappable.
     */
    size_t offset(void) const override;
  };

  /**
   * \brief Convert a mmaptwo mode character to a `CreateFile.` desired access flag.
   * \param mmode the character to convert
   * \return an `CreateFile.` desired access flag on success, zero otherwise
   */
  static DWORD mode_rw_cvt(int mmode);

  /**
   * \brief Convert a mmaptwo mode character to a `CreateFile.`
   *   creation disposition.
   * \param mmode the character to convert
   * \return a `CreateFile.` creation disposition on success, zero otherwise
   */
  static DWORD mode_disposition_cvt(int mmode);

  /**
   * \brief Convert UTF-8 encoded text to UTF-16 LE text.
   * \param nm file name encoded in UTF-8
   * \param out output string
   * \param outlen output length
   * \return an errno code
   */
  static int u8towc_shim
    (unsigned char const* nm, wchar_t* out, size_t* outlen);

  /**
   * \brief Convert UTF-8 encoded text to UTF-16 LE text.
   * \param nm file name encoded in UTF-8
   * \return a wide string on success, NULL otherwise
   */
  static wchar_t* u8towc(unsigned char const* nm);

  /**
   * \brief Fetch a file size from a file descriptor.
   * \param fd target file handle
   * \return a file size, or zero on failure
   */
  static size_t file_size_e(HANDLE fd);

  /**
   * \brief Convert a mmaptwo mode character to a
   *   `CreateFileMapping.` protection flag.
   * \param mmode the character to convert
   * \return a `CreateFileMapping.` protection flag on success, zero otherwise
   */
  static DWORD mode_prot_cvt(int mmode);

  /**
   * \brief Convert a mmaptwo mode tag to a `MapViewOfFile`
   *   desired access flag.
   * \param mmode the tag to convert
   * \return a `MapViewOfFile` desired access flag on success, zero otherwise
   */
  static DWORD mode_access_cvt(struct mode_tag const mt);
};
#endif /*MMAPTWO_PLUS_OS*/

namespace mmaptwo {
  //BEGIN static functions
  struct mode_tag mode_parse(char const* mmode) {
    struct mode_tag out = { 0, 0, 0, 0 };
    int i;
    for (i = 0; i < 8; ++i) {
      switch (mmode[i]) {
      case 0: /* NUL termination */
        return out;
      case mode_write:
        out.mode = mode_write;
        break;
      case mode_read:
        out.mode = mode_read;
        break;
      case mode_end:
        out.end = mode_end;
        break;
      case mode_private:
        out.privy = mode_private;
        break;
      case mode_bequeath:
        out.bequeath = mode_bequeath;
        break;
      }
    }
    return out;
  }

#if MMAPTWO_PLUS_OS == MMAPTWO_OS_UNIX
  char* wctomb(wchar_t const* nm) {
#if (defined __cplusplus) && (__cplusplus >= 201103L)
    /* use multibyte conversion */
    size_t ns;
    char* out;
    /* try the length */{
      std::mbstate_t mbs;
      wchar_t const* test_nm = nm;
      std::memset(&mbs, 0, sizeof(mbs));
      ns = std::wcsrtombs(nullptr, &test_nm, 0, &mbs);
    }
    if (ns == static_cast<size_t>(-1)
    &&  ns == std::numeric_limits<size_t>::max()) {
      /* conversion error caused by bad sequence, so */return nullptr;
    }
    out = static_cast<char*>(calloc(ns+1, sizeof(char)));
    if (out) {
      std::mbstate_t mbs;
      wchar_t const* test_nm = nm;
      std::memset(&mbs, 0, sizeof(mbs));
      std::wcsrtombs(out, &test_nm, ns+1, &mbs);
      out[ns] = 0;
    }
    return out;
#else
    /* no thread-safe version, so give up */
    return nullptr;
#endif /*__STDC_VERSION__*/
  }

  int mode_rw_cvt(int mmode) {
#if (defined O_CLOEXEC)
    int constexpr fast_no_bequeath = static_cast<int>(O_CLOEXEC);
#else
    int constexpr fast_no_bequeath = 0;
#endif /*O_CLOEXEC*/
    switch (mmode) {
    case mode_write:
      return O_RDWR|fast_no_bequeath;
    case mode_read:
      return O_RDONLY|fast_no_bequeath;
    default:
      return 0;
    }
  }

  int mode_prot_cvt(int mmode) {
    switch (mmode) {
    case mode_write:
      return PROT_WRITE|PROT_READ;
    case mode_read:
      return PROT_READ;
    default:
      return 0;
    }
  }

  int mode_flag_cvt(int mprivy) {
    return mprivy ? MAP_PRIVATE : MAP_SHARED;
  }

  size_t file_size_e(int fd) {
    struct stat fsi;
    std::memset(&fsi, 0, sizeof(fsi));
    /* stat pull */{
      int const res = ::fstat(fd, &fsi);
      if (res != 0) {
        return 0u;
      } else return (size_t)(fsi.st_size);
    }
  }
#elif MMAPTWO_PLUS_OS == MMAPTWO_OS_WIN32
  DWORD mode_rw_cvt(int mmode) {
    switch (mmode) {
    case mode_write:
      return GENERIC_READ|GENERIC_WRITE;
    case mode_read:
      return GENERIC_READ;
    default:
      return 0;
    }
  }
  DWORD mode_disposition_cvt(int mmode) {
    switch (mmode) {
    case mode_write:
      return OPEN_ALWAYS;
    case mode_read:
      return OPEN_EXISTING;
    default:
      return 0;
    }
  }

  int u8towc_shim
    (unsigned char const* nm, wchar_t* out, size_t* outlen)
  {
    size_t n = 0;
    unsigned char const* p;
    static size_t const sz_max = UINT_MAX/2u-4u;
    for (p = nm; *p && n < sz_max; ++p) {
      unsigned char const v = *p;
      if (n >= sz_max) {
        return ERANGE;
      }
      if (v < 0x80) {
        /* Latin-1 compatibility */
        if (out) {
          out[n] = v;
        }
        n += 1;
      } else if (v < 0xC0) {
        return EILSEQ;
      } else if (v < 0xE0) {
        /* check extension codes */
        unsigned int i;
        unsigned long int qv = v&31;
        for (i = 0; i < 1; ++i) {
          unsigned char const v1 = *(p+i);
          if (v1 < 0x80 || v1 >= 0xC0) {
            return EILSEQ;
          } else qv = (qv<<6)|(v1&63);
        }
        if (out) {
          out[n] = (wchar_t)qv;
        }
        n += 1;
        p += 1;
      } else if (v < 0xF0) {
        /* check extension codes */
        unsigned int i;
        unsigned long int qv = v&15;
        for (i = 0; i < 2; ++i) {
          unsigned char const v1 = *(p+i);
          if (v1 < 0x80 || v1 >= 0xC0) {
            return EILSEQ;
          } else qv = (qv<<6)|(v1&63);
        }
        if (out) {
          out[n] = (wchar_t)qv;
        }
        n += 1;
        p += 2;
      } else if (v < 0xF8) {
        /* check extension codes */
        unsigned int i;
        unsigned long int qv = v&3;
        for (i = 0; i < 3; ++i) {
          unsigned char const v1 = *(p+i);
          if (v1 < 0x80 || v1 >= 0xC0) {
            return EILSEQ;
          } else qv = (qv<<6)|(v1&63);
        }
        if (qv >= 0x10FFFFL) {
          return EILSEQ;
        }
        if (out) {
          qv -= 0x10000;
          out[n] = (wchar_t)(0xD800 | ((qv>>10)&1023));
          out[n+1] = (wchar_t)(0xDC00 | (qv&1023));
        }
        n += 2;
        p += 3;
      } else {
        return EILSEQ; /* since beyond U+1FFFFF, no valid UTF-16 encoding */
      }
    }
    (*outlen) = n;
    return 0;
  }

  wchar_t* u8towc(unsigned char const* nm) {
    /* use in-house wide character conversion */
    size_t ns;
    wchar_t* out;
    /* try the length */{
      int err = u8towc_shim(nm, NULL, &ns);
      if (err != 0) {
        /* conversion error caused by bad sequence, so */return NULL;
      }
    }
    out = static_cast<wchar_t*>(calloc(ns+1, sizeof(wchar_t)));
    if (out) {
      u8towc_shim(nm, out, &ns);
      out[ns] = 0;
    }
    return out;
  }

  size_t file_size_e(HANDLE fd) {
    LARGE_INTEGER sz;
    BOOL res = GetFileSizeEx(fd, &sz);
    if (res) {
#if (defined ULLONG_MAX)
      return static_cast<size_t>(sz.QuadPart);
#else
      return static_cast<size_t>(sz.u.LowPart)
        |    (static_cast<size_t>(sz.u.HighPart)<<32);
#endif /*ULLONG_MAX*/
    } else return 0u;
  }

  DWORD mode_prot_cvt(int mmode) {
    switch (mmode) {
    case mode_write:
      return PAGE_READWRITE;
    case mode_read:
      return PAGE_READONLY;
    default:
      return 0;
    }
  }

  DWORD mode_access_cvt(struct mode_tag const mt) {
    DWORD flags = 0;
    switch (mt.mode) {
    case mode_write:
      flags = FILE_MAP_READ|FILE_MAP_WRITE;
      break;
    case mode_read:
      flags = FILE_MAP_READ;
      break;
    default:
      return 0;
    }
    if (mt.privy) {
      flags |= FILE_MAP_COPY;
    }
    return flags;
  }
#endif /*MMAPTWO_PLUS_OS*/
  //END   static functions
};

namespace mmaptwo {
  //BEGIN public methods
  mmaptwo_i::~mmaptwo_i(void) {
    return;
  }

  page_i::~page_i(void) {
    return;
  }

#if MMAPTWO_PLUS_OS == MMAPTWO_OS_UNIX
  mmaptwo_unix::mmaptwo_unix
    (int fd, struct mode_tag const mt, size_t sz, size_t off)
  {
    /* assign the close-on-exec flag */{
      int const old_flags = ::fcntl(fd, F_GETFD);
      int bequeath_break = 0;
      if (old_flags < 0) {
        bequeath_break = 1;
      } else if (mt.bequeath) {
        bequeath_break = (::fcntl(fd, F_SETFD, old_flags&(~FD_CLOEXEC)) < 0);
      } else {
        bequeath_break = (::fcntl(fd, F_SETFD, old_flags|FD_CLOEXEC) < 0);
      }
      if (bequeath_break) {
        ::close(fd);
        errno = EDOM;
        throw std::runtime_error
          ( "mmaptwo::mmaptwo_unix::mmaptwo_unix:"
            " bequeath negotiation failure");
      }
    }
    if (mt.end) /* fix map size */{
      size_t const xsz = file_size_e(fd);
      if (xsz < off)
        sz = 0 /*to fail*/;
      else sz = xsz-off;
    }
    if (sz == 0)/* then fail */ {
      ::close(fd);
      errno = EDOM;
      throw std::runtime_error
        ( "mmaptwo::mmaptwo_unix::mmaptwo_unix:"
          " size of zero invalid");
    }
    /* initialize the interface */{
      this->len = sz;
      this->fd = fd;
      this->offnum = off;
      this->mt = mt;
    }
    return;
  }

  mmaptwo_unix::~mmaptwo_unix(void) {
    ::close(this->fd);
    this->fd = -1;
    return;
  }

  page_i* mmaptwo_unix::acquire(size_t sz, size_t pre_off) {
    size_t off;
    /* repair input size and offset */{
      if (pre_off > this->len
      ||  sz > this->len - pre_off
      ||  sz == 0u)
      {
        errno = EDOM;
        throw std::invalid_argument
          ( "mmaptwo::mmaptwo_unix::acquire: "
            "size and offset out of range");
      }
      off = pre_off + this->offnum;
    }
    return new page_unix(fd, mt, sz, off, pre_off);
  }
  page_unix::page_unix
    (int fd, struct mode_tag mt, size_t sz, size_t off, size_t pre_off)
  {
    size_t fullsize;
    size_t fullshift;
    void *ptr;
    off_t fulloff;
    /* fix to page sizes */{
      long const psize = ::sysconf(_SC_PAGE_SIZE);
      fullsize = sz;
      if (psize > 0) {
        /* adjust the offset */
        fullshift = off%((unsigned long)psize);
        fulloff = (off_t)(off-fullshift);
        if (fullshift >= ((~(size_t)0u)-sz)) {
          /* range fix failure */
          errno = ERANGE;
          throw std::length_error
            ("mmaptwo::page_unix::page_unix: range fix failure");
        } else fullsize += fullshift;
      } else fulloff = (off_t)off;
    }
    ptr = ::mmap(nullptr, fullsize, mode_prot_cvt(mt.mode),
         mode_flag_cvt(mt.privy), fd, fulloff);
    if (!ptr) {
      throw std::runtime_error
        ("mmaptwo::page_unix::page_unix: mmap failure");
    }
    /* initialize the interface */{
      this->ptr = ptr;
      this->len = fullsize;
      this->shift = fullshift;
      this->offnum = pre_off;
    }
    return;
  }

  size_t mmaptwo_unix::length(void) const {
    return this->len;
  }

  size_t page_unix::length(void) const {
    return this->len-this->shift;
  }

  size_t mmaptwo_unix::offset(void) const {
    return this->offnum;
  }

  size_t page_unix::offset(void) const {
    return this->offnum;
  }

  page_unix::~page_unix(void) {
    ::munmap(this->ptr, this->len);
    this->ptr = nullptr;
    return;
  }

  void* page_unix::get(void) {
    return static_cast<unsigned char*>(this->ptr)+this->shift;
  }

  void const* page_unix::get(void) const {
    return static_cast<unsigned char const*>(this->ptr)+this->shift;
  }
#elif MMAPTWO_PLUS_OS == MMAPTWO_OS_WIN32
  mmaptwo_win32::mmaptwo_win32
    (HANDLE fd, struct mode_tag const mt, size_t sz, size_t off)
  {
    /*
     * based on
     * https://docs.microsoft.com/en-us/windows/win32/memory/
     *   creating-a-view-within-a-file
     */
    void *ptr;
    size_t fullsize;
    size_t fullshift;
    size_t fulloff;
    size_t extended_size;
    size_t const size_clamp = file_size_e(fd);
    ::HANDLE fmd;
    ::SECURITY_ATTRIBUTES cfmsa;
    if (mt.end) /* fix map size */{
      size_t const xsz = size_clamp;
      if (xsz < off) {
        /* reject non-ending zero parameter */
        ::CloseHandle(fd);
        throw std::invalid_argument
          ( "mmaptwo::mmaptwo_win32::mmaptwo_win32: "
            "offset too far from start of file");
      } else sz = xsz-off;
    } else if (sz == 0) {
      /* reject non-ending zero parameter */
      ::CloseHandle(fd);
      throw std::invalid_argument
        ( "mmaptwo::mmaptwo_win32::mmaptwo_win32: "
          "non-ending zero parameter rejected");
    }
    /* fix to allocation granularity */{
      ::DWORD psize;
      /* get the allocation granularity */{
        ::SYSTEM_INFO s_info;
        ::GetSystemInfo(&s_info);
        psize = s_info.dwAllocationGranularity;
      }
      fullsize = sz;
      if (psize > 0) {
        /* adjust the offset */
        fullshift = off;
        fulloff = (off-fullshift);
        if (fullshift >= ((~(size_t)0u)-sz)) {
          /* range fix failure */
          ::CloseHandle(fd);
          errno = ERANGE;
          throw std::range_error
            ( "mmaptwo::mmaptwo_win32::mmaptwo_win32: "
              "range fix failure");
        } else fullsize += fullshift;
        /* adjust the size */{
          size_t size_shift = (fullsize % psize);
          if (size_shift > 0) {
            extended_size = fullsize + (psize - size_shift);
          } else extended_size = fullsize;
        }
      } else {
        fulloff = off;
        extended_size = sz;
      }
    }
    /* prepare the security attributes */{
      std::memset(&cfmsa, 0, sizeof(cfmsa));
      cfmsa.nLength = sizeof(cfmsa);
      cfmsa.lpSecurityDescriptor = NULL;
      cfmsa.bInheritHandle = (::BOOL)(mt.bequeath ? TRUE : FALSE);
    }
    /* check for potential overflow */{
      if (fulloff >= ((~(size_t)0u)-extended_size)) {
        /* range fix failure */
        ::CloseHandle(fd);
        errno = ERANGE;
        throw std::range_error
          ( "mmaptwo::mmaptwo_win32::mmaptwo_win32: "
            "overflow of size and offset");
      }
    }
    /* create the file mapping object */{
      /*
       * clamp size to end of file;
       * based on https://stackoverflow.com/a/46014637
       */
      size_t const fullextent = size_clamp > extended_size+fulloff
          ? extended_size + fulloff
          : size_clamp;
      fmd = ::CreateFileMappingA(
          fd, /*hFile*/
          &cfmsa, /*lpFileMappingAttributes*/
          mode_prot_cvt(mt.mode), /*flProtect*/
          (::DWORD)((fullextent>>32)&0xFFffFFff), /*dwMaximumSizeHigh*/
          (::DWORD)(fullextent&0xFFffFFff), /*dwMaximumSizeLow*/
          nullptr /*lpName*/
        );
    }
    if (fmd == nullptr) {
      /* file mapping failed */
      ::CloseHandle(fd);
      throw std::runtime_error
        ( "mmaptwo::mmaptwo_win32::mmaptwo_win32: "
          "CreateFileMappingA fault");
    }
    /* initialize the interface */{
      this->len = fullsize;
      this->fd = fd;
      this->fmd = fmd;
      this->offnum = off;
      this->mt = mt;
    }
    return;
  }

  mmaptwo_win32::~mmaptwo_win32(void) {
    ::CloseHandle(this->fmd);
    this->fmd = nullptr;
    ::CloseHandle(this->fd);
    this->fd = nullptr;
    return;
  }

  page_i* mmaptwo_win32::acquire(size_t sz, size_t pre_off) {
    size_t off;
    /* repair input size and offset */{
      size_t const shifted_len = this->len - this->offnum;
      if (pre_off > shifted_len
      ||  sz > shifted_len - pre_off
      ||  sz == 0u)
      {
        errno = EDOM;
        throw std::invalid_argument
          ( "mmaptwo::mmaptwo_win32::acquire: "
            "size and offset out of range");
      }
      off = pre_off + this->offnum;
    }
    return new page_win32(fmd, mt, sz, off, pre_off);
  }
  page_win32::page_win32
    (HANDLE fmd, struct mode_tag mt, size_t sz, size_t off, size_t pre_off)
  {
    size_t fullsize;
    size_t fullshift;
    void *ptr;
    size_t fulloff;
    /* compute new offsets */{
      ::DWORD psize;
      /* get the allocation granularity */{
        ::SYSTEM_INFO s_info;
        ::GetSystemInfo(&s_info);
        psize = s_info.dwAllocationGranularity;
      }
      fullsize = sz;
      if (psize > 0) {
        /* adjust the offset */
        fullshift = off%psize;
        fulloff = (off-fullshift);
        if (fullshift >= ((~(size_t)0u)-sz)) {
          /* range fix failure */
          errno = ERANGE;
          throw std::length_error
            ("mmapio::page_win32::page_win32: range fix failure");
        } else fullsize += fullshift;
      } else {
        fulloff = off;
      }
    }
    ptr = ::MapViewOfFile(
        fmd, /*hFileMappingObject*/
        mode_access_cvt(mt), /*dwDesiredAccess*/
        (::DWORD)((fulloff>>32)&0xFFffFFff), /* dwFileOffsetHigh */
        (::DWORD)(fulloff&0xFFffFFff), /* dwFileOffsetLow */
        (::SIZE_T)(fullsize) /* dwNumberOfBytesToMap */
      );
    if (!ptr) {
      throw std::runtime_error
        ("mmapio::page_win32::page_win32: MapViewOfFile failure");
    }
    /* initialize the interface */{
      this->ptr = ptr;
      this->len = fullsize;
      this->offnum = pre_off;
      this->shift = fullshift;
    }
    return;
  }

  page_win32::~page_win32(void) {
    UnmapViewOfFile(this->ptr);
    this->ptr = nullptr;
    return;
  }

  size_t mmaptwo_win32::length(void) const {
    return this->len-this->offnum;
  }

  size_t page_win32::length(void) const {
    return this->len-this->shift;
  }

  size_t mmaptwo_win32::offset(void) const {
    return this->offnum;
  }

  size_t page_win32::offset(void) const {
    return this->offnum;
  }

  void* page_win32::get(void) {
    return static_cast<unsigned char*>(this->ptr)+this->shift;
  }

  void const* page_win32::get(void) const {
    return static_cast<unsigned char const*>(this->ptr)+this->shift;
  }
#endif /*MMAPTWO_PLUS_OS*/
  //END   public method
};

namespace mmaptwo {
  //BEGIN configuration functions
  int get_os(void) {
    return (int)(MMAPTWO_PLUS_OS);
  }

  bool check_bequeath_stop(void) {
#if MMAPTWO_PLUS_OS == MMAPTWO_OS_UNIX
#  if (defined O_CLOEXEC)
    return true;
#  else
    return false;
#  endif /*O_CLOEXEC*/
#elif MMAPTWO_PLUS_OS == MMAPTWO_OS_WIN32
    return true;
#else
    return static_cast<bool>(-1);
#endif /*MMAPTWO_PLUS_OS*/
  }

  size_t get_page_size(void) {
#if MMAPTWO_PLUS_OS == MMAPTWO_OS_UNIX
    return (size_t)(::sysconf(_SC_PAGE_SIZE));
#elif MMAPTWO_PLUS_OS == MMAPTWO_OS_WIN32
    ::SYSTEM_INFO s_info;
    ::GetSystemInfo(&s_info);
    return (size_t)(s_info.dwAllocationGranularity);
#else
    return 1;
#endif /*MMAPTWO_PLUS_OS*/
  }
  //END   configuration functions

  //BEGIN open functions
#if MMAPTWO_PLUS_OS == MMAPTWO_OS_UNIX
  mmaptwo_i* open
    (char const* nm, char const* mode, size_t sz, size_t off, bool throwing)
  {
    try {
      int fd;
      struct mode_tag const mt = mode_parse(mode);
      mmaptwo_i* out;
      fd = ::open(nm, mode_rw_cvt(mt.mode));
      if (fd == -1) {
        /* can't open file, so */throw std::runtime_error(strerror(errno));
      }
      return new mmaptwo_unix(fd, mt, sz, off);
    } catch (...) {
      if (throwing) throw;
      else return nullptr;
    }
  }

  mmaptwo_i* u8open
    ( unsigned char const* nm, char const* mode, size_t sz, size_t off,
      bool throwing)
  {
    try {
      int fd;
      struct mode_tag const mt = mode_parse(mode);
      fd = ::open((char const*)nm, mode_rw_cvt(mt.mode));
      if (fd == -1) {
        /* can't open file, so */throw std::runtime_error(strerror(errno));
      }
      return new mmaptwo_unix(fd, mt, sz, off);
    } catch (...) {
      if (throwing) throw;
      else return nullptr;
    }
  }

  mmaptwo_i* wopen
    (wchar_t const* nm, char const* mode, size_t sz, size_t off, bool throwing)
  {
    try {
      int fd;
      struct mode_tag const mt = mode_parse(mode);
      char* const mbfn = wctomb(nm);
      if (mbfn == nullptr) {
        /* conversion failure, so give up */
        throw std::runtime_error("mmaptwo::wopen: text conversion failure");
      }
      fd = ::open(mbfn, mode_rw_cvt(mt.mode));
      free(mbfn);
      if (fd == -1) {
        /* can't open file, so */throw std::runtime_error(strerror(errno));
      }
      return new mmaptwo_unix(fd, mt, sz, off);
    } catch (...) {
      if (throwing) throw;
      else return nullptr;
    }
  }
#elif MMAPTWO_PLUS_OS == MMAPTWO_OS_WIN32
  mmaptwo_i* open
    (char const* nm, char const* mode, size_t sz, size_t off, bool throwing)
  {
    try {
      HANDLE fd;
      struct mode_tag const mt = mode_parse(mode);
      SECURITY_ATTRIBUTES cfsa;
      memset(&cfsa, 0, sizeof(cfsa));
      cfsa.nLength = sizeof(cfsa);
      cfsa.lpSecurityDescriptor = nullptr;
      cfsa.bInheritHandle = (BOOL)(mt.bequeath ? TRUE : FALSE);
      fd = CreateFileA(
          nm, mode_rw_cvt(mt.mode),
          FILE_SHARE_READ|FILE_SHARE_WRITE,
          &cfsa,
          mode_disposition_cvt(mt.mode),
          FILE_ATTRIBUTE_NORMAL,
          nullptr
        );
      if (fd == INVALID_HANDLE_VALUE) {
        /* can't open file, so */throw std::runtime_error
            ("mmaptwo::open: can't open file");
      }
      return new mmaptwo_win32(fd, mt, sz, off);
    } catch (...) {
      if (throwing) throw;
      else return nullptr;
    }
  }

  mmaptwo_i* u8open
    ( unsigned char const* nm, char const* mode, size_t sz, size_t off,
      bool throwing)
  {
    try {
      HANDLE fd;
      struct mode_tag const mt = mode_parse(mode);
      wchar_t* const wcfn = u8towc(nm);
      SECURITY_ATTRIBUTES cfsa;
      memset(&cfsa, 0, sizeof(cfsa));
      cfsa.nLength = sizeof(cfsa);
      cfsa.lpSecurityDescriptor = nullptr;
      cfsa.bInheritHandle = (BOOL)(mt.bequeath ? TRUE : FALSE);
      if (wcfn == nullptr) {
        /* conversion failure, so give up */
        throw std::runtime_error
            ("mmaptwo::u8open: text conversion failure");
      }
      fd = CreateFileW(
          wcfn, mode_rw_cvt(mt.mode),
          FILE_SHARE_READ|FILE_SHARE_WRITE,
          &cfsa,
          mode_disposition_cvt(mt.mode),
          FILE_ATTRIBUTE_NORMAL,
          nullptr
        );
      free(wcfn);
      if (fd == INVALID_HANDLE_VALUE) {
        /* can't open file, so */throw std::runtime_error
            ("mmaptwo::u8open: can't open file");
      }
      return new mmaptwo_win32(fd, mt, sz, off);
    } catch (...) {
      if (throwing) throw;
      else return nullptr;
    }
  }

  mmaptwo_i* wopen
    ( wchar_t const* nm, char const* mode, size_t sz, size_t off,
      bool throwing)
  {
    try {
      HANDLE fd;
      struct mode_tag const mt = mode_parse(mode);
      SECURITY_ATTRIBUTES cfsa;
      memset(&cfsa, 0, sizeof(cfsa));
      cfsa.nLength = sizeof(cfsa);
      cfsa.lpSecurityDescriptor = nullptr;
      cfsa.bInheritHandle = (BOOL)(mt.bequeath ? TRUE : FALSE);
      fd = CreateFileW(
          nm, mode_rw_cvt(mt.mode),
          FILE_SHARE_READ|FILE_SHARE_WRITE,
          &cfsa,
          mode_disposition_cvt(mt.mode),
          FILE_ATTRIBUTE_NORMAL,
          nullptr
        );
      if (fd == INVALID_HANDLE_VALUE) {
        /* can't open file, so */throw std::runtime_error
            ("mmaptwo::wopen: can't open file");
      }
      return new mmaptwo_win32(fd, mt, sz, off);
    } catch (...) {
      if (throwing) throw;
      else return nullptr;
    }
  }
#else
  mmaptwo_i* open
    (char const* nm, char const* mode, size_t sz, size_t off, bool throwing)
  {
    /* no-op */
    if (throwing)
      throw std::runtime_error("mmaptwo::open: unavailable for this system");
    else return nullptr;
  }

  mmaptwo_i* u8open
    ( unsigned char const* nm, char const* mode, size_t sz, size_t off,
      bool throwing)
  {
    /* no-op */
    if (throwing)
      throw std::runtime_error("mmaptwo::u8open: unavailable for this system");
    else return nullptr;
  }

  mmaptwo_i* wopen
    (wchar_t const* nm, char const* mode, size_t sz, size_t off, bool throwing)
  {
    /* no-op */
    if (throwing)
      throw std::runtime_error("mmaptwo::wopen: unavailable for this system");
    else return nullptr;
  }
#endif /*MMAPTWO_PLUS_OS*/
  //END   open functions
};

