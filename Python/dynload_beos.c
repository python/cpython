
/* Support for dynamic loading of extension modules */

#include <kernel/image.h>
#include <kernel/OS.h>
#include <stdlib.h>

#include "Python.h"
#include "importdl.h"

const struct filedescr _PyImport_DynLoadFiletab[] = {
    {".so", "rb", C_EXTENSION},
    {"module.so", "rb", C_EXTENSION},
    {0, 0}
};

#if defined(MAXPATHLEN) && !defined(_SYS_PARAM_H)
#undef MAXPATHLEN
#endif

#ifdef WITH_THREAD
#include "pythread.h"
static PyThread_type_lock beos_dyn_lock;
#endif

static PyObject *beos_dyn_images = NULL;

/* ----------------------------------------------------------------------
 * BeOS dynamic loading support
 *
 * This uses shared libraries, but BeOS has its own way of doing things
 * (much easier than dlfnc.h, from the look of things).  We'll use a
 * Python Dictionary object to store the images_ids so we can be very
 * nice and unload them when we exit.
 *
 * Note that this is thread-safe.  Probably irrelevent, because of losing
 * systems... Python probably disables threads while loading modules.
 * Note the use of "probably"!  Better to be safe than sorry. [chrish]
 *
 * As of 1.5.1 this should also work properly when you've configured
 * Python without thread support; the 1.5 version required it, which wasn't
 * very friendly.  Note that I haven't tested it without threading... why
 * would you want to avoid threads on BeOS? [chrish]
 *
 * As of 1.5.2, the PyImport_BeImageID() function has been removed; Donn
 * tells me it's not necessary anymore because of PyCObject_Import().
 * [chrish]
 */

/* Whack an item; the item is an image_id in disguise, so we'll call
 * unload_add_on() for it.
 */
static void beos_nuke_dyn( PyObject *item )
{
    status_t retval;

    if( item ) {
        image_id id = (image_id)PyInt_AsLong( item );

        retval = unload_add_on( id );
    }
}

/* atexit() handler that'll call unload_add_on() for every item in the
 * dictionary.
 */
static void beos_cleanup_dyn( void )
{
    if( beos_dyn_images ) {
        int idx;
        int list_size;
        PyObject *id_list;

#ifdef WITH_THREAD
        PyThread_acquire_lock( beos_dyn_lock, 1 );
#endif

        id_list = PyDict_Values( beos_dyn_images );

        list_size = PyList_Size( id_list );
        for( idx = 0; idx < list_size; idx++ ) {
            PyObject *the_item;

            the_item = PyList_GetItem( id_list, idx );
            beos_nuke_dyn( the_item );
        }

        PyDict_Clear( beos_dyn_images );

#ifdef WITH_THREAD
        PyThread_free_lock( beos_dyn_lock );
#endif
    }
}

/*
 * Initialize our dictionary, and the dictionary mutex.
 */
static void beos_init_dyn( void )
{
    /* We're protected from a race condition here by the atomic init_count
     * variable.
     */
    static int32 init_count = 0;
    int32 val;

    val = atomic_add( &init_count, 1 );
    if( beos_dyn_images == NULL && val == 0 ) {
        beos_dyn_images = PyDict_New();
#ifdef WITH_THREAD
        beos_dyn_lock = PyThread_allocate_lock();
#endif
        atexit( beos_cleanup_dyn );
    }
}

/*
 * Add an image_id to the dictionary; the module name of the loaded image
 * is the key.  Note that if the key is already in the dict, we unload
 * that image; this should allow reload() to work on dynamically loaded
 * modules (super-keen!).
 */
static void beos_add_dyn( char *name, image_id id )
{
    int retval;
    PyObject *py_id;

    if( beos_dyn_images == NULL ) {
        beos_init_dyn();
    }

#ifdef WITH_THREAD
    retval = PyThread_acquire_lock( beos_dyn_lock, 1 );
#endif

    /* If there's already an object with this key in the dictionary,
     * we're doing a reload(), so let's nuke it.
     */
    py_id = PyDict_GetItemString( beos_dyn_images, name );
    if( py_id ) {
        beos_nuke_dyn( py_id );
        retval = PyDict_DelItemString( beos_dyn_images, name );
    }

    py_id = PyInt_FromLong( (long)id );
    if( py_id ) {
        retval = PyDict_SetItemString( beos_dyn_images, name, py_id );
    }

#ifdef WITH_THREAD
    PyThread_release_lock( beos_dyn_lock );
#endif
}



dl_funcptr _PyImport_GetDynLoadFunc(const char *fqname, const char *shortname,
                                    const char *pathname, FILE *fp)
{
    dl_funcptr p;
    image_id the_id;
    status_t retval;
    char fullpath[PATH_MAX];
    char funcname[258];

    if( Py_VerboseFlag ) {
        printf( "load_add_on( %s )\n", pathname );
    }

    /* Hmm, this old bug appears to have regenerated itself; if the
     * path isn't absolute, load_add_on() will fail.  Reported to Be
     * April 21, 1998.
     */
    if( pathname[0] != '/' ) {
        (void)getcwd( fullpath, PATH_MAX );
        (void)strncat( fullpath, "/", PATH_MAX );
        (void)strncat( fullpath, pathname, PATH_MAX );

        if( Py_VerboseFlag ) {
            printf( "load_add_on( %s )\n", fullpath );
        }
    } else {
        (void)strcpy( fullpath, pathname );
    }

    the_id = load_add_on( fullpath );
    if( the_id < B_NO_ERROR ) {
        /* It's too bad load_add_on() doesn't set errno or something...
         */
        char buff[256];  /* hate hard-coded string sizes... */

        if( Py_VerboseFlag ) {
            printf( "load_add_on( %s ) failed", fullpath );
        }

        if( the_id == B_ERROR )
            PyOS_snprintf( buff, sizeof(buff),
                           "BeOS: Failed to load %.200s",
                           fullpath );
        else
            PyOS_snprintf( buff, sizeof(buff),
                           "Unknown error loading %.200s",
                           fullpath );

        PyErr_SetString( PyExc_ImportError, buff );
        return NULL;
    }

    PyOS_snprintf(funcname, sizeof(funcname), "init%.200s", shortname);
    if( Py_VerboseFlag ) {
        printf( "get_image_symbol( %s )\n", funcname );
    }

    retval = get_image_symbol( the_id, funcname, B_SYMBOL_TYPE_TEXT, &p );
    if( retval != B_NO_ERROR || p == NULL ) {
        /* That's bad, we can't find that symbol in the module...
         */
        char buff[256];  /* hate hard-coded string sizes... */

        if( Py_VerboseFlag ) {
            printf( "get_image_symbol( %s ) failed", funcname );
        }

        switch( retval ) {
        case B_BAD_IMAGE_ID:
            PyOS_snprintf( buff, sizeof(buff),
                   "can't load init function for dynamic module: "
                   "Invalid image ID for %.180s", fullpath );
            break;
        case B_BAD_INDEX:
            PyOS_snprintf( buff, sizeof(buff),
                   "can't load init function for dynamic module: "
                   "Bad index for %.180s", funcname );
            break;
        default:
            PyOS_snprintf( buff, sizeof(buff),
                   "can't load init function for dynamic module: "
                   "Unknown error looking up %.180s", funcname );
            break;
        }

        retval = unload_add_on( the_id );

        PyErr_SetString( PyExc_ImportError, buff );
        return NULL;
    }

    /* Save the module name and image ID for later so we can clean up
     * gracefully.
     */
    beos_add_dyn( fqname, the_id );

    return p;
}
