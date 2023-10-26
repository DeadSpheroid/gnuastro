/*********************************************************************
kdtree -- Create k-d tree and nearest neighbour searches.
This is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Sachin Kumar Singh <sachinkumarsingh092@gmail.com>
Contributing author(s):
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Copyright (C) 2020-2023 Free Software Foundation, Inc.

Gnuastro is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

Gnuastro is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with Gnuastro. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <float.h>

#include <gnuastro/data.h>
#include <gnuastro/table.h>
#include <gnuastro/blank.h>
#include <gnuastro/pointer.h>
#include <gnuastro/permutation.h>




















/****************************************************************
 ********                  Utilities                      *******
 ****************************************************************/
/* Main structure to keep kd-tree parameters. */
struct kdtree_params
{
  size_t ndim;            /* Number of dimentions in the nodes. */
  size_t *input_row;      /* The indexes of the input table. */
  gal_data_t **coords;    /* The input coordinates array. */
  uint32_t *left, *right; /* The indexes of the left and right nodes. */

  /* The values of the left and right columns. */
  gal_data_t *left_col, *right_col;
};





/* Swap 2 nodes of the tree. Instead of physically swaping all the values
   we swap just the indexes of the node. */
static void
kdtree_node_swap(struct kdtree_params *p, size_t node1, size_t node2)
{
  uint32_t tmp_left=p->left[node1];
  uint32_t tmp_right=p->right[node1];
  size_t tmp_input_row=p->input_row[node1];

  /* No need to swap same node. */
  if(node1==node2) return;

  p->left[node1]=p->left[node2];
  p->right[node1]=p->right[node2];
  p->input_row[node1]=p->input_row[node2];

  p->left[node2]=tmp_left;
  p->right[node2]=tmp_right;
  p->input_row[node2]=tmp_input_row;
}





/* Return the distance between 2 given nodes. The distance is equivalent
   to the radius of the hypersphere having node as its center.

   Return: Radial distace from given point to the node.
*/
static double
kdtree_distance_find(struct kdtree_params *p, size_t node,
                     double *point)
{
  size_t i;
  double *carr;
  double t_distance, node_distance=0;

  /* For all dimensions. */
  for(i=0; i<p->ndim; ++i)
    {
      carr=p->coords[i]->array;
      t_distance=carr[node]-point[i];

      node_distance += t_distance*t_distance;
    }

  return node_distance;
}




















/****************************************************************
 ********           Preperations and Cleanup              *******
 ****************************************************************/
