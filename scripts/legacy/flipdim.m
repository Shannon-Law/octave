########################################################################
##
## Copyright (C) 2004-2020 The Octave Project Developers
##
## See the file COPYRIGHT.md in the top-level directory of this
## distribution or <https://octave.org/copyright/>.
##
## This file is part of Octave.
##
## Octave is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## Octave is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with Octave; see the file COPYING.  If not, see
## <https://www.gnu.org/licenses/>.
##
########################################################################

## -*- texinfo -*-
## @deftypefn  {} {} flipdim (@var{x})
## @deftypefnx {} {} flipdim (@var{x}, @var{dim})
## This function is obsolete.  Use @code{flip} instead.
## @seealso{flip, fliplr, flipud, rot90, rotdim}
## @end deftypefn

function y = flipdim (varargin)

  persistent warned = false;
  if (! warned)
    warned = true;
    warning ("Octave:legacy-function",
             "flipdim is obsolete; please use flip instead");
  endif

  y = flip (varargin{:});

endfunction
