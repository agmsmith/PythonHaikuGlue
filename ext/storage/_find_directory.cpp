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
	PyDict_SetItemString( dict, "B_SYSTEM_DIRECTORY", PyInt_FromLong( B_SYSTEM_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_ADDONS_DIRECTORY", PyInt_FromLong( B_SYSTEM_ADDONS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_BOOT_DIRECTORY", PyInt_FromLong( B_SYSTEM_BOOT_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_FONTS_DIRECTORY", PyInt_FromLong( B_SYSTEM_FONTS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_LIB_DIRECTORY", PyInt_FromLong( B_SYSTEM_LIB_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_SERVERS_DIRECTORY", PyInt_FromLong( B_SYSTEM_SERVERS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_APPS_DIRECTORY", PyInt_FromLong( B_SYSTEM_APPS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_BIN_DIRECTORY", PyInt_FromLong( B_SYSTEM_BIN_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_DOCUMENTATION_DIRECTORY", PyInt_FromLong( B_SYSTEM_DOCUMENTATION_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_PREFERENCES_DIRECTORY", PyInt_FromLong( B_SYSTEM_PREFERENCES_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_TRANSLATORS_DIRECTORY", PyInt_FromLong( B_SYSTEM_TRANSLATORS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_MEDIA_NODES_DIRECTORY", PyInt_FromLong( B_SYSTEM_MEDIA_NODES_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_SOUNDS_DIRECTORY", PyInt_FromLong( B_SYSTEM_SOUNDS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_DATA_DIRECTORY", PyInt_FromLong( B_SYSTEM_DATA_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_DEVELOP_DIRECTORY", PyInt_FromLong( B_SYSTEM_DEVELOP_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_PACKAGES_DIRECTORY", PyInt_FromLong( B_SYSTEM_PACKAGES_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_HEADERS_DIRECTORY", PyInt_FromLong( B_SYSTEM_HEADERS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_ETC_DIRECTORY", PyInt_FromLong( B_SYSTEM_ETC_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_SETTINGS_DIRECTORY", PyInt_FromLong( B_SYSTEM_SETTINGS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_LOG_DIRECTORY", PyInt_FromLong( B_SYSTEM_LOG_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_SPOOL_DIRECTORY", PyInt_FromLong( B_SYSTEM_SPOOL_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_TEMP_DIRECTORY", PyInt_FromLong( B_SYSTEM_TEMP_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_VAR_DIRECTORY", PyInt_FromLong( B_SYSTEM_VAR_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_CACHE_DIRECTORY", PyInt_FromLong( B_SYSTEM_CACHE_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_NONPACKAGED_DIRECTORY", PyInt_FromLong( B_SYSTEM_NONPACKAGED_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_NONPACKAGED_ADDONS_DIRECTORY", PyInt_FromLong( B_SYSTEM_NONPACKAGED_ADDONS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_NONPACKAGED_TRANSLATORS_DIRECTORY", PyInt_FromLong( B_SYSTEM_NONPACKAGED_TRANSLATORS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_NONPACKAGED_MEDIA_NODES_DIRECTORY", PyInt_FromLong( B_SYSTEM_NONPACKAGED_MEDIA_NODES_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_NONPACKAGED_BIN_DIRECTORY", PyInt_FromLong( B_SYSTEM_NONPACKAGED_BIN_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_NONPACKAGED_DATA_DIRECTORY", PyInt_FromLong( B_SYSTEM_NONPACKAGED_DATA_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_NONPACKAGED_FONTS_DIRECTORY", PyInt_FromLong( B_SYSTEM_NONPACKAGED_FONTS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_NONPACKAGED_SOUNDS_DIRECTORY", PyInt_FromLong( B_SYSTEM_NONPACKAGED_SOUNDS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_NONPACKAGED_DOCUMENTATION_DIRECTORY", PyInt_FromLong( B_SYSTEM_NONPACKAGED_DOCUMENTATION_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_NONPACKAGED_LIB_DIRECTORY", PyInt_FromLong( B_SYSTEM_NONPACKAGED_LIB_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_NONPACKAGED_HEADERS_DIRECTORY", PyInt_FromLong( B_SYSTEM_NONPACKAGED_HEADERS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_SYSTEM_NONPACKAGED_DEVELOP_DIRECTORY", PyInt_FromLong( B_SYSTEM_NONPACKAGED_DEVELOP_DIRECTORY ) );
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
	PyDict_SetItemString( dict, "B_USER_DATA_DIRECTORY", PyInt_FromLong( B_USER_DATA_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_CACHE_DIRECTORY", PyInt_FromLong( B_USER_CACHE_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_PACKAGES_DIRECTORY", PyInt_FromLong( B_USER_PACKAGES_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_HEADERS_DIRECTORY", PyInt_FromLong( B_USER_HEADERS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_NONPACKAGED_DIRECTORY", PyInt_FromLong( B_USER_NONPACKAGED_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_NONPACKAGED_ADDONS_DIRECTORY", PyInt_FromLong( B_USER_NONPACKAGED_ADDONS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_NONPACKAGED_TRANSLATORS_DIRECTORY", PyInt_FromLong( B_USER_NONPACKAGED_TRANSLATORS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_NONPACKAGED_MEDIA_NODES_DIRECTORY", PyInt_FromLong( B_USER_NONPACKAGED_MEDIA_NODES_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_NONPACKAGED_BIN_DIRECTORY", PyInt_FromLong( B_USER_NONPACKAGED_BIN_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_NONPACKAGED_DATA_DIRECTORY", PyInt_FromLong( B_USER_NONPACKAGED_DATA_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_NONPACKAGED_FONTS_DIRECTORY", PyInt_FromLong( B_USER_NONPACKAGED_FONTS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_NONPACKAGED_SOUNDS_DIRECTORY", PyInt_FromLong( B_USER_NONPACKAGED_SOUNDS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_NONPACKAGED_DOCUMENTATION_DIRECTORY", PyInt_FromLong( B_USER_NONPACKAGED_DOCUMENTATION_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_NONPACKAGED_LIB_DIRECTORY", PyInt_FromLong( B_USER_NONPACKAGED_LIB_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_NONPACKAGED_HEADERS_DIRECTORY", PyInt_FromLong( B_USER_NONPACKAGED_HEADERS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_NONPACKAGED_DEVELOP_DIRECTORY", PyInt_FromLong( B_USER_NONPACKAGED_DEVELOP_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_DEVELOP_DIRECTORY", PyInt_FromLong( B_USER_DEVELOP_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_DOCUMENTATION_DIRECTORY", PyInt_FromLong( B_USER_DOCUMENTATION_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_SERVERS_DIRECTORY", PyInt_FromLong( B_USER_SERVERS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_APPS_DIRECTORY", PyInt_FromLong( B_USER_APPS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_BIN_DIRECTORY", PyInt_FromLong( B_USER_BIN_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_PREFERENCES_DIRECTORY", PyInt_FromLong( B_USER_PREFERENCES_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_ETC_DIRECTORY", PyInt_FromLong( B_USER_ETC_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_LOG_DIRECTORY", PyInt_FromLong( B_USER_LOG_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_SPOOL_DIRECTORY", PyInt_FromLong( B_USER_SPOOL_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_USER_VAR_DIRECTORY", PyInt_FromLong( B_USER_VAR_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_APPS_DIRECTORY", PyInt_FromLong( B_APPS_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_PREFERENCES_DIRECTORY", PyInt_FromLong( B_PREFERENCES_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_UTILITIES_DIRECTORY", PyInt_FromLong( B_UTILITIES_DIRECTORY ) );
	PyDict_SetItemString( dict, "B_PACKAGE_LINKS_DIRECTORY", PyInt_FromLong( B_PACKAGE_LINKS_DIRECTORY ) );
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

	return mod;
}
