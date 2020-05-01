/*****************************************************************************

Copyright (c) 1995, 2020, Oracle and/or its affiliates. All Rights Reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License, version 2.0, as published by the
Free Software Foundation.

This program is also distributed with certain software (including but not
limited to OpenSSL) that is licensed under separate terms, as designated in a
particular file or component or in included license documentation. The authors
of MySQL hereby grant you an additional permission to link the program and
your derivative works with the separately licensed software that they have
included with MySQL.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License, version 2.0,
for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

*****************************************************************************/

/** @file include/buf0dblwr.h
 Doublewrite buffer module

 Created 2011/12/19 Inaam Rana
 *******************************************************/

#ifndef buf0dblwr_h
#define buf0dblwr_h

#include "buf0types.h"
#include "log0log.h"
#include "log0recv.h"
#include "ut0byte.h"

<<<<<<< HEAD
/** Doublewrite system */
extern buf_dblwr_t *buf_dblwr;
/** Set to TRUE when the doublewrite buffer is being created */
extern ibool buf_dblwr_being_created;

/** Creates the doublewrite buffer to a new InnoDB installation. The header of
 the doublewrite buffer is placed on the trx system header page.
 @return true if successful, false if not. */
MY_ATTRIBUTE((warn_unused_result))
bool buf_dblwr_create(void);

/** At a database startup initializes the doublewrite buffer memory structure if
 we already have a doublewrite buffer created in the data files. If we are
 upgrading to an InnoDB version which supports multiple tablespaces, then this
 function performs the necessary update operations. If we are in a crash
 recovery, this function loads the pages from double write buffer into memory.
 @return DB_SUCCESS or error code */
dberr_t buf_dblwr_init_or_load_pages(pfs_os_file_t file, const char *path,
                                     bool load_corrupt_pages);

/** Process and remove the double write buffer pages for all tablespaces. */
void buf_dblwr_process(void);

/** frees doublewrite buffer. */
void buf_dblwr_free(void);
/** Updates the doublewrite buffer when an IO request is completed. */
void buf_dblwr_update(
    const buf_page_t *bpage, /*!< in: buffer block descriptor */
    buf_flush_t flush_type); /*!< in: flush type */
/** Determines if a page number is located inside the doublewrite buffer.
 @return true if the location is inside the two blocks of the
 doublewrite buffer */
ibool buf_dblwr_page_inside(page_no_t page_no); /*!< in: page number */

/** Posts a buffer page for writing. If the doublewrite memory buffer
is full, calls buf_dblwr_flush_buffered_writes and waits for for free
space to appear.
@param[in]	bpage	buffer block to write */
void buf_dblwr_add_to_batch(buf_page_t *bpage);

/** Flush a batch of writes to the datafiles that have already been
 written to the dblwr buffer on disk. */
void buf_dblwr_sync_datafiles();

/** Flushes possible buffered writes from the doublewrite memory buffer to disk,
 and also wakes up the aio thread if simulated aio is used. It is very
 important to call this function after a batch of writes has been posted,
 and also when we may have to wait for a page latch! Otherwise a deadlock
 of threads can occur. */
void buf_dblwr_flush_buffered_writes(void);
/** Writes a page to the doublewrite buffer on disk, sync it, then write
 the page to the datafile and sync the datafile. This function is used
 for single page flushes. If all the buffers allocated for single page
 flushes in the doublewrite buffer are in use we wait here for one to
 become free. We are guaranteed that a slot will become free because any
 thread that is using a slot must also release the slot before leaving
 this function. */
void buf_dblwr_write_single_page(
    buf_page_t *bpage, /*!< in: buffer block to write */
    bool sync);        /*!< in: true if sync IO requested */

/** Recover pages from the double write buffer for a specific tablespace.
The pages that were read from the doublewrite buffer are written to the
tablespace they belong to.
@param[in]	space		Tablespace instance */
void buf_dblwr_recover_pages(fil_space_t *space);

