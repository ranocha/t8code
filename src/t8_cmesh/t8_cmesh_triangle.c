/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element classes in parallel.

  Copyright (C) 2015 the developers

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

#include <t8_cmesh_triangle.h>
#include <t8_cmesh_tetgen.h>
#include <t8_cmesh_vtk.h>
#include "t8_cmesh_types.h"
#include "t8_cmesh_stash.h"

/* TODO: eventually compute neighbours only from .node and .ele files, since
 *       creating .neigh files with tetgen/triangle is not common and even seems
 *       to not work sometimes */

/* Read a the next line from a file stream that does not start with '#' or
 * contains only whitespaces (tabs etc.)
 *
 * \param [in,out] line     An allocated string to store the line.
 * \param [in,out] n        The number of allocated bytes.
 *                          If more bytes are needed line is reallocated and
 *                          the new number of bytes is stored in n.
 * \param [in]     fp       The file stream to read from.
 * \return                  The number of read arguments of the last line read.
 *                          negative on failure */
static int
t8_cmesh_triangle_read_next_line (char **line, size_t * n, FILE * fp)
{
  int                 retval;

  do {
    /* read first non-comment line from file */
    /* TODO: getline depends on IEEE Std 1003.1-2008 (``POSIX.1'')
     *       p4est therefore has its own getline function in p4est_connectivity.h. */
    retval = getline (line, n, fp);
    if (retval < 0) {
      return retval;
    }
  }
  /* check if line is a comment (trainling '#') or consists solely of
   * blank spaces/tabs */
  while (*line[0] == '#' || strspn (*line, " \t\r\v\n") == strlen (*line));
  return retval;
}

/* Open .node file  and read node input
 * vertices is needed to temporarily store the vertex coordinates and pass
 * to t8_cmesh_triangle_read_eles.
 * memory for vertices is allocated here.
 * On succes the index of the first node is returned (0 or 1).
 * On failure -1 is returned. */
static int
t8_cmesh_triangle_read_nodes (t8_cmesh_t cmesh, char *filename,
                              double **vertices, t8_topidx_t * num_corners,
                              int dim)
{
  FILE               *fp;
  char               *line = T8_ALLOC (char, 1024);
  size_t              linen = 1024;
  t8_topidx_t         cit;
  t8_topidx_t         corner, corner_offset;
  double              x, y, z;
#if 0                           /* used for currently disabeld code */
  int                 i, bdy_marker;
#endif
  int                 num_attributes;
  int                 nbdy_marker;
  int                 retval;
  int                 temp;
  int                 num_read;

  T8_ASSERT (filename != NULL);
  T8_ASSERT (dim == 2 || dim == 3);
  fp = fopen (filename, "r");
  if (fp == NULL) {
    t8_global_errorf ("Failed to open %s.\n", filename);
    goto die_node;
  }

  /* read first non-comment line from .node file */
  retval = t8_cmesh_triangle_read_next_line (&line, &linen, fp);
  if (retval < 0) {
    t8_global_errorf ("Failed to read first line from %s.\n", filename);
    goto die_node;
  }

  /* read number of corners, dimension (must be 2), number of attributes
   * and number of boundary markers (0 or 1) */
  retval = sscanf (line, "%i %i %i %i", num_corners, &temp, &num_attributes,
                   &nbdy_marker);
  if (retval != 4) {
    t8_global_errorf ("Premature end of line.\n");
    goto die_node;
  }
  if (temp != dim) {
    t8_global_errorf ("Dimension must equal %i.\n", dim);
    goto die_node;
  }
  T8_ASSERT (num_attributes >= 0);
  T8_ASSERT (nbdy_marker == 0 || nbdy_marker == 1);

  *vertices = T8_ALLOC (double, dim * *num_corners);
  /* read all vertex coordinates */
  for (cit = 0; cit < *num_corners; cit++) {
    retval = t8_cmesh_triangle_read_next_line (&line, &linen, fp);
    if (retval < 0) {
      t8_global_errorf ("Failed to read line from %s.\n", filename);
      goto die_node;
    }
    /* read corner number and coordinates */
    retval = sscanf (line, "%i %lf %lf%n", &corner, &x, &y, &num_read);
    if (dim == 3) {
      retval += sscanf (line + num_read, "%lf", &z);
    }
    if (retval != dim + 1) {
      t8_global_errorf ("Premature end of line in %s.\n", filename);
    }
    /* The corners in a triangle file are indexed starting with zero or one.
     * The corners in the cmesh always start with zero */
    if (cit == 0) {
      T8_ASSERT (corner == 0 || corner == 1);
      corner_offset = corner;
    }
    (*vertices)[dim * cit] = x;
    (*vertices)[dim * cit + 1] = y;
    if (dim == 3) {
      (*vertices)[dim * cit + 2] = z;
    }

#if 0                           /* read attributes and boundary marker. This part is currently not needed */
    /* read attributes but do not save them */
    for (i = 0; i < num_attributes; i++) {
      retval = sscanf (line, "%*f ");
      if (retval != 0) {
        t8_global_errorf ("Premature end of line in %s.\n", filename);
      }
    }
    retval = sscanf (&line, "%i", &bdy_marker);
    if (retval != 1) {
      t8_global_errorf ("Premature end of line in %s.\n", filename);
    }
#endif /* if 0 */
  }
  fclose (fp);
  /* Done reading .node file */
  T8_FREE (line);
  return corner_offset;
die_node:
  /* Clean up on error. */
  /* Close open file */
  if (fp != NULL) {
    fclose (fp);
  }
  T8_FREE (line);
  return -1;
}

