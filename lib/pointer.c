/*********************************************************************
pointer -- facilitate working with pointers and allocation.
This is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
Copyright (C) 2016-2024 Free Software Foundation, Inc.

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
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <gnuastro/type.h>
#include <gnuastro/pointer.h>

#include <gnuastro-internal/checkset.h>


/* Increment a give pointer depending on the given type.

   When working with the 'array' elements of 'gal_data_t', we are actually
   dealing with 'void *' pointers. Pointer arithmetic doesn't apply to
   'void *', because the system doesn't know how much space each element
   has to increment the pointer respectively.

   So, here, we will use the type information to find the increment. This
   is mainly useful when dealing with the 'block' pointer of a tile over a
   larger image. This function reads the address as a 'char *' type (note
   that 'char' is guaranteed to have a size of 1 (byte)). It then
   increments the 'char *' by 'increment*sizeof(type)'. */
void *
gal_pointer_increment(void *pointer, size_t increment, uint8_t type)
{
  char *p=(char *)pointer;
  return p + increment * gal_type_sizeof(type);
}





/* Find the number of values between two void pointers with a given
   type. See the explanations before 'gal_data_ptr_increment'. */
size_t
gal_pointer_num_between(void *earlier, void *later, uint8_t type)
{
  char *e=(char *)earlier, *l=(char *)later;
  return (l-e)/gal_type_sizeof(type);
}





/* Allocate an array based on the value of type. Note that the argument
   'size' is the number of elements, necessary in the array, the number of
   bytes each element needs will be determined internaly by this function
   using the datatype argument, so you don't have to worry about it. */
void *
gal_pointer_allocate(uint8_t type, size_t size, int clear,
                     const char *funcname, const char *varname)
{
  void *array;

  errno=0;
  array = ( clear
            ? calloc( size,  gal_type_sizeof(type) )
            : malloc( size * gal_type_sizeof(type) ) );
  if(array==NULL)
    {
      if(varname)
        error(EXIT_FAILURE, errno, "%s: %zu bytes couldn't be allocated "
              "for variable '%s'", funcname ? funcname : __func__,
              size * gal_type_sizeof(type), varname);
      else
        error(EXIT_FAILURE, errno, "%s: %zu bytes couldn't be allocated",
              funcname ? funcname : __func__, size * gal_type_sizeof(type));
    }

  return array;
}





void *
gal_pointer_mmap_allocate(uint8_t type, size_t size, int clear,
                          char **filename, int quietmmap,
                          int allocfailed)
{
  void *out;
  int filedes;
  uint8_t uc=0;
  char *dirname=NULL;
  size_t bsize=size*gal_type_sizeof(type);


  /* Check if the 'gnuastro_mmap' folder exists, write the file there. If
     it doesn't exist, then make it. If it can't be built, we'll make a
     randomly named file in the current directory. */
  gal_checkset_allocate_copy("./gnuastro_mmap/", &dirname);
  if( gal_checkset_mkdir(dirname) )
    {
      /* The directory couldn't be built. Free the old name. */
      free(dirname);

      /* Set 'dirname' to NULL so it knows not to write in a directory. */
      dirname=NULL;
    }


  /* Set the filename. If 'dirname' couldn't be allocated, directly make
     the memory map file in the current directory (just as a hidden
     file). */
  if( asprintf(filename, "%sXXXXXX", dirname?dirname:"./gnuastro_mmap_")<0 )
    error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
  if(dirname) free(dirname);


  /* Create a zero-sized file and keep its descriptor. */
  errno=0;
  /*filedes=open(filename, O_RDWR | O_CREAT | O_EXCL | O_TRUNC );*/
  filedes=mkstemp(*filename);
  if(filedes==-1)
    error(EXIT_FAILURE, errno, "%s: %s couldn't be created", __func__,
          *filename);


  /* Make the necessary space on the file. */
  errno=0;
  if( lseek(filedes, bsize, SEEK_SET) == -1 )
    error(EXIT_FAILURE, errno, "%s: %s: unable to change file position by "
          "%zu bytes", __func__, *filename, bsize);


  /* Inform the user. */
  if(!quietmmap)
    error(EXIT_SUCCESS, 0, "%s: temporary memory-mapped file (%zu bytes) "
          "created for intermediate data that is not stored in RAM (see "
          "the \"Memory management\" section of Gnuastro's manual for "
          "optimizing your project's memory management, and thus speed). "
          "To disable this warning, please use the option '--quiet-mmap'",
          *filename, bsize);


  /* Write to the newly set file position so the space is allocated. To do
     this, we are simply writing 'uc' (a byte with value 0) into the space
     we identified by 'lseek' (above). This will ensure that this space is
     set a side for this array and prepare us to use 'mmap'. */
  if( write(filedes, &uc, 1) == -1)
    error(EXIT_FAILURE, errno, "%s: %s: unable to write one byte at the "
          "%zu-th position", __func__, *filename, bsize);


  /* Map the memory. */
  errno=0;
  out=mmap(NULL, bsize, PROT_READ | PROT_WRITE, MAP_SHARED, filedes, 0);
  if(out==MAP_FAILED)
    {
      if(allocfailed)
        error(EXIT_FAILURE, 0, "%s: memory could not be allocated for "
              "%zu bytes (with 'malloc' or 'calloc') while there is "
              "space available in the RAM! A subsequent attempt to use "
              "memory-mapping also failed ('%s' was created). If this "
              "error is reported within a queue of submitted jobs on a "
              "High Performance Computing (HPC) facility, it is most "
              "probably due to resource restrictions imposed by the "
              "job scheduler. For example if you use the Slurm Workload "
              "Manager you should add '--mem=<float>G' (per node, where "
              "'<float>' should be replaced by a the amount of memory "
              "your require in Gigabytes) or '–mem-per-cpu=<float>G' to "
              "your job submitting command ('sbatch'). If you use the "
              "Sun Grid System (SGE) you should add "
              "'-l h_vmem=<float>G' to your 'qsub' command", __func__,
              bsize, *filename);
      else
        {
          /* This is using 'fprintf' first to avoid confusing the long
             description of the potential solution with the actual error
             message (that is printed with 'error'). */
          fprintf(stderr, "\n%s: WARNING: the following error may be "
                  "due to many mmap allocations. Recall that the kernel "
                  "only allows finite number of mmap allocations. It is "
                  "recommended to use ordinary RAM allocation for smaller "
                  "arrays and keep mmap'd allocation only for the large "
                  "volumes.\n\n", __func__);
          error(EXIT_FAILURE, errno, "couldn't map %zu bytes into the "
                "file '%s'", bsize, *filename);
        }
    }


  /* Close the file. */
  if( close(filedes) == -1 )
    error(EXIT_FAILURE, errno, "%s: %s couldn't be closed",
          __func__, *filename);


  /* If it was supposed to be cleared, then clear the memory. */
  if(clear) memset(out, 0, bsize);


  /* Return the mmap'd pointer and save the file name. */
  return out;
}





