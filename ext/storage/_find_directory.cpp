// BeOS.find_directory
//
// Copyright © 1999 Arcane Dragon Software (arcanedragon@pobox.com)
//                  All Right Reserved
//
// Copyright (C) 2005 Mikael Jansson <apps@mikael.jansson.be>
//

#include "Python.h"

#include <storage/FindDirectory.h>
#include <storage/Path.h>
#include <errno.h>	// for errno
#include <string.h>	// for strerror()
#include <limits.h>

#include <strstream>

// ----------------------------------------------------------------------
// Find the specified directory.
//
// args:
// 	which
//  create_it = 0 (optional)

static PyObject *bfs_find_directory( PyObject *self, PyObject *args )
{
	// self isn't used for normal functions
	self = self;

	int which = -1;
	int create_it = 0;

	if( !PyArg_ParseTuple( args, "i|i", &which, &create_it ) ) {
		// two arguments, one is optional
		PyErr_SetString( PyExc_TypeError, "you must specify a directory constant" );
		return NULL;
	}
	
	BPath path;
	status_t retval = find_directory( static_cast<directory_which>( which ),
									  &path,
									  create_it ? true : false );
	if( B_OK != retval ) {
		try {
			strstream s;
			s << "error finding directory (" << strerror( retval ) << ")" << ends;
			PyErr_SetString( PyExc_IOError, s.str() );
		} catch ( ... ) {
			PyErr_SetString( PyExc_IOError, strerror( retval ) );
		}

		return NULL;
	}
	
	PyObject *the_dir = PyString_FromString( path.Path() );
	if( NULL == the_dir ) return PyErr_NoMemory();

	Py_INCREF( the_dir );
	return the_dir;
}

// ----------------------------------------------------------------------
// List of functions defined in the module
static PyMethodDef find_directory_methods[] = {
	{
		"find_directory",
		bfs_find_directory,
		METH_VARARGS,
		"find_directory( which )\n" \
		"\n" \
		"Finds the specified directory; which must be one of the B_*_DIRECTORY\n" \
		"constants." \
	},
	{ // sentinel
		NULL,	// name
		NULL,	// function
		0,		// flags
		""		// docstring
	}
};