/* Initialise the kdtree_params structure and do sanity checks. */
static void
kdtree_prepare(struct kdtree_params *p, gal_data_t *coords_raw)
{
  size_t i;
  gal_data_t *tmp;
  p->ndim=gal_list_data_number(coords_raw);

  /* Allocate the coordinate array. */
  errno=0;
  p->coords=malloc(p->ndim*sizeof(**(p->coords)));
  if(p->coords==NULL)
    error(EXIT_FAILURE, errno, "%s: couldn't allocate %zu bytes "
	  "for 'coords'", __func__, p->ndim*sizeof(**(p->coords)));

  /* Convert input to double type. */
  tmp=coords_raw;
  for(i=0; i<p->ndim; ++i)
    {
      if(tmp->type == GAL_TYPE_FLOAT64)
      	p->coords[i]=tmp;
      else
        p->coords[i]=gal_data_copy_to_new_type(tmp, GAL_TYPE_FLOAT64);

      /* Go to the next column list. */
      tmp=tmp->next;
    }

  /* If the 'left_col' is already defined, then we just need to do
     some sanity checks. */
  if(p->left_col)
    {
      /* Make sure there is more than one column. */
      if(p->left_col->next==NULL)
        error(EXIT_FAILURE, 0, "%s: the input kd-tree should be 2 columns",
              __func__);

      /* Set the right column and check if there aren't any
         more columns. */
      p->right_col=p->left_col->next;
      if(p->right_col->next)
        error(EXIT_FAILURE, 0, "%s: the input kd-tree shoudn't be more "
              "than 2 columns", __func__);

      /* Make sure they are the same size. */
      if(p->left_col->size!=p->right_col->size)
        error(EXIT_FAILURE, 0, "%s: left and right columns should have "
              "same size", __func__);

      /* Make sure left is 'uint32_t'. */
      if(p->left_col->type!=GAL_TYPE_UINT32)
        error(EXIT_FAILURE, 0, "%s: left kd-tree column should be uint32_t",
              __func__);

      /* Make sure right is 'uint32_t'. */
      if(p->right_col->type!=GAL_TYPE_UINT32)
        error(EXIT_FAILURE, 0, "%s: right kd-tree column should be uint32_t",
              __func__);

      /* Initailise left and right arrays. */
      p->left=p->left_col->array;
      p->right=p->right_col->array;
    }
  else
    {
      /* Allocate and initialise the kd-tree input_row. */
      p->input_row=gal_pointer_allocate(GAL_TYPE_SIZE_T, coords_raw->size, 0,
                                        __func__, "p->input_row");
      for(i=0; i<coords_raw->size; ++i)	p->input_row[i]=i;

      /* Allocate output and initialize them. */
      p->left_col=gal_data_alloc(NULL, GAL_TYPE_UINT32, 1,
                                 coords_raw->dsize, NULL, 0,
                                 coords_raw->minmapsize,
                                 coords_raw->quietmmap, "left",
                                 "index",
                                 "index of left subtree in the kd-tree");
      p->right_col=gal_data_alloc(NULL, GAL_TYPE_UINT32, 1,
                                  coords_raw->dsize, NULL, 0,
                                  coords_raw->minmapsize,
                                  coords_raw->quietmmap, "right",
                                  "index",
                                  "index of right subtree in the kd-tree");

      /* Fill the elements of the params structure. */
      p->left_col->next=p->right_col;

      /* Initialise the left and right arrays. */
      p->left=p->left_col->array;
      p->right=p->right_col->array;
      for(i=0;i<coords_raw->size;++i)
        { p->left[i]=p->right[i]=GAL_BLANK_UINT32; }
    }
}





/* Cleanup the data and free the memory used by the structure after use. */
static void
kdtree_cleanup(struct kdtree_params *p, gal_data_t *coords_raw)
{
  size_t i;
  gal_data_t *tmp;

  /* Clean up. */
  tmp = coords_raw;
  for(i = 0; i<p->ndim; ++i)
    {
      if(p->coords[i]!=tmp) gal_data_free(p->coords[i]);
      tmp=tmp->next;
    }

  /* Free memory. */
  free(p->coords);
  free(p->input_row);
}




















/****************************************************************
 ********                Create KD-Tree                   *******
 ****************************************************************/
/* Divide the array into two parts, values more than that of k'th node
   and values less than k'th node.

   Return: Index of the node whose value is greater than all
           the nodes before it.
*/
static size_t
kdtree_make_partition(struct kdtree_params *p, size_t node_left,
                      size_t node_right, size_t node_k,
                      double *coordinate)
{
  /* store_index is the index before which all values are smaller than
     the value of k'th node. */
  size_t i, store_index;
  double k_node_value = coordinate[p->input_row[node_k]];

  /* Move the k'th node to the right. */
  kdtree_node_swap(p, node_k, node_right);

  /* Move all nodes smaller than k'th node to its left and check
     the number of elements smaller than the value present at the
     k'th index. */
  store_index = node_left;
  for(i = node_left; i < node_right; ++i)
    if(coordinate[p->input_row[i]] < k_node_value)
      {
        /* Move i'th node to the left side of the k'th index. */
        kdtree_node_swap(p, store_index, i);

        /* Prepare the place of next smaller node. */
        store_index++;
      }

  /* Place k'th node after all the nodes that have lesser value
     than it, as it was moved to the right initially. */
  kdtree_node_swap(p, node_right, store_index);

  /* Return the store_index. */
  return store_index;
}





