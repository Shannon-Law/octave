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
#include "ov-cx-mat.h"
#include "ops.h"
#include "xdiv.h"

#include "sparse-xpow.h"
#include "sparse-xdiv.h"
#include "smx-scm-cm.h"
#include "smx-cm-scm.h"
#include "ov-cx-sparse.h"

OCTAVE_BEGIN_NAMESPACE(octave)

// complex matrix by sparse complex matrix ops.

DEFBINOP_OP (add, complex_matrix, sparse_complex_matrix, +)
DEFBINOP_OP (sub, complex_matrix, sparse_complex_matrix, -)

DEFBINOP_OP (mul, complex_matrix, sparse_complex_matrix, *)

DEFBINOP (div, complex_matrix, sparse_complex_matrix)
{
  OCTAVE_CAST_BASE_VALUE (const octave_complex_matrix&, v1, a1);
  OCTAVE_CAST_BASE_VALUE (const octave_sparse_complex_matrix&, v2, a2);

  if (v2.rows () == 1 && v2.columns () == 1)
    return octave_value (v1.complex_array_value () / v2.complex_value ());
  else
    {
      MatrixType typ = v2.matrix_type ();

      ComplexMatrix ret = xdiv (v1.complex_matrix_value (),
                                v2.sparse_complex_matrix_value (), typ);

      v2.matrix_type (typ);
      return ret;
    }
}

DEFBINOPX (pow, complex_matrix, sparse_complex_matrix)
{
  error ("can't do A ^ B for A and B both matrices");
}

DEFBINOP (ldiv, complex_matrix, sparse_complex_matrix)
{
  OCTAVE_CAST_BASE_VALUE (const octave_complex_matrix&, v1, a1);
  OCTAVE_CAST_BASE_VALUE (const octave_sparse_complex_matrix&, v2, a2);
  MatrixType typ = v1.matrix_type ();

  ComplexMatrix ret = xleftdiv (v1.complex_matrix_value (),
                                v2.complex_matrix_value (), typ);

  v1.matrix_type (typ);
  return ret;
}

DEFBINOP_FN (mul_trans, complex_matrix, sparse_complex_matrix, mul_trans);
DEFBINOP_FN (mul_herm, complex_matrix, sparse_complex_matrix, mul_herm);

DEFBINOP_FN (lt, complex_matrix, sparse_complex_matrix, mx_el_lt)
DEFBINOP_FN (le, complex_matrix, sparse_complex_matrix, mx_el_le)
DEFBINOP_FN (eq, complex_matrix, sparse_complex_matrix, mx_el_eq)
DEFBINOP_FN (ge, complex_matrix, sparse_complex_matrix, mx_el_ge)
DEFBINOP_FN (gt, complex_matrix, sparse_complex_matrix, mx_el_gt)
DEFBINOP_FN (ne, complex_matrix, sparse_complex_matrix, mx_el_ne)

DEFBINOP_FN (el_mul, complex_matrix, sparse_complex_matrix, product)
DEFBINOP_FN (el_div, complex_matrix, sparse_complex_matrix, quotient)

DEFBINOP (el_pow, complex_matrix, sparse_complex_matrix)
{
  OCTAVE_CAST_BASE_VALUE (const octave_complex_matrix&, v1, a1);
  OCTAVE_CAST_BASE_VALUE (const octave_sparse_complex_matrix&, v2, a2);

  return octave_value
         (elem_xpow (SparseComplexMatrix (v1.complex_matrix_value ()),
                     v2.sparse_complex_matrix_value ()));
}

DEFBINOP (el_ldiv, sparse_complex_matrix, matrix)
{
  OCTAVE_CAST_BASE_VALUE (const octave_complex_matrix&, v1, a1);
  OCTAVE_CAST_BASE_VALUE (const octave_sparse_complex_matrix&, v2, a2);

  return octave_value (quotient (v2.sparse_complex_matrix_value (),
                                 v1.complex_matrix_value ()));
}

DEFBINOP_FN (el_and, complex_matrix, sparse_complex_matrix, mx_el_and)
DEFBINOP_FN (el_or,  complex_matrix, sparse_complex_matrix, mx_el_or)