void
gal_pointer_mmap_free(char **mmapname, int quietmmap)
{
  /* Delete the file keeping the array. */
  remove(*mmapname);

  /* Inform the user. */
  if(!quietmmap)
    error(EXIT_SUCCESS, 0, "%s: deleted", *mmapname);

  /* Free the file name space. */
  free(*mmapname);

  /* Set the name pointer to NULL since it has been freed. */
  *mmapname=NULL;
}

#if GAL_CONFIG_HAVE_OPENCL
void *
gal_pointer_allocate_ram_or_mmap_cl(uint8_t type, size_t size, int clear,
                                 size_t minmapsize, char **mmapname,
                                 int quietmmap, const char *funcname,
                                 const char *varname, cl_context context)
{
  void *out;
  size_t bytesize=gal_type_sizeof(type)*size;

  /* See if the requested size is larger than 1MB (otherwise,
     its not worth checking RAM, which involves reading a text
     file, we won't try memory-mapping anyway). */

  /* If it is decided to do memory-mapping, then do it. */
  printf("Allocating\n");
  if( gal_checkset_need_mmap(bytesize, minmapsize, quietmmap) )
    out=gal_pointer_mmap_allocate(type, size, clear, mmapname,
                                  quietmmap, 0);
  else
    {
      /* Allocate the necessary space in the RAM. */
      errno=0;
      // if(context == NUL)
      // // out = ( clear
      // //         ? calloc( size,  gal_type_sizeof(type) )
      // //         : malloc( size * gal_type_sizeof(type) ) );
      out = clSVMAlloc(context, CL_MEM_READ_WRITE, size * gal_type_sizeof(type), 0);
      if(out == NULL)
      {
        printf("Abort");
        exit(1);
      }
      /* If the array is NULL (there was no RAM left: on
         systems other than Linux, 'malloc' will actually
         return NULL, Linux doesn't do this unfortunately so we
         need to read the available RAM). */
      if(out==NULL)
        out=gal_pointer_mmap_allocate(type, size, clear,
                                      mmapname, quietmmap, 1);

      /* The 'errno' is re-set to zero just in case 'malloc'
         changed it, which may cause problems later. */
      errno=0;
    }

  /* Return the allocated dataset. */
  return out;
}

#endif




void *
gal_pointer_allocate_ram_or_mmap(uint8_t type, size_t size, int clear,
                                 size_t minmapsize, char **mmapname,
                                 int quietmmap, const char *funcname,
                                 const char *varname)
{
  void *out;
  size_t bytesize=gal_type_sizeof(type)*size;

  /* See if the requested size is larger than 1MB (otherwise,
     its not worth checking RAM, which involves reading a text
     file, we won't try memory-mapping anyway). */

  /* If it is decided to do memory-mapping, then do it. */
  if( gal_checkset_need_mmap(bytesize, minmapsize, quietmmap) )
    out=gal_pointer_mmap_allocate(type, size, clear, mmapname,
                                  quietmmap, 0);
  else
    {
      /* Allocate the necessary space in the RAM. */
      errno=0;
      out = ( clear
              ? calloc( size,  gal_type_sizeof(type) )
              : malloc( size * gal_type_sizeof(type) ) );

      /* If the array is NULL (there was no RAM left: on
         systems other than Linux, 'malloc' will actually
         return NULL, Linux doesn't do this unfortunately so we
         need to read the available RAM). */
      if(out==NULL)
        out=gal_pointer_mmap_allocate(type, size, clear,
                                      mmapname, quietmmap, 1);

      /* The 'errno' is re-set to zero just in case 'malloc'
         changed it, which may cause problems later. */
      errno=0;
    }

  /* Return the allocated dataset. */
  return out;
}
