////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 1996-2023 The Octave Project Developers
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

#if ! defined (octave_oct_prcstrm_h)
#define octave_oct_prcstrm_h 1

#include "octave-config.h"

#include "oct-stdstrm.h"

// FIXME: why don't these classes use iprocstream and oprocstream,
//        which in turn use the octave_procbuf class?

// Note: Even though these classes are now inside the octave namespace,
// we can't rename them to iprocstream and oprocstream because we
// already have classes with those names (see procstream.h).  We need to
// find a way to resolve this naming issue.

OCTAVE_BEGIN_NAMESPACE(octave)

class
octave_iprocstream : public octave::stdiostream
{
public:

  octave_iprocstream (const std::string& n,
                      std::ios::openmode arg_md = std::ios::in,
                      octave::mach_info::float_format flt_fmt
                      = octave::mach_info::native_float_format (),
                      const std::string& encoding = "utf-8");

  OCTAVE_DISABLE_CONSTRUCT_COPY_MOVE (octave_iprocstream)

  static octave::stream
  create (const std::string& n, std::ios::openmode arg_md = std::ios::in,
          octave::mach_info::float_format flt_fmt
          = octave::mach_info::native_float_format (),
          const std::string& encoding = "utf-8");

protected:

  ~octave_iprocstream ();
};

class
octave_oprocstream : public octave::stdiostream
{
public:

  octave_oprocstream (const std::string& n,
                      std::ios::openmode arg_md = std::ios::out,
                      octave::mach_info::float_format flt_fmt
                      = octave::mach_info::native_float_format (),
                      const std::string& encoding = "utf-8");

  OCTAVE_DISABLE_CONSTRUCT_COPY_MOVE (octave_oprocstream)

  static octave::stream
  create (const std::string& n, std::ios::openmode arg_md = std::ios::out,
          octave::mach_info::float_format flt_fmt
          = octave::mach_info::native_float_format (),
          const std::string& encoding = "utf-8");

protected:

  ~octave_oprocstream ();
};

OCTAVE_END_NAMESPACE(octave)

#endif