DEFCATOP (cm_scm, complex_matrix, sparse_complex_matrix)
{
  OCTAVE_CAST_BASE_VALUE (const octave_complex_matrix&, v1, a1);
  OCTAVE_CAST_BASE_VALUE (const octave_sparse_complex_matrix&, v2, a2);
  SparseComplexMatrix tmp (v1.complex_matrix_value ());
  return octave_value (tmp. concat (v2.sparse_complex_matrix_value (),
                                    ra_idx));
}

DEFCONV (sparse_complex_matrix_conv, complex_matrix,
         sparse_complex_matrix)
{
  OCTAVE_CAST_BASE_VALUE (const octave_complex_matrix&, v, a);
  return new octave_sparse_complex_matrix
         (SparseComplexMatrix (v.complex_matrix_value ()));
}

DEFNDASSIGNOP_FN (assign, complex_matrix, sparse_complex_matrix,
                  complex_array, assign)

void
install_cm_scm_ops (octave::type_info& ti)
{
  INSTALL_BINOP_TI (ti, op_add, octave_complex_matrix,
                    octave_sparse_complex_matrix, add);
  INSTALL_BINOP_TI (ti, op_sub, octave_complex_matrix,
                    octave_sparse_complex_matrix, sub);
  INSTALL_BINOP_TI (ti, op_mul, octave_complex_matrix,
                    octave_sparse_complex_matrix, mul);
  INSTALL_BINOP_TI (ti, op_div, octave_complex_matrix,
                    octave_sparse_complex_matrix, div);
  INSTALL_BINOP_TI (ti, op_pow, octave_complex_matrix,
                    octave_sparse_complex_matrix, pow);
  INSTALL_BINOP_TI (ti, op_ldiv, octave_complex_matrix,
                    octave_sparse_complex_matrix, ldiv);
  INSTALL_BINOP_TI (ti, op_mul_trans, octave_complex_matrix,
                    octave_sparse_complex_matrix, mul_trans);
  INSTALL_BINOP_TI (ti, op_mul_herm, octave_complex_matrix,
                    octave_sparse_complex_matrix, mul_herm);
  INSTALL_BINOP_TI (ti, op_lt, octave_complex_matrix,
                    octave_sparse_complex_matrix, lt);
  INSTALL_BINOP_TI (ti, op_le, octave_complex_matrix,
                    octave_sparse_complex_matrix, le);
  INSTALL_BINOP_TI (ti, op_eq, octave_complex_matrix,
                    octave_sparse_complex_matrix, eq);
  INSTALL_BINOP_TI (ti, op_ge, octave_complex_matrix,
                    octave_sparse_complex_matrix, ge);
  INSTALL_BINOP_TI (ti, op_gt, octave_complex_matrix,
                    octave_sparse_complex_matrix, gt);
  INSTALL_BINOP_TI (ti, op_ne, octave_complex_matrix,
                    octave_sparse_complex_matrix, ne);
  INSTALL_BINOP_TI (ti, op_el_mul, octave_complex_matrix,
                    octave_sparse_complex_matrix, el_mul);
  INSTALL_BINOP_TI (ti, op_el_div, octave_complex_matrix,
                    octave_sparse_complex_matrix, el_div);
  INSTALL_BINOP_TI (ti, op_el_pow, octave_complex_matrix,
                    octave_sparse_complex_matrix, el_pow);
  INSTALL_BINOP_TI (ti, op_el_ldiv, octave_complex_matrix,
                    octave_sparse_complex_matrix, el_ldiv);
  INSTALL_BINOP_TI (ti, op_el_and, octave_complex_matrix,
                    octave_sparse_complex_matrix, el_and);
  INSTALL_BINOP_TI (ti, op_el_or, octave_complex_matrix,
                    octave_sparse_complex_matrix, el_or);

  INSTALL_CATOP_TI (ti, octave_complex_matrix,
                    octave_sparse_complex_matrix, cm_scm);

  INSTALL_ASSIGNOP_TI (ti, op_asn_eq, octave_complex_matrix,
                       octave_sparse_complex_matrix, assign)
  INSTALL_ASSIGNCONV_TI (ti, octave_complex_matrix, octave_sparse_complex_matrix,
                         octave_complex_matrix);

  INSTALL_WIDENOP_TI (ti, octave_complex_matrix, octave_sparse_complex_matrix,
                      sparse_complex_matrix_conv);
}

OCTAVE_END_NAMESPACE(octave)
