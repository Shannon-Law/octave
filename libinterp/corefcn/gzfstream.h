////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2005-2023 The Octave Project Developers
//
// See the file COPYRIGHT.md in the top-level directory of this
// distribution or <https://octave.org/copyright/>.
//
// This file is part of Octave.
//
// Octave is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Octave is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Octave; see the file COPYING.  If not, see
// <https://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////

/*

 This file is adapted from the zlib 1.2.2 contrib/iostream3 code,
 written by

   Ludwig Schwardt <schwardt@sun.ac.za>
   original version by Kevin Ruland <kevin@rodin.wustl.edu>

*/

#if ! defined (octave_zfsstream_h)
#define octave_zfsstream_h 1

#include "octave-config.h"

#if defined (HAVE_ZLIB)

#include <iosfwd>

#include "zlib.h"

/**
 *  @brief  Gzipped file stream buffer class.
 *
 *  This class implements basic_filebuf for gzipped files.  It doesn't yet
 *  support seeking (allowed by zlib but slow/limited), putback and read/write
 *  access *  (tricky).  Otherwise, it attempts to be a drop-in replacement for
 *  the standard file streambuf.
*/
class gzfilebuf : public std::streambuf
{
public:
  //  Default constructor.
  gzfilebuf ();

  OCTAVE_DISABLE_COPY_MOVE (gzfilebuf)

  //  Destructor.
  virtual ~gzfilebuf ();

  /**
   *  @brief  Set compression level and strategy on the fly.
   *  @param  comp_level  Compression level (see zlib.h for allowed values)
   *  @param  comp_strategy  Compression strategy (see zlib.h for allowed values)
   *  @return  Z_OK on success, Z_STREAM_ERROR otherwise.
   *
   *  Unfortunately, these parameters cannot be modified separately, as the
   *  previous zfstream version assumed.  Since the strategy is seldom changed,
   *  it can default and setcompression(level) then becomes like the old
   *  setcompressionlevel(level).
  */
  int
  setcompression (int comp_level,
                  int comp_strategy = Z_DEFAULT_STRATEGY);

  /**
   *  @brief  Check if file is open.
   *  @return  True if file is open.
  */
  bool
  is_open () const { return (m_file != nullptr); }

  /**
   *  @brief  Open gzipped file.
   *  @param  name  Filename.
   *  @param  mode  Open mode flags.
   *  @return  @c this on success, NULL on failure.
  */
  gzfilebuf *
  open (const char *name,
        std::ios_base::openmode mode);

  /**
   *  @brief  Attach to already open gzipped file.
   *  @param  fd  File descriptor.
   *  @param  mode  Open mode flags.
   *  @return  @c this on success, NULL on failure.
  */
  gzfilebuf *
  attach (int fd,
          std::ios_base::openmode mode);

  /**
   *  @brief  Close gzipped file.
   *  @return  @c this on success, NULL on failure.
  */
  gzfilebuf *
  close ();

protected:
  /**
   *  @brief  Convert ios open mode int to mode string used by zlib.
   *  @return  True if valid mode flag combination.
  */
  bool
  open_mode (std::ios_base::openmode mode,
             char *c_mode) const;

  /**
   *  @brief  Number of characters available in stream buffer.
   *  @return  Number of characters.
   *
   *  This indicates number of characters in get area of stream buffer.
   *  These characters can be read without accessing the gzipped file.
  */
  virtual std::streamsize
  showmanyc ();

  /**
   *  @brief  Fill get area from gzipped file.
   *  @return  First character in get area on success, EOF on error.
   *
   *  This actually reads characters from gzipped file to stream
   *  buffer.  Always buffered.
  */
  virtual int_type
  underflow ();

  /**
   *  @brief  Write put area to gzipped file.
   *  @param  c  Extra character to add to buffer contents.
   *  @return  Non-EOF on success, EOF on error.
   *
   *  This actually writes characters in stream buffer to
   *  gzipped file.  With unbuffered output this is done one
   *  character at a time.
  */
  virtual int_type
  overflow (int_type c = traits_type::eof ());

  /**
   *  @brief  Installs external stream buffer.
   *  @param  p  Pointer to char buffer.
   *  @param  n  Size of external buffer.
   *  @return  @c this on success, NULL on failure.
   *
   *  Call setbuf(0,0) to enable unbuffered output.
  */
  virtual std::streambuf *
  setbuf (char_type *p,
          std::streamsize n);

