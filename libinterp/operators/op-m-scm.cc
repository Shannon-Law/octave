////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 1998-2023 The Octave Project Developers
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

#if defined (HAVE_CONFIG_H)
#  include "config.h"
#endif

#include "ovl.h"
#include "ov.h"
#include "ov-typeinfo.h"
#include "ov-re-mat.h"
#include "ov-cx-mat.h"
#include "ops.h"
#include "xdiv.h"

#include "sparse-xpow.h"
#include "sparse-xdiv.h"
#include "smx-scm-m.h"
#include "smx-m-scm.h"
#include "ov-cx-sparse.h"

OCTAVE_BEGIN_NAMESPACE(octave)

// matrix by sparse complex matrix ops.

DEFBINOP_OP (add, matrix, sparse_complex_matrix, +)
DEFBINOP_OP (sub, matrix, sparse_complex_matrix, -)

DEFBINOP_OP (mul, matrix, sparse_complex_matrix, *)

DEFBINOP (div, matrix, sparse_complex_matrix)
{
  OCTAVE_CAST_BASE_VALUE (const octave_matrix&, v1, a1);
  OCTAVE_CAST_BASE_VALUE (const octave_sparse_complex_matrix&, v2, a2);

  if (v2.rows () == 1 && v2.columns () == 1)
    return octave_value (v1.array_value () / v2.complex_value ());
  else
    {
      MatrixType typ = v2.matrix_type ();

      ComplexMatrix ret = xdiv (v1.matrix_value (),
                                v2.sparse_complex_matrix_value (), typ);

      v2.matrix_type (typ);
      return ret;
    }
}

DEFBINOPX (pow, matrix, sparse_complex_matrix)
{
  error ("can't do A ^ B for A and B both matrices");
}

DEFBINOP (ldiv, matrix, sparse_complex_matrix)
{
  OCTAVE_CAST_BASE_VALUE (const octave_matrix&, v1, a1);
  OCTAVE_CAST_BASE_VALUE (const octave_sparse_complex_matrix&, v2, a2);
  MatrixType typ = v1.matrix_type ();

  ComplexMatrix ret = xleftdiv (v1.matrix_value (),
                                v2.complex_matrix_value (), typ);

  v1.matrix_type (typ);
  return ret;
}

DEFBINOP_FN (lt, matrix, sparse_complex_matrix, mx_el_lt)
DEFBINOP_FN (le, matrix, sparse_complex_matrix, mx_el_le)
DEFBINOP_FN (eq, matrix, sparse_complex_matrix, mx_el_eq)
DEFBINOP_FN (ge, matrix, sparse_complex_matrix, mx_el_ge)
DEFBINOP_FN (gt, matrix, sparse_complex_matrix, mx_el_gt)
DEFBINOP_FN (ne, matrix, sparse_complex_matrix, mx_el_ne)

DEFBINOP_FN (el_mul, matrix, sparse_complex_matrix, product)
DEFBINOP_FN (el_div, matrix, sparse_complex_matrix, quotient)

DEFBINOP (el_pow, matrix, sparse_complex_matrix)
{
  OCTAVE_CAST_BASE_VALUE (const octave_matrix&, v1, a1);
  OCTAVE_CAST_BASE_VALUE (const octave_sparse_complex_matrix&, v2, a2);

  return octave_value
         (elem_xpow (SparseMatrix (v1.matrix_value ()),
                     v2.sparse_complex_matrix_value ()));
}

DEFBINOP (el_ldiv, matrix, sparse_complex_matrix)
{
  OCTAVE_CAST_BASE_VALUE (const octave_matrix&, v1, a1);
  OCTAVE_CAST_BASE_VALUE (const octave_sparse_complex_matrix&, v2, a2);
  return octave_value
         (quotient (v2.sparse_complex_matrix_value (), v1.matrix_value ()));
}

DEFBINOP_FN (el_and, matrix, sparse_complex_matrix, mx_el_and)
DEFBINOP_FN (el_or,  matrix, sparse_complex_matrix, mx_el_or)

DEFCATOP (m_scm, matrix, sparse_complex_matrix)
{
  OCTAVE_CAST_BASE_VALUE (const octave_matrix&, v1, a1);
  OCTAVE_CAST_BASE_VALUE (const octave_sparse_complex_matrix&, v2, a2);
  SparseMatrix tmp (v1.matrix_value ());
  return octave_value (tmp. concat (v2.sparse_complex_matrix_value (),
                                    ra_idx));
}

DEFCONV (sparse_complex_matrix_conv, matrix, sparse_complex_matrix)
{
  OCTAVE_CAST_BASE_VALUE (const octave_matrix&, v, a);
  return new octave_sparse_complex_matrix
         (SparseComplexMatrix (v.complex_matrix_value ()));
}

void
install_m_scm_ops (octave::type_info& ti)
{
  INSTALL_BINOP_TI (ti, op_add, octave_matrix, octave_sparse_complex_matrix, add);
  INSTALL_BINOP_TI (ti, op_sub, octave_matrix, octave_sparse_complex_matrix, sub);
  INSTALL_BINOP_TI (ti, op_mul, octave_matrix, octave_sparse_complex_matrix, mul);
  INSTALL_BINOP_TI (ti, op_div, octave_matrix, octave_sparse_complex_matrix, div);
  INSTALL_BINOP_TI (ti, op_pow, octave_matrix, octave_sparse_complex_matrix, pow);
  INSTALL_BINOP_TI (ti, op_ldiv, octave_matrix, octave_sparse_complex_matrix,
                    ldiv);
  INSTALL_BINOP_TI (ti, op_lt, octave_matrix, octave_sparse_complex_matrix, lt);
  INSTALL_BINOP_TI (ti, op_le, octave_matrix, octave_sparse_complex_matrix, le);
  INSTALL_BINOP_TI (ti, op_eq, octave_matrix, octave_sparse_complex_matrix, eq);
  INSTALL_BINOP_TI (ti, op_ge, octave_matrix, octave_sparse_complex_matrix, ge);
  INSTALL_BINOP_TI (ti, op_gt, octave_matrix, octave_sparse_complex_matrix, gt);
  INSTALL_BINOP_TI (ti, op_ne, octave_matrix, octave_sparse_complex_matrix, ne);
  INSTALL_BINOP_TI (ti, op_el_mul, octave_matrix, octave_sparse_complex_matrix,
                    el_mul);
  INSTALL_BINOP_TI (ti, op_el_div, octave_matrix, octave_sparse_complex_matrix,
                    el_div);
  INSTALL_BINOP_TI (ti, op_el_pow, octave_matrix, octave_sparse_complex_matrix,
                    el_pow);
  INSTALL_BINOP_TI (ti, op_el_ldiv, octave_matrix, octave_sparse_complex_matrix,
                    el_ldiv);
  INSTALL_BINOP_TI (ti, op_el_and, octave_matrix, octave_sparse_complex_matrix,
                    el_and);
  INSTALL_BINOP_TI (ti, op_el_or, octave_matrix, octave_sparse_complex_matrix,
                    el_or);

  INSTALL_CATOP_TI (ti, octave_matrix, octave_sparse_complex_matrix, m_scm);

  INSTALL_ASSIGNCONV_TI (ti, octave_matrix, octave_sparse_complex_matrix,
                         octave_complex_matrix);

  INSTALL_WIDENOP_TI (ti, octave_matrix, octave_sparse_complex_matrix,
                      sparse_complex_matrix_conv);
}

OCTAVE_END_NAMESPACE(octave)
