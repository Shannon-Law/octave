########################################################################
##
## Copyright (C) 1995-2023 The Octave Project Developers
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
## @deftypefn  {} {@var{rho} =} spearman (@var{x})
## @deftypefnx {} {@var{rho} =} spearman (@var{x}, @var{y})
## @cindex Spearman's Rho
## Compute Spearman's rank correlation coefficient
## @tex
## $\rho$.
## @end tex
## @ifnottex
## @var{rho}.
## @end ifnottex
##
## For two data vectors @var{x} and @var{y}, Spearman's
## @tex
## $\rho$
## @end tex
## @ifnottex
## @var{rho}
## @end ifnottex
## is the correlation coefficient of the ranks of @var{x} and @var{y}.
##
## If @var{x} and @var{y} are drawn from independent distributions,
## @tex
## $\rho$
## @end tex
## @ifnottex
## @var{rho}
## @end ifnottex
## has zero mean and variance
## @tex
## $1 / (N - 1)$,
## @end tex
## @ifnottex
## @code{1 / (N - 1)},
## @end ifnottex
## where @math{N} is the length of the @var{x} and @var{y} vectors, and is
## asymptotically normally distributed.
##
## @code{spearman (@var{x})} is equivalent to
## @code{spearman (@var{x}, @var{x})}.
## @seealso{ranks, kendall}
## @end deftypefn

function rho = spearman (x, y = [])

  if (nargin < 1)
    print_usage ();
  endif

  if (   ! (isnumeric (x) || islogical (x))
      || ! (isnumeric (y) || islogical (y)))
    error ("spearman: X and Y must be numeric matrices or vectors");
  endif

  if (ndims (x) != 2 || ndims (y) != 2)
    error ("spearman: X and Y must be 2-D matrices or vectors");
  endif

  if (isrow (x))
    x = x.';
  endif

  if (nargin == 1)
    rho = corr (ranks (x));
  else
    if (isrow (y))
      y = y.';
    endif
    if (rows (x) != rows (y))
      error ("spearman: X and Y must have the same number of observations");
    endif
    rho = corr (ranks (x), ranks (y));
  endif

  ## Restore class cleared by ranks
  if (isa (x, "single") || isa (y, "single"))
    rho = single (rho);
  endif

endfunction


%!test
%! x = 1:10;
%! y = exp (x);
%! assert (spearman (x,y), 1, 5*eps);
%! assert (spearman (x,-y), -1, 5*eps);

%!assert (spearman ([1 2 3], [-1 1 -2]), -0.5, 5*eps)

%!assert (spearman (1), NaN)
%!assert (spearman (single (1)), single (NaN))

## Test input validation
%!error <Invalid call> spearman ()
%!error spearman (['A'; 'B'])
%!error spearman (ones (1,2), {1, 2})
%!error spearman (ones (2,2,2))
%!error spearman (ones (2,2), ones (2,2,2))
%!error spearman (ones (2,2), ones (3,2))