  /**
   *  @brief  Flush stream buffer to file.
   *  @return  0 on success, -1 on error.
   *
   *  This calls underflow(EOF) to do the job.
  */
  virtual int
  sync ();

  /**
   *  @brief  Alters the stream positions.
   *
   *  Each derived class provides its own appropriate behavior.
   */
  virtual pos_type
  seekoff (off_type off, std::ios_base::seekdir way,
           std::ios_base::openmode mode =
             std::ios_base::in | std::ios_base::out);

  /**
   *  @brief  Alters the stream positions.
   *
   *  Each derived class provides its own appropriate behavior.
   */
  virtual pos_type
  seekpos (pos_type sp, std::ios_base::openmode mode =
             std::ios_base::in | std::ios_base::out);

  virtual int_type
  pbackfail (int_type c = traits_type::eof ());

//
// Some future enhancements
//
//  virtual int_type uflow();
//  virtual int_type pbackfail(int_type c = traits_type::eof());

private:

  /**
   *  @brief  Allocate internal buffer.
   *
   *  This function is safe to call multiple times.  It will ensure
   *  that a proper internal buffer exists if it is required.  If the
   *  buffer already exists or is external, the buffer pointers will be
   *  reset to their original state.
  */
  void
  enable_buffer ();

  /**
   *  @brief  Destroy internal buffer.
   *
   *  This function is safe to call multiple times.  It will ensure
   *  that the internal buffer is deallocated if it exists.  In any
   *  case, it will also reset the buffer pointers.
  */
  void
  disable_buffer ();

  /**
   *  Underlying file pointer.
  */
  gzFile m_file;

  /**
   *  Mode in which file was opened.
  */
  std::ios_base::openmode m_io_mode;

  /**
   *  @brief  True if this object owns file descriptor.
   *
   *  This makes the class responsible for closing the file
   *  upon destruction.
  */
  bool m_own_fd;

  /**
   *  @brief  Stream buffer.
   *
   *  For simplicity this remains allocated on the free store for the
   *  entire life span of the gzfilebuf object, unless replaced by setbuf.
  */
  char_type *m_buffer;

  /**
   *  @brief  Stream buffer size.
   *
   *  Defaults to system default buffer size (typically 8192 bytes).
   *  Modified by setbuf.
  */
  std::streamsize m_buffer_size;

  /**
   *  @brief  True if this object owns stream buffer.
   *
   *  This makes the class responsible for deleting the buffer
   *  upon destruction.
  */
  bool m_own_buffer;
};

/**
 *  @brief  Gzipped file input stream class.
 *
 *  This class implements ifstream for gzipped files.  Seeking and putback
 *  is not supported yet.
*/
class gzifstream : public std::istream
{
public:
  //  Default constructor
  gzifstream ();

  /**
   *  @brief  Construct stream on gzipped file to be opened.
   *  @param  name  Filename.
   *  @param  mode  Open mode flags (forced to contain ios::in).
  */
  explicit
  gzifstream (const char *name,
              std::ios_base::openmode mode = std::ios_base::in);

  /**
   *  @brief  Construct stream on already open gzipped file.
   *  @param  fd    File descriptor.
   *  @param  mode  Open mode flags (forced to contain ios::in).
  */
  explicit
  gzifstream (int fd,
              std::ios_base::openmode mode = std::ios_base::in);

  /**
   *  Obtain underlying stream buffer.
  */
  gzfilebuf *
  rdbuf () const
  { return const_cast<gzfilebuf *>(&m_sb); }

  /**
   *  @brief  Check if file is open.
   *  @return  True if file is open.
  */
  bool
  is_open () { return m_sb.is_open (); }

  /**
   *  @brief  Open gzipped file.
   *  @param  name  Filename.
   *  @param  mode  Open mode flags (forced to contain ios::in).
   *
   *  Stream will be in state good() if file opens successfully;
   *  otherwise in state fail().  This differs from the behavior of
   *  ifstream, which never sets the state to good() and therefore
   *  won't allow you to reuse the stream for a second file unless
   *  you manually clear() the state.  The choice is a matter of
   *  convenience.
  */
  void
  open (const char *name,
        std::ios_base::openmode mode = std::ios_base::in);

  /**
   *  @brief  Attach to already open gzipped file.
   *  @param  fd  File descriptor.
   *  @param  mode  Open mode flags (forced to contain ios::in).
   *
   *  Stream will be in state good() if attach succeeded; otherwise
   *  in state fail().
  */
  void
  attach (int fd,
          std::ios_base::openmode mode = std::ios_base::in);

