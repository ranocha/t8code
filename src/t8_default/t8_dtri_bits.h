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

/** \file t8_dtri_bits.h
 */

#ifndef T8_DTRI_BITS_H
#define T8_DTRI_BITS_H

#include <t8_element.h>
#include "t8_dtri.h"

T8_EXTERN_C_BEGIN ();

/** Compute the parent of a triangle.
 * \param [in]  elem Input triangle.
 * \param [in,out] parent Existing triangle whose data will
 *                  be filled with the data of elem's parent.
 * \note \a elem may point to the same triangle as \a parent.
 */
void                t8_dtri_parent (const t8_dtri_t * t, t8_dtri_t * parent);

/** Compute the coordinates of a vertex of a triangle.
 * \param [in] t    Input triangle.
 * \param [out] coordinates An array of 2 t8_dtri_coord_t that
 * 		     will be filled with the coordinates of the vertex.
 * \param [in] vertex The number of the vertex.
 */
void                t8_dtri_compute_coords (const t8_dtri_t * t,
                                            t8_dtri_coord_t coordinates[2],
                                            const int vertex);

/** Compute the coordinates of the four vertices of a triangle.
 * \param [in] t    Input triangle.
 * \param [out] coordinates An array of 4x3 t8_dtri_coord_t that
 * 		     will be filled with the coordinates of t's vertices.
 */
void                t8_dtri_compute_all_coords (const t8_dtri_t * t,
                                                t8_dtri_coord_t
                                                coordinates[3][2]);

/** Compute the childid-th child in Bey order of a triangle.
 * \param [in] t    Input triangle.
 * \param [in,out] childid The id of the child, 0..7 in Bey order.
 * \param [out] child  Existing triangle whose data will be filled
 * 		    with the date of t's childid-th child.
 */
void                t8_dtri_child (const t8_dtri_t * elem,
                                   int childid, t8_dtri_t * child);

/** Compute the 4 children of a triangle, array version.
 * \param [in]     t  Input triangle.
 * \param [in,out] c  Pointers to the 4 computed children in Morton order.
 *                    t may point to the same quadrant as c[0].
 */
void                t8_dtri_childrenpv (const t8_dtri_t * t, t8_dtri_t * c[]);

/** Compute a specific sibling of a triangle.
 * \param [in]     elem  Input triangle.
 * \param [in,out] sibling  Existing triangle whose data will be filled
 *                    with the data of sibling no. sibling_id of elem.
 * \param [in]     sibid The id of the sibling computed, 0..7 in Bey order.
 */
void                t8_dtri_sibling (const t8_dtri_t * elem,
                                     int sibid, t8_dtri_t * sibling);

/** Compute the face neighbor of a triangle.
 * \param [in]     t      Input triangle.
 * \param [in]     face   The face across which to generate the neighbor.
 * \param [in,out] n      Existing triangle whose data will be filled.
 * \note \a t may point to the same triangle as \a n.
 */
int                 t8_dtri_face_neighbour (const t8_dtri_t * t,
                                            t8_dtri_t * n, int face);

/** Test if a triangle lies inside of the root triangle,
 *  that is the triangle of level 0, anchor node (0,0)
 *  and type 0.
 *  \param [in]     t Input triangle.
 *  \return true    If \a t lies inside of the root triangle.
 */
int
t8_dtri_is_inside_root(t8_dtri_t *t);

/** Test if a triangle lies outside of a reference triangle of
 *  a given type and level lying in the ancestor cube of the triangle.
 * \param [in]     t     Input triangle.
 * \param [in]     roottype The type of the reference triangle.
 * \param [in]     level The level of the reference triangle,
 *                       must be smaller then the level of \a t.
 * \return true if \a t lies outside the reference triangle.
 */
int                 t8_dtri_is_outside (const t8_dtri_t * t, int8_t roottype,
                                        int8_t level);

/** Test if two triangles have the same coordinates, type and level.
 * \return true if \a t1 describes the same triangle as \a t2.
 */
int                 t8_dtri_is_equal (const t8_dtri_t * t1,
                                      const t8_dtri_t * t2);

/** Test if two triangles are siblings.
 * \param [in] t1 First triangle to be tested.
 * \param [in] t2 Second triangle to be tested.
 * \return true if \a t1 is unequal to and a sibling of \a t2.
 */
int                 t8_dtri_is_sibling (const t8_dtri_t * t1,
                                        const t8_dtri_t * t2);

/** Test if a triangle is the parent of another triangle.
 * \param [in] t triangle to be tested.
 * \param [in] c Possible child triangle.
 * \return true if \a t is the parent of \a c.
 */
int                 t8_dtri_is_parent (const t8_dtri_t * t,
                                       const t8_dtri_t * c);

/** Test if a triangle is an ancestor of another triangle.
 * \param [in] t triangle to be tested.
 * \param [in] c Descendent triangle.
 * \return true if \a t is unequal to and an ancestor of \a c.
 */
int                 t8_dtri_is_ancestor (const t8_dtri_t * t,
                                         const t8_dtri_t * c);

T8_EXTERN_C_END ();

#endif /* T8_DTRI_BITS_H */