/** Doublewrite control struct */
struct buf_dblwr_t {
  ib_mutex_t mutex;           /*!< mutex protecting the first_free
                              field and write_buf */
  page_no_t block1;           /*!< the page number of the first
                              doublewrite block (64 pages) */
  page_no_t block2;           /*!< page number of the second block */
  page_no_t first_free;       /*!< first free position in write_buf
                           measured in units of UNIV_PAGE_SIZE */
  ulint b_reserved;           /*!< number of slots currently reserved
                           for batch flush. */
  os_event_t b_event;         /*!< event where threads wait for a
                              batch flush to end. */
  ulint s_reserved;           /*!< number of slots currently
                           reserved for single page flushes. */
  os_event_t s_event;         /*!< event where threads wait for a
                              single page flush slot. */
  bool *in_use;               /*!< flag used to indicate if a slot is
                              in use. Only used for single page
                              flushes. */
  bool batch_running;         /*!< set to TRUE if currently a batch
                        is being written from the doublewrite
                        buffer. */
  byte *write_buf;            /*!< write buffer used in writing to the
                            doublewrite buffer, aligned to an
                            address divisible by UNIV_PAGE_SIZE
                            (which is required by Windows aio) */
  byte *write_buf_unaligned;  /*!< pointer to write_buf,
                  but unaligned */
  buf_page_t **buf_block_arr; /*!< array to store pointers to
                        the buffer blocks which have been
                        cached to write_buf */
=======
/** Size of the doublewrite block in pages. */
#define DBLWR_V1_EXTENT_SIZE FSP_EXTENT_SIZE

/** Offset of the doublewrite buffer header on the trx system header page.  */
#define TRX_SYS_DBLWR_V1 (UNIV_PAGE_SIZE - 200)

/** 4-byte ver number which shows if we have created the doublewrite buffer. */
constexpr ulint DBLWR_VER = FSEG_HEADER_SIZE;

/** Page number of the first page in the first sequence of 64 (=
FSP_EXTENT_SIZE) consecutive pages in the doublewrite buffer. */
constexpr ulint DBLWR_V1_BLOCK1 = (4 + FSEG_HEADER_SIZE);

/** Page number of the first page in the second sequence of 64 consecutive
pages in the doublewrite buffer. */
constexpr ulint DBLWR_V1_BLOCK2 = (8 + FSEG_HEADER_SIZE);

namespace dblwr {
/** IO buffer in UNIV_PAGE_SIZE units and aligned on UNIV_PAGE_SIZE */
struct Buffer {
  /** Constructor
  @param[in]	n_pages		        Number of pages to create */
  explicit Buffer(size_t n_pages) noexcept
      : m_n_bytes(n_pages * univ_page_size.physical()) {
    ut_a(n_pages > 0);

    auto n_bytes = m_n_bytes + univ_page_size.physical();

    m_ptr_unaligned = static_cast<byte *>(ut_zalloc_nokey(n_bytes));

    m_ptr = static_cast<byte *>(ut_align(m_ptr_unaligned, UNIV_PAGE_SIZE));

    ut_a(ptrdiff_t(m_ptr - m_ptr_unaligned) <=
         (ssize_t)univ_page_size.physical());

    m_next = m_ptr;
  }

  /** Destructor */
  ~Buffer() noexcept {
    if (m_ptr_unaligned != nullptr) {
      ut_free(m_ptr_unaligned);
    }
    m_ptr_unaligned = nullptr;
  }

  /** Add the contents of ptr upto n_bytes to the buffer.
  @return false if it won't fit. Nothing is copied if it won't fit. */
  bool append(const void *ptr, size_t n_bytes) noexcept {
    ut_a(m_next >= m_ptr && m_next <= m_ptr + m_n_bytes);

    if (m_next + univ_page_size.physical() > m_ptr + m_n_bytes) {
      return false;
    }

    memcpy(m_next, ptr, n_bytes);
    m_next += univ_page_size.physical();

    return true;
  }