  /**
   *  @brief  Close gzipped file.
   *
   *  Stream will be in state fail() if close failed.
  */
  void
  close ();

private:
  /**
   *  Underlying stream buffer.
  */
  gzfilebuf m_sb;
};

/**
 *  @brief  Gzipped file output stream class.
 *
 *  This class implements ofstream for gzipped files.  Seeking and putback
 *  is not supported yet.
*/
class gzofstream : public std::ostream
{
public:
  //  Default constructor
  gzofstream ();

  /**
   *  @brief  Construct stream on gzipped file to be opened.
   *  @param  name  Filename.
   *  @param  mode  Open mode flags (forced to contain ios::out).
  */
  explicit
  gzofstream (const char *name,
              std::ios_base::openmode mode = std::ios_base::out);

  /**
   *  @brief  Construct stream on already open gzipped file.
   *  @param  fd    File descriptor.
   *  @param  mode  Open mode flags (forced to contain ios::out).
  */
  explicit
  gzofstream (int fd,
              std::ios_base::openmode mode = std::ios_base::out);

  /**
   *  Obtain underlying stream buffer.
  */
  gzfilebuf *
  rdbuf () const
  { return const_cast<gzfilebuf *>(&m_sb); }

  /**
   *  @brief  Check if file is open.
   *  @return  True if file is open.
  */
  bool
  is_open () { return m_sb.is_open (); }

  /**
   *  @brief  Open gzipped file.
   *  @param  name  Filename.
   *  @param  mode  Open mode flags (forced to contain ios::out).
   *
   *  Stream will be in state good() if file opens successfully;
   *  otherwise in state fail().  This differs from the behavior of
   *  ofstream, which never sets the state to good() and therefore
   *  won't allow you to reuse the stream for a second file unless
   *  you manually clear() the state.  The choice is a matter of
   *  convenience.
  */
  void
  open (const char *name,
        std::ios_base::openmode mode = std::ios_base::out);

  /**
   *  @brief  Attach to already open gzipped file.
   *  @param  fd  File descriptor.
   *  @param  mode  Open mode flags (forced to contain ios::out).
   *
   *  Stream will be in state good() if attach succeeded; otherwise
   *  in state fail().
  */
  void
  attach (int fd,
          std::ios_base::openmode mode = std::ios_base::out);

  /**
   *  @brief  Close gzipped file.
   *
   *  Stream will be in state fail() if close failed.
  */
  void
  close ();

private:
  /**
   *  Underlying stream buffer.
  */
  gzfilebuf m_sb;
};

/**
 *  @brief  Gzipped file output stream manipulator class.
 *
 *  This class defines a two-argument manipulator for gzofstream.  It is used
 *  as base for the setcompression(int,int) manipulator.
*/
template <typename T1, typename T2>
class gzomanip2
{
public:
  // Allows insertor to peek at internals
  template <typename Ta, typename Tb>
  friend gzofstream&
  operator<<(gzofstream&,
             const gzomanip2<Ta, Tb>&);

  // Constructor
  gzomanip2 (gzofstream& (*f)(gzofstream&, T1, T2),
             T1 v1,
             T2 v2);
private:
  // Underlying manipulator function
  gzofstream&
  (*m_fcn)(gzofstream&, T1, T2);

  // Arguments for manipulator function
  T1 m_val1;
  T2 m_val2;
};

// Manipulator function thunks through to stream buffer
inline gzofstream&
setcompression (gzofstream& gzs, int l, int s = Z_DEFAULT_STRATEGY)
{
  (gzs.rdbuf ())->setcompression (l, s);
  return gzs;
}

// Manipulator constructor stores arguments
template <typename T1, typename T2>
inline
gzomanip2<T1, T2>::gzomanip2 (gzofstream &(*f)(gzofstream&, T1, T2),
                              T1 v1,
                              T2 v2)
  : m_fcn(f), m_val1(v1), m_val2(v2)
{ }

// Insertor applies underlying manipulator function to stream
template <typename T1, typename T2>
inline gzofstream&
operator<<(gzofstream& s, const gzomanip2<T1, T2>& m)
{ return (*m.m_fcn)(s, m.m_val1, m.m_val2); }

// Insert this onto stream to simplify setting of compression level
inline gzomanip2<int, int>
setcompression (int l, int s = Z_DEFAULT_STRATEGY)
{ return gzomanip2<int, int>(&setcompression, l, s); }

#endif

#endif