/* Find the median node of the current axis. Instead of randomly
   choosing the median node, we use `quickselect alogorithm` to
   find median node in linear time between the left and right node.
   This also makes the values in the current axis partially sorted.

   See `https://en.wikipedia.org/wiki/Quickselect`
   for pseudocode and more details of the algorithm.

   Return: Median node between the given left and right nodes.
*/
static size_t
kdtree_median_find(struct kdtree_params *p, size_t node_left,
                   size_t node_right, double *coordinate)
{
  size_t node_pivot, node_median;

  /* False state, this is a programming error. */
  if(node_right < node_left)
    error(EXIT_FAILURE, 0, "%s: a bug! Please contact us to fix "
          "the problem! For some reason, the node_right (%zu) is "
          "smaller than node_left (%zu)", __func__, node_right,
          node_left);

  /* If the two nodes are the same, just return the node. */
  if(node_right == node_left)
    error(EXIT_FAILURE, 0, "%s: a bug! Please contact us to fix "
          "the problem! For some reason, the node_right (%zu) is "
          "equal to node_left (%zu)", __func__, node_right, node_left);

  /* The required median node between left and right node. */
  node_median = node_left+(node_right-node_left)/2;

  /* Loop until the median of the current axis is returned. */
  while(1)
    {
      /* Pivot node acts as a reference for the distance from the desired
        (here median) node. */
      node_pivot = kdtree_make_partition(p, node_left, node_right,
                                         node_median, coordinate);
      /* If median is found, break the loop and return median node. */
      if(node_median == node_pivot) break;

      /* Change the left or right node based on the position of
         the pivot node with respect to the required median node. */
      if(node_median < node_pivot)  node_right = node_pivot - 1;
      else                          node_left  = node_pivot + 1;
    }
  /* Return the median node. */
  return node_median;
}





/* Make a kd-tree from a given set of points. For tree construction, a
   median point is selected for each axis and the left and right branches
   are recursively created by comparing points in that axis.

   Return : Indexes of the nodes in the kd-tree.
*/
static uint32_t
kdtree_fill_subtrees(struct kdtree_params *p, size_t node_left,
                     size_t node_right, size_t depth)
{
  /* Set the working axis. */
  size_t axis=depth % p->ndim;

  /* node_median is a counter over the `input_row` array.
     `input_row` array has the input_row(row number). */
  size_t node_median;

  /* Recursion terminates when the left and right nodes are the
     same. */
  if(node_left==node_right) return p->input_row[node_left];

  /* Find the median node. */
  node_median = kdtree_median_find(p, node_left, node_right,
                                   p->coords[axis]->array);

  /* node_median == 0 : We are in the lowest node (leaf) so no need
     When we only have 2 nodes and the median is equal to the left,
     its the end of the subtree.
  */
  if(node_median)
    p->left[node_median] = ( (node_median == node_left)
                             ? GAL_BLANK_UINT32
                             : kdtree_fill_subtrees(p, node_left,
                                                    node_median-1,
                                                    depth+1) );

  /* Right and left nodes are non-symytrical. Node left can be equal
     to node median when there are only 2 points and at this point,
     there can never be a single point (node left == node right).
     But node right can never be equal to node median.
     So we don't check for it. */
  p->right[node_median] = kdtree_fill_subtrees(p, node_median+1,
                                               node_right,
                                               depth+1);

  /* All subtrees have been parsed, return the node. */
  return p->input_row[node_median];
}





/* High level function to construct the kd-tree. This function initilises
   and creates the tree in top-down manner. Returns a list containing the
   indexes of left and right subtrees. */