/* Open .ele file and read element input
 * On succes the index of the first element is returned (0 or 1).
 * On failure -1 is returned. */
/* TODO: We can use this file to scan for the neighbors as well
 *       for each node create a list of all nodes (with smaller index)
 *       that it shares a face with. And for each triangle face, look-up
 *       in this list.
 */
static int
t8_cmesh_triangle_read_eles (t8_cmesh_t cmesh, int corner_offset,
                             char *filename, double *vertices, int dim
#ifdef T8_ENABLE_DEBUG
                             , t8_topidx_t num_vertices
#endif
  )
{
  FILE               *fp;
  char               *line = T8_ALLOC (char, 1024);
  size_t              linen = 1024;
  t8_topidx_t         num_elems, tit;
  t8_topidx_t         triangle, triangle_offset;
  t8_topidx_t         tcorners[4];      /* in 2d only the first 3 values are needed */
  int                 retval;
  int                 temp;
  int                 i;
  int                 num_read;
  double              tree_vertices[9];

  /* Open .ele file and read element input */
  T8_ASSERT (filename != NULL);
  T8_ASSERT (dim == 2 || dim == 3);
  fp = fopen (filename, "r");
  if (fp == NULL) {
    t8_global_errorf ("Failed to open %s.\n", filename);
    goto die_ele;
  }
  /* read first non-comment line from .ele file */
  retval = t8_cmesh_triangle_read_next_line (&line, &linen, fp);
  if (retval < 0) {
    t8_global_errorf ("Failed to read first line from %s.\n", filename);
    goto die_ele;
  }

  /* get number of triangles and points per triangle */
  retval = sscanf (line, "%i %i", &num_elems, &temp);
  if (retval != 2) {
    t8_global_errorf ("Premature end of line in %s.\n", filename);
  }
  T8_ASSERT (temp >= 3);
  /* This step is actually only necessary if the cmesh will be bcasted and
   * partitioned. Then we use the num_elems variable to compute the partition table
   * on the remote processes */
  t8_cmesh_set_num_trees (cmesh, num_elems);
  /* For each triangle read the corner indices */
  for (tit = 0; tit < num_elems; tit++) {
    retval = t8_cmesh_triangle_read_next_line (&line, &linen, fp);
    if (retval < 0) {
      t8_global_errorf ("Failed to read line from %s.\n", filename);
      goto die_ele;
    }
    retval = sscanf (line, "%i %i %i %i%n", &triangle, tcorners, tcorners + 1,
                     tcorners + 2, &num_read);
    if (dim == 3) {
      /* TODO: this is kind of unelegant, can we do it better? */
      retval += sscanf (line + num_read, "%i", tcorners + 3);
    }
    if (retval != dim + 2) {
      t8_global_errorf ("Premature end of line in %s.\n", filename);
      goto die_ele;
    }
    /* The triangles in a triangle file are indexed starting with zero or one.
     * The triangles in the cmesh always start with zero */
    if (tit == 0) {
      triangle_offset = triangle;
      T8_ASSERT (triangle == 0 || triangle == 1);
    }
    T8_ASSERT (triangle - triangle_offset == tit);
    t8_cmesh_set_tree_class (cmesh, triangle - triangle_offset,
                             dim == 2 ? T8_ECLASS_TRIANGLE : T8_ECLASS_TET);
    if (corner_offset != 0) {
      tcorners[0] -= corner_offset;
      tcorners[1] -= corner_offset;
      tcorners[2] -= corner_offset;
      tcorners[3] -= corner_offset;
    }
    T8_ASSERT (tcorners[0] < num_vertices);
    T8_ASSERT (tcorners[1] < num_vertices);
    T8_ASSERT (tcorners[2] < num_vertices);
    T8_ASSERT (dim == 2 || tcorners[3] < num_vertices);
    for (i = 0; i < dim + 1; i++) {
      tree_vertices[3 * i] = vertices[dim * tcorners[i]];
      tree_vertices[3 * i + 1] = vertices[dim * tcorners[i] + 1];
      tree_vertices[3 * i + 2] =
        dim == 2 ? 0 : vertices[dim * tcorners[i] + 2];
    }
    t8_cmesh_set_tree_vertices (cmesh, triangle - triangle_offset,
                                t8_get_package_id(), 0,
                                tree_vertices, dim + 1);
  }
  fclose (fp);
  T8_FREE (vertices);
  T8_FREE (line);
  /* Done reading .ele file */
  return triangle_offset;
die_ele:
  /* Clean up on error. */
  /* Close open file */
  if (fp != NULL) {
    fclose (fp);
  }
  T8_FREE (vertices);
  T8_FREE (line);
  return -1;
}

