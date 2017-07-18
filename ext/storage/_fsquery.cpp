// BeOS.fsquery
//
// Copyright © 1999 Arcane Dragon Software (arcanedragon@pobox.com)
//                  All Right Reserved
//
// Copyright (C) 2005 Mikael Jansson <apps@mikael.jansson.be>
//

#include "Python.h"

#include <kernel/OS.h>			// for port_id in fs_query.h... tsk tsk.
#include <kernel/fs_query.h>
#include <kernel/fs_info.h>
#include <storage/StorageDefs.h>
#include <errno.h>	// for errno
#include <string.h>	// for strerror()
#include <limits.h>

#include <strstream>

// ----------------------------------------------------------------------
// Perform a query
//
// args:
// 	query
//  volume = /boot (optional)
//  flags = 0 (optional)

static PyObject *bfs_query( PyObject *self, PyObject *args )
{
	// self isn't used for normal functions
	self = self;

	char *query;
	char *volume = NULL;
	uint32 flags = 0;
	dev_t vol_dev = dev_for_path( "/boot" );

	if( PyArg_ParseTuple( args, "s|si", &query, &volume, &flags ) ) {
		// three arguments, two are optional
		if( NULL != volume ) vol_dev = dev_for_path( volume );
		if( 0 != flags ) {
			PyErr_SetString( PyExc_ValueError, "don't use flags" );
			return NULL;
		}
	} else {
		PyErr_SetString( PyExc_TypeError, "you must specify a query string" );
		return NULL;
	}

	DIR *qdir = fs_open_query( vol_dev, query, flags );
	if( NULL == qdir ) {
		try {
			strstream s;
			s << "error with query \"" << query << "\": "
			  << strerror( errno ) << ends;
			PyErr_SetString( PyExc_RuntimeError, s.str() );
		} catch ( ... ) {
			PyErr_SetString( PyExc_RuntimeError, strerror( errno ) );
		}

		return NULL;
	}

	PyObject *query_list = PyList_New( 0 );
	if( NULL == query_list ) {
		(void)fs_close_query( qdir );
		return PyErr_NoMemory();
	}

	struct dirent *qent;
	while( NULL != ( qent = fs_read_query( qdir ) ) ) {
		char buff[B_PATH_NAME_LENGTH];
		status_t retval = get_path_for_dirent( qent, buff, B_PATH_NAME_LENGTH );
		if( retval != B_OK ) continue;	// throw an exception instead?

		PyObject *entry = PyString_FromString( buff );
		if( NULL == entry ) {
			(void)fs_close_query( qdir );
			return PyErr_NoMemory();
		}

		if( PyList_Append( query_list, entry ) ) continue;	// throw an exception instead/
	}

	(void)fs_close_query( qdir );
	
	Py_INCREF( query_list );
	return query_list;
}

// ----------------------------------------------------------------------
// List of functions defined in the module
static PyMethodDef fsquery_methods[] = {
	{
		"query",
		bfs_query,
		METH_VARARGS,
		"query( query_string, device = \"/boot\", flags = 0 )\n" \
		"\n" \
		"Perform a one-shot query.  The query_string must be a standard BeOS\n" \
		"query, specified as a string.  Device can be any path, and defaults\n" \
		"to your boot volume; it specifies the volume that will be queried.\n" \
		"flags must currently be 0, so don't bother specifying it.\n" \
		"\n" \
		"Returns a list of paths."
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
extern "C" DL_EXPORT(PyObject *) init_fsquery( void )
{
	// Create the module and add the functions
	PyObject *mod = Py_InitModule4( "_fsquery", fsquery_methods, 
									"Filesystem queries:\n" \
									"\n" \
									"query() - perform a query\n",
									static_cast<PyObject *>( NULL ),
									PYTHON_API_VERSION );

	// Add some symbolic constants to the module
	PyObject *dict = PyModule_GetDict( mod );
	PyDict_SetItemString( dict, "__rcs_id__", 
						  PyString_FromString( "$Id: fsquerymodule.cpp,v 1.1 1999/10/08 17:44:36 chrish Exp $" ) );

	return mod;
}
