/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element types in parallel.

  Copyright (C) 2010 The University of Texas System
  Written by Carsten Burstedde, Lucas C. Wilcox, and Tobin Isaac

  t8code is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  t8code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with t8code; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <sc_refcount.h>
#include <t8_default.h>
#include <t8_forest.h>

static void
t8_basic (int do_dup, int set_level, int do_commit)
{
  t8_forest_t         forest;

  t8_forest_init (&forest);

  t8_forest_set_cmesh (forest, t8_cmesh_new_tet (sc_MPI_COMM_WORLD, do_dup));
  t8_forest_set_scheme (forest, t8_scheme_new_default ());

  t8_forest_set_level (forest, set_level);

  if (do_commit) {
    t8_forest_commit (forest);
    t8_forest_write_vtk (forest, "basic");
  }

  t8_forest_unref (&forest);
}

int
main (int argc, char **argv)
{
  int                 mpiret;
  int                 level;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_ESSENTIAL);
  p4est_init (NULL, SC_LP_ESSENTIAL);
  t8_init (SC_LP_DEFAULT);

  level = 3;
  t8_basic (0, level, 0);
  t8_basic (1, level, 0);
  t8_basic (0, level, 1);
  t8_basic (1, level, 1);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