/* Open .neigh file and read element neighbor information
 * On success 0 is returned.
 * On failure -1 is returned. */
static int
t8_cmesh_triangle_read_neigh (t8_cmesh_t cmesh, int element_offset,
                              char *filename, int dim)
{
  FILE               *fp;
  char               *line = T8_ALLOC (char, 1024);
  size_t              linen = 1024;
  t8_topidx_t         element, num_elems, tit;
  t8_topidx_t        *tneighbors;
  int                 retval;
  int                 temp;
  int                 orientation, face1, face2;
  int                 num_read;
  const int           num_faces = dim + 1;
  double             *el_vertices1, *el_vertices2;
  int                 ivertex, firstvertex;

  /* Open .neigh file and read face neighbor information */
  T8_ASSERT (filename != NULL);
  T8_ASSERT (dim == 2 || dim == 3);
  fp = fopen (filename, "r");
  if (fp == NULL) {
    t8_global_errorf ("Failed to open %s.\n", filename);
    goto die_neigh;
  }
  /* read first non-comment line from .ele file */
  retval = t8_cmesh_triangle_read_next_line (&line, &linen, fp);
  if (retval < 0) {
    t8_global_errorf ("Failed to read first line from %s.\n", filename);
    goto die_neigh;
  }
  retval = sscanf (line, "%i %i", &num_elems, &temp);
  if (retval != 2) {
    t8_global_errorf ("Premature end of line in   %s.\n", filename);
    goto die_neigh;
  }
  T8_ASSERT (temp == dim + 1);

  tneighbors = T8_ALLOC (t8_topidx_t, num_elems * num_faces);

  /* We read all the neighbors and write them into an array.
   * Since TRIANGLE provides us for each triangle and each face with
   * which triangle ist is connected, we still need to find
   * out with which face of this triangle it is connected. */
  for (tit = 0; tit < num_elems; tit++) {
    retval = t8_cmesh_triangle_read_next_line (&line, &linen, fp);
    if (retval < 0) {
      t8_global_errorf ("Failed to read line from %s.\n", filename);
      goto die_neigh;
    }
    retval =
      sscanf (line, "%i %i %i %i%n", &element, tneighbors + num_faces * tit,
              tneighbors + num_faces * tit + 1,
              tneighbors + num_faces * tit + 2, &num_read);
    if (dim == 3) {
      retval +=
        sscanf (line + num_read, "%i", tneighbors + num_faces * tit + 3);
    }
    if (retval != dim + 2) {
      t8_global_errorf ("Premature end of line in %s.\n", filename);
      goto die_neigh;
    }
    T8_ASSERT (element - element_offset == tit);

  }
  /* We are done reading the file. */
  fclose (fp);

  /* To compute the face neighbor orientations it is necessary to look up the
   * vertices of a given tree_id. This is only possible if the attribute array
   * is sorted. */
  t8_stash_attribute_sort (cmesh->stash);
  /* Finde the neighboring faces */
  for (tit = 0; tit < num_elems; tit++) {
    for (face1 = 0; face1 < num_faces; face1++) {
      element = tneighbors[num_faces * tit + face1] - element_offset;
      /* triangle store the neighbor triangle on face1 of tit
       * or -1 if there is no neighbor */
      if (element != -1 - element_offset && tit < element) {
        for (face2 = 0; face2 < 3; face2++) {
          /* Finde the face number of triangle which is connected to tit */
          if (tneighbors[num_faces * element + face2] == tit + element_offset) {
            break;
          }
        }
        /* jump here after break */
        T8_ASSERT (face2 < num_faces);
        if (dim == 2) {
          /* compute orientation after the pattern
           *         f1
           *        0 1 2
           *       ======
           *    0 | 1 0 1
           * f2 1 | 0 1 0
           *    2 | 1 0 1
           */
          orientation = (face1 + face2 + 1) % 2;
        }
        else {
          /* TODO: compute correct orientation in 3d
             or do we do this here? */
          firstvertex = face1 == 0 ? 1 : 0;
          el_vertices1 =
            (double *) t8_stash_get_attribute (cmesh->stash, tit);
          el_vertices2 =
            (double *) t8_stash_get_attribute (cmesh->stash, element);
          el_vertices1 += 3 * firstvertex;
          for (ivertex = 1; ivertex <= 3; ivertex++) {
            /* The face with number k consists of the vertices with numbers
             * k+1, k+2, k+3 (mod 4)
             * in el_vertices are the coordinates of these vertices in order
             * v_0x v_0y v_0z v_1x v_1y ... */
            if (el_vertices1[0] == el_vertices2[3 * ((face2 + ivertex) % 4)]
                && el_vertices1[1] ==
                el_vertices2[3 * ((face2 + ivertex) % 4) + 1]
                && el_vertices1[2] ==
                el_vertices2[3 * ((face2 + ivertex) % 4) + 2]) {
              orientation = ivertex;
              ivertex = 4;      /* Abort loop */
            }
          }
          T8_ASSERT (ivertex == 5);     /* asserts if an orientation was successfully found */
        }
        /* Insert this face connection if we did not insert it before */
        if (tit < element || face1 <= face2) {
          /* if tit !< element then tit == element,
           * face1 > face2 would mean that we already inserted this connection */
          t8_cmesh_set_join (cmesh, tit, element, face1, face2,
                               orientation);
        }
      }
    }
  }
  T8_FREE (tneighbors);
  T8_FREE (line);
  return 0;
die_neigh:
  /* Clean up on error. */
  /* Close open file */
  if (fp != NULL) {
    fclose (fp);
  }
  T8_FREE (line);
  return -1;
}