// ----------------------------------------------------------------------
// Initialization function for the find_directory module
extern "C" DL_EXPORT(PyObject *) init_find_directory( void )
{
	// Create the module and add the functions
	PyObject *mod = Py_InitModule4( "_find_directory", find_directory_methods, 
									"BeOS standard directories:\n" \
									"\n" \
									"find_directory() - find a specified directory\n",
									static_cast<PyObject *>( NULL ),
									PYTHON_API_VERSION );

	// Add some symbolic constants to the module
	PyObject *mdict = PyModule_GetDict( mod );
	
	PyObject *dict = PyDict_New();
	PyDict_SetItemString(mdict, "directory_which", dict);

	// Why look, a whole bunch of untested object constructors...
	PyDict_SetItemString( dict, "B_DESKTOP_DIRECTORY", PyInt_FromLong( B_DESKTOP_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_TRASH_DIRECTORY", PyInt_FromLong( B_TRASH_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_BEOS_DIRECTORY", PyInt_FromLong( B_BEOS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_BEOS_SYSTEM_DIRECTORY", PyInt_FromLong( B_BEOS_SYSTEM_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_BEOS_ADDONS_DIRECTORY", PyInt_FromLong( B_BEOS_ADDONS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_BEOS_BOOT_DIRECTORY", PyInt_FromLong( B_BEOS_BOOT_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_BEOS_FONTS_DIRECTORY", PyInt_FromLong( B_BEOS_FONTS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_BEOS_LIB_DIRECTORY", PyInt_FromLong( B_BEOS_LIB_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_BEOS_SERVERS_DIRECTORY", PyInt_FromLong( B_BEOS_SERVERS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_BEOS_APPS_DIRECTORY", PyInt_FromLong( B_BEOS_APPS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_BEOS_BIN_DIRECTORY", PyInt_FromLong( B_BEOS_BIN_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_BEOS_ETC_DIRECTORY", PyInt_FromLong( B_BEOS_ETC_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_BEOS_DOCUMENTATION_DIRECTORY", PyInt_FromLong( B_BEOS_DOCUMENTATION_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_BEOS_PREFERENCES_DIRECTORY", PyInt_FromLong( B_BEOS_PREFERENCES_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_BEOS_TRANSLATORS_DIRECTORY", PyInt_FromLong( B_BEOS_TRANSLATORS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_BEOS_MEDIA_NODES_DIRECTORY", PyInt_FromLong( B_BEOS_MEDIA_NODES_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_BEOS_SOUNDS_DIRECTORY", PyInt_FromLong( B_BEOS_SOUNDS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_DIRECTORY", PyInt_FromLong( B_COMMON_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_SYSTEM_DIRECTORY", PyInt_FromLong( B_COMMON_SYSTEM_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_ADDONS_DIRECTORY", PyInt_FromLong( B_COMMON_ADDONS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_BOOT_DIRECTORY", PyInt_FromLong( B_COMMON_BOOT_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_FONTS_DIRECTORY", PyInt_FromLong( B_COMMON_FONTS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_LIB_DIRECTORY", PyInt_FromLong( B_COMMON_LIB_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_SERVERS_DIRECTORY", PyInt_FromLong( B_COMMON_SERVERS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_BIN_DIRECTORY", PyInt_FromLong( B_COMMON_BIN_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_ETC_DIRECTORY", PyInt_FromLong( B_COMMON_ETC_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_DOCUMENTATION_DIRECTORY", PyInt_FromLong( B_COMMON_DOCUMENTATION_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_SETTINGS_DIRECTORY", PyInt_FromLong( B_COMMON_SETTINGS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_DEVELOP_DIRECTORY", PyInt_FromLong( B_COMMON_DEVELOP_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_LOG_DIRECTORY", PyInt_FromLong( B_COMMON_LOG_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_SPOOL_DIRECTORY", PyInt_FromLong( B_COMMON_SPOOL_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_TEMP_DIRECTORY", PyInt_FromLong( B_COMMON_TEMP_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_VAR_DIRECTORY", PyInt_FromLong( B_COMMON_VAR_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_TRANSLATORS_DIRECTORY", PyInt_FromLong( B_COMMON_TRANSLATORS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_MEDIA_NODES_DIRECTORY", PyInt_FromLong( B_COMMON_MEDIA_NODES_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_COMMON_SOUNDS_DIRECTORY", PyInt_FromLong( B_COMMON_SOUNDS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_DIRECTORY", PyInt_FromLong( B_USER_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_CONFIG_DIRECTORY", PyInt_FromLong( B_USER_CONFIG_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_ADDONS_DIRECTORY", PyInt_FromLong( B_USER_ADDONS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_BOOT_DIRECTORY", PyInt_FromLong( B_USER_BOOT_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_FONTS_DIRECTORY", PyInt_FromLong( B_USER_FONTS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_LIB_DIRECTORY", PyInt_FromLong( B_USER_LIB_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_SETTINGS_DIRECTORY", PyInt_FromLong( B_USER_SETTINGS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_DESKBAR_DIRECTORY", PyInt_FromLong( B_USER_DESKBAR_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_PRINTERS_DIRECTORY", PyInt_FromLong( B_USER_PRINTERS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_TRANSLATORS_DIRECTORY", PyInt_FromLong( B_USER_TRANSLATORS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_MEDIA_NODES_DIRECTORY", PyInt_FromLong( B_USER_MEDIA_NODES_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_SOUNDS_DIRECTORY", PyInt_FromLong( B_USER_SOUNDS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_APPS_DIRECTORY", PyInt_FromLong( B_APPS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_PREFERENCES_DIRECTORY", PyInt_FromLong( B_PREFERENCES_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_UTILITIES_DIRECTORY", PyInt_FromLong( B_UTILITIES_DIRECTORY ) );

	return mod;
}