gal_data_t *
gal_kdtree_create(gal_data_t *coords_raw, size_t *root)
{
  struct kdtree_params p={0};

  /* If there are no coordinates, just return NULL. */
  if(coords_raw->size==0) return NULL;

  /* Initialise the params structure. */
  kdtree_prepare(&p, coords_raw);

  /* Fill the kd-tree. */
  *root=kdtree_fill_subtrees(&p, 0, coords_raw->size-1, 0);

  /* For a check
  size_t i;
  for(i=0;i<coords_raw->size;++i)
    printf("%-15zu%-15u%-15u\n", p.input_row[i], p.left[i], p.right[i]);
  */

  /* Do a reverse permutation to sort the indexes
     back in the input order. */
  gal_permutation_apply_inverse(p.left_col, p.input_row);
  gal_permutation_apply_inverse(p.right_col, p.input_row);

  /* Free and clean up. */
  kdtree_cleanup(&p, coords_raw);

  /* Return results. */
  return p.left_col;
}




















/****************************************************************
 ********          Nearest-Neighbour Search               *******
 ****************************************************************/
/* This is a helper function which finds the nearest neighbour of
   the given point in a kdtree. It calculates the least distance
   from the point, and the index of that nearest node (out_nn).

   See `https://en.wikipedia.org/wiki/K-d_tree#Nearest_neighbour_search`
   for more information.
*/
static void
kdtree_nearest_neighbour(struct kdtree_params *p, uint32_t node_current,
                         double *point, double *least_dist,
                         size_t *out_nn, size_t depth)
{
  double d, dx, dx2;
  size_t axis=depth % p->ndim;    /* Set the working axis. */
  double *coordinates=p->coords[axis]->array;

  /* If no subtree present, don't search further. */
  if(node_current==GAL_BLANK_UINT32) return;

  /* The distance between search point to the current node. */
  d = kdtree_distance_find(p, node_current, point);

  /* Distance between the splitting coordinate of the search
     point and current node. */
  dx = coordinates[node_current]-point[axis];

  /* Check if the current node is nearer than the previous
     nearest node. */
  if(d < *least_dist)
    {
      *least_dist = d;
      *out_nn = node_current;
    }

  /* If exact match found (least distance 0), return it. */
  if(*least_dist==0.0f) return;

  /* Recursively search in subtrees. */
  kdtree_nearest_neighbour(p, dx > 0
                              ? p->left[node_current]
                              : p->right[node_current],
                           point, least_dist, out_nn, depth+1);

  /* Since the hyperplanes are all axis-aligned, to check if there is a
     node in other branch which is nearer to the current node is done by a
     simple comparison to see whether the distance between the splitting
     coordinate (median node) of the search point and current node is
     lesser (i.e on same side of hyperplane) than the distance (overall
     coordinates) from the search point to the current nearest. */
  dx2 = dx*dx;
  if(dx2 >= *least_dist) return;

  /* Recursively search other subtrees. */
  kdtree_nearest_neighbour(p, dx > 0
                              ? p->right[node_current]
                              : p->left[node_current],
                           point, least_dist, out_nn, depth+1);
}





/* High-level function used to find the nearest neighbour of a given
   point in a kd-tree. It calculates the least distance of the point
   from the nearest node and returns the index of that node.

   Return: The index of the nearest neighbour node in the kd-tree.
*/
size_t
gal_kdtree_nearest_neighbour(gal_data_t *coords_raw, gal_data_t *kdtree,
                             size_t root, double *point,
                             double *least_dist)
{
  struct kdtree_params p={0};
  size_t out_nn=GAL_BLANK_SIZE_T;

  /* Initialisation. */
  p.left_col=kdtree;
  *least_dist=DBL_MAX;
  kdtree_prepare(&p, coords_raw);

  /* Use the low-level function to find th nearest neighbour. */
  kdtree_nearest_neighbour(&p, root, point, least_dist, &out_nn, 0);

  /* least_dist is the square of the distance between the nearest
     neighbour and the point (used to improve processing).
     Square root of that is the actual distance. */
  *least_dist = sqrt(*least_dist);

  /* For a check
  printf("%s: root=%zu, out_nn=%zu, least_dis=%f\n",
         __func__, root, out_nn, least_dist);
  */

  /* Clean up and return. */
  kdtree_cleanup(&p, coords_raw);
  return out_nn;
}