static              t8_cmesh_t
t8_cmesh_from_tetgen_or_triangle_file (char *fileprefix, int partition,
                                       sc_MPI_Comm comm, int do_dup, int dim)
{
  int                 mpirank, mpisize, mpiret;
  t8_cmesh_t          cmesh;
  double             *vertices;
  t8_topidx_t         num_vertices;
  t8_gloidx_t         first_tree, last_tree;

  mpiret = sc_MPI_Comm_size (comm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (comm, &mpirank);
  SC_CHECK_MPI (mpiret);

  cmesh = NULL;
  if (mpirank == 0 || partition) {
    int                 retval, corner_offset;
    char                current_file[BUFSIZ];

    t8_cmesh_init (&cmesh);
    t8_cmesh_set_mpicomm (cmesh, comm, do_dup);
    /* read .node file */
    snprintf (current_file, BUFSIZ, "%s.node", fileprefix);
    retval =
      t8_cmesh_triangle_read_nodes (cmesh, current_file, &vertices,
                                    &num_vertices, dim);
    if (retval != 0 && retval != 1) {
      t8_global_errorf ("Error while parsing file %s.\n", current_file);
      t8_cmesh_unref (&cmesh);
    }
    else {
      /* read .ele file */
      corner_offset = retval;
      snprintf (current_file, BUFSIZ, "%s.ele", fileprefix);
      retval =
        t8_cmesh_triangle_read_eles (cmesh, corner_offset, current_file,
                                     vertices, dim
#ifdef T8_ENABLE_DEBUG
                                     , num_vertices
#endif
        );
      if (retval != 0 && retval != 1) {
        t8_global_errorf ("Error while parsing file %s.\n", current_file);
        t8_cmesh_unref (&cmesh);
      }
      else {
        /* read .neigh file */
        snprintf (current_file, BUFSIZ, "%s.neigh", fileprefix);
        retval = t8_cmesh_triangle_read_neigh (cmesh, corner_offset,
                                               current_file, dim);
        if (retval != 0) {
          t8_global_errorf ("Error while parsing file %s.\n", current_file);
          t8_cmesh_unref (&cmesh);
        }
      }
    }
    T8_ASSERT (cmesh != NULL);
  }
  /* TODO: broadcasting NULL does not work. We need a way to tell the
   *       other processes if something went wrong. */
  /* This broadcasts the NULL pointer if anything went wrong */
  if (!partition ) {
    cmesh = t8_cmesh_bcast (cmesh, 0, comm);
  }

  if (cmesh != NULL) {
    if (partition) {
      first_tree = (mpirank * cmesh->num_trees)/mpisize;
      last_tree = ((mpirank + 1) * cmesh->num_trees)/mpisize - 1;
      t8_debugf ("Partition range [%lli,%lli]\n", (long long) first_tree,
                 (long long) last_tree);
      t8_cmesh_set_partitioned (cmesh, 1, 3, first_tree, last_tree);
    }
    t8_cmesh_commit (cmesh);
  }
  return cmesh;
}

t8_cmesh_t
t8_cmesh_from_triangle_file (char *fileprefix, int partition,
                             sc_MPI_Comm comm, int do_dup)
{
  return t8_cmesh_from_tetgen_or_triangle_file (fileprefix, partition, comm,
                                                do_dup, 2);
}

t8_cmesh_t
t8_cmesh_from_tetgen_file (char *fileprefix, int partition,
                           sc_MPI_Comm comm, int do_dup)
{
  return t8_cmesh_from_tetgen_or_triangle_file (fileprefix, partition, comm,
                                                do_dup, 3);
}