  /** @return the start of the buffer to write from. */
  byte *begin() noexcept { return m_ptr; }

  /** @return the start of the buffer to write from. */
  const byte *begin() const noexcept { return m_ptr; }

  /** @return the size of of the buffer to write. */
  size_t size() const noexcept {
    ut_a(m_next >= m_ptr);
    return std::ptrdiff_t(m_next - m_ptr);
  }

  /** @return the capacity of the buffer in bytes. */
  size_t capacity() const noexcept { return m_n_bytes; }

  /** @return true if the buffer is empty. */
  bool empty() const noexcept { return size() == 0; }

  /** Empty the buffer. */
  void clear() noexcept { m_next = m_ptr; }

  /** Write buffer used in writing to the doublewrite buffer,
  aligned to an address divisible by UNIV_PAGE_SIZE (which is
  required by Windows AIO) */
  byte *m_ptr{};

  /** Start of  next write to the buffer. */
  byte *m_next{};

  /** Pointer to m_ptr, but unaligned */
  byte *m_ptr_unaligned{};

  /** Size of the unaligned (raw) buffer. */
  const size_t m_n_bytes{};

  // Disable copying
  Buffer(const Buffer &) = delete;
  Buffer(const Buffer &&) = delete;
  Buffer &operator=(Buffer &&) = delete;
  Buffer &operator=(const Buffer &) = delete;
};

}  // namespace dblwr

#ifndef UNIV_HOTBACKUP

// Forward declaration
class buf_page_t;
class Double_write;

namespace dblwr {

/** Double write files location. */
extern std::string dir;

#ifdef UNIV_DEBUG
/** Crash the server after writing this page to the data file. */
extern page_id_t Force_crash;
#endif /* UNIV_DEBUG */

/** Startup the background thread(s) and create the instance.
@param[in]  create_new_db Create new database.
@return DB_SUCCESS or error code */
dberr_t open(bool create_new_db) noexcept MY_ATTRIBUTE((warn_unused_result));

/** Shutdown the background thread and destroy the instance */
void close() noexcept;

/** Force a write of all pages in the queue.
@param[in] flush_type           FLUSH LIST or LRU_LIST flush request.
@param[in] buf_pool_index       Buffer pool instance for which called. */
void force_flush(buf_flush_t flush_type, uint32_t buf_pool_index) noexcept;

/** Writes a page to the doublewrite buffer on disk, syncs it,
then writes the page to the datafile.
@param[in]  flush_type          Flush type
@param[in]	bpage		            Buffer block to write
@param[in]	sync		            True if sync IO requested
@return DB_SUCCESS or error code */
dberr_t write(buf_flush_t flush_type, buf_page_t *bpage,
              bool sync) noexcept MY_ATTRIBUTE((warn_unused_result));

/** Updates the double write buffer when a write request is completed.
@param[in] bpage               Block that has just been writtent to disk.
@param[in] flush_type          Flush type that triggered the write. */
void write_complete(buf_page_t *bpage, buf_flush_t flush_type) noexcept;

/** Delete or adjust the dblwr file size if required. */
void reset_files() noexcept;

namespace v1 {
/** Read the boundaries of the legacy dblwr buffer extents.
@return DB_SUCCESS or error code. */
dberr_t init() noexcept MY_ATTRIBUTE((warn_unused_result));

/** Create the dblwr data structures in the system tablespace.
@return DB_SUCCESS or error code. */
dberr_t create() noexcept MY_ATTRIBUTE((warn_unused_result));

/** Check if the read is of a page inside the legacy dblwr buffer.
@param[in] page_no              Page number to check.
@return true if offset inside legacy dblwr buffer. */
// clang-format off
bool is_inside(page_no_t page_no) noexcept
    MY_ATTRIBUTE((warn_unused_result));
// clang-format on

}  // namespace v1
}  // namespace dblwr

#endif /* !UNIV_HOTBACKUP */

namespace dblwr {

/** Number of pages per doublewrite thread/segment */
extern ulong n_pages;

/** true if enabled. */
extern bool enabled;

/** Number of files to use for the double write buffer. It must be <= than
the number of buffer pool instances. */
extern ulong n_files;

/** Maximum number of pages to write in one batch. */
extern ulong batch_size;

/** Toggle the doublewrite buffer. */
void set();

namespace recv {

class Pages;

/** Create the recovery dblwr data structures
@param[out]	pages		Pointer to newly created instance */
void create(Pages *&pages) noexcept;

/** Load the doublewrite buffer pages.
@param[in,out] pages           For storing the doublewrite pages read
                               from the double write buffer
@return DB_SUCCESS or error code */
dberr_t load(Pages *pages) noexcept MY_ATTRIBUTE((warn_unused_result));

/** Restore pages from the double write buffer to the tablespace.
@param[in,out]	pages		Pages from the doublewrite buffer
@param[in]	space		Tablespace pages to restore, if set
                                to nullptr then try and restore all. */
void recover(Pages *pages, fil_space_t *space) noexcept;

/** Find a doublewrite copy of a page.
@param[in]	pages		Pages read from the doublewrite buffer
@param[in]	page_id		Page number to lookup
@return	page frame
@retval NULL if no page was found */
const byte *find(
    const Pages *pages,
    const page_id_t &page_id) noexcept MY_ATTRIBUTE((warn_unused_result));

/** Check if some pages from the double write buffer could not be
restored because of the missing tablespace IDs.
@param[in]	pages		Pages to check */
void check_missing_tablespaces(const Pages *pages) noexcept;

/** Free the recovery dblwr data structures
@param[out]	pages		Free the instance */
void destroy(Pages *&pages) noexcept;

/** Redo recovery configuration. */
class DBLWR {
 public:
  /** Constructor. */
  explicit DBLWR() noexcept { create(m_pages); }

  /** Destructor */
  ~DBLWR() noexcept { destroy(m_pages); }

  /** @return true if empty. */
  bool empty() const noexcept { return (m_pages == nullptr); }

  /** Load the doublewrite buffer pages. Doesn't create the doublewrite
  @return DB_SUCCESS or error code */
  dberr_t load() noexcept MY_ATTRIBUTE((warn_unused_result)) {
    return (dblwr::recv::load(m_pages));
  }

  /** Restore pages from the double write buffer to the tablespace.
  @param[in]	space		Tablespace pages to restore,
                                  if set to nullptr then try
                                  and restore all. */
  void recover(fil_space_t *space = nullptr) noexcept {
    dblwr::recv::recover(m_pages, space);
  }

  // clang-format off
  /** Find a doublewrite copy of a page.
  @param[in]	page_id		Page number to lookup
  @return	page frame
  @retval nullptr if no page was found */
  const byte *find(const page_id_t &page_id) noexcept
      MY_ATTRIBUTE((warn_unused_result)) {
    return (dblwr::recv::find(m_pages, page_id));
  }
  // clang-format on

  /** Check if some pages from the double write buffer
  could not be restored because of the missing tablespace IDs. */
  void check_missing_tablespaces() noexcept {
    dblwr::recv::check_missing_tablespaces(m_pages);
  }

#ifndef UNIV_HOTBACKUP
  /** Note that recovery is complete. Adjust the file sizes if necessary. */
  void recovered() noexcept { dblwr::reset_files(); }
#endif /* !UNIV_HOTBACKUP */

  /** Disably copying. */
  DBLWR(const DBLWR &) = delete;
  DBLWR(const DBLWR &&) = delete;
  DBLWR &operator=(DBLWR &&) = delete;
  DBLWR &operator=(const DBLWR &) = delete;

 private:
  /** Pages read from the double write file. */
  Pages *m_pages{};
>>>>>>> mysql-8.0.20
};
}  // namespace recv
}  // namespace dblwr

#endif /* buf0dblwr_h */
