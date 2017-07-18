// BeOS.fsattrmodule
//
// Copyright © 1999 Arcane Dragon Software, All Right Reserved
//
// Copyright (C) 2005 Mikael Jansson <apps@mikael.jansson.be>
//

#include "Python.h"

#include <kernel/fs_attr.h>
#include <kernel/fs_info.h>
#include <support/TypeConstants.h>	// Type constants except:
#include <storage/Mime.h>			// B_MIME_STRING_TYPE is here instead
#include <malloc.h>
#include <interface/Point.h>
#include <interface/Rect.h>
#include <interface/GraphicsDefs.h>
#include <storage/Entry.h>
#include <storage/Path.h>
#include <storage/StorageDefs.h>
#include <errno.h>	// for errno
#include <string.h>	// for strerror()
#include <limits.h>
#include <support/ByteOrder.h>

#include <strstream>

// ----------------------------------------------------------------------
// Some useful constants
#define ATTR_SYMLINK		0x00000001
#define ATTR_BIG_ENDIAN		0x00000002
#define ATTR_LITTLE_ENDIAN	0x00000004

// ----------------------------------------------------------------------
// Load the file attributes for a file/directory/symlink into a dictionary
// of tuples; each tuple is ( type, data ), the key is the attribute name.
//
// args:
// 	filename
//  flags = 0 (optional)

static PyObject *bfs_read_attrs( PyObject *self, PyObject *args )
{
	// self isn't used for normal functions
	self = self;

	char *filename;
	int mode = O_RDONLY;
	int flags = 0;

	if( PyArg_ParseTuple( args, "s|i", &filename, &flags ) ) {
		// two arguments, last is optional
		if( flags & ATTR_SYMLINK ) mode |= O_NOTRAVERSE;
		if( ( flags & ATTR_BIG_ENDIAN ) && ( flags & ATTR_LITTLE_ENDIAN ) ) {
			PyErr_SetString( PyExc_ValueError, 
							 "can't specify ATTR_BIG_ENDIAN and ATTR_LITTLE_ENDIAN, it's just not right" );
			return NULL;
		}
	} else {
		PyErr_SetString( PyExc_TypeError, "you must specify a path name" );
		return NULL;
	}
	
	int fd = open( filename, mode );
	if( fd < 0 ) {
		try {
			strstream s;
			s << "can't open file: " << filename \
			  << " (" << strerror( errno ) << ")" << ends;
			PyErr_SetString( PyExc_IOError, s.str() );
		} catch ( ... ) {
			PyErr_SetString( PyExc_IOError, strerror( errno ) );
		}

		return NULL;
	}
	
	DIR *fa_dir = fs_fopen_attr_dir( fd );
	if( fa_dir == NULL ) {
		close( fd );

		try {
			strstream s;
			s << "can't open file's attributes: " << filename \
			  << " (" << strerror( errno ) << ")" << ends;
			PyErr_SetString( PyExc_IOError, s.str() );
		} catch ( ... ) {
			PyErr_SetString( PyExc_IOError, "can't open file's attributes" );
		}

		return NULL;
	}

	PyObject *attributes = PyDict_New();
	uint32 attributes_added = 0;
	if( attributes == NULL ) {
		(void)fs_close_attr_dir( fa_dir );
		close( fd );

		return PyErr_NoMemory();
	}
	
	struct dirent *fa_ent = fs_read_attr_dir( fa_dir );
	while( fa_ent != NULL ) {
		struct attr_info fa_info;
		status_t retval = fs_stat_attr( fd, fa_ent->d_name, &fa_info );
		
		if( retval == B_OK ) {
			char *ptr = (char *)malloc( fa_info.size );
			if( ptr == NULL ) {
				(void)fs_close_attr_dir( fa_dir );
				close( fd );

				return PyErr_NoMemory();
			}
			
			ssize_t read_bytes = fs_read_attr( fd, 
											   fa_ent->d_name, fa_info.type, 
											   0, ptr, fa_info.size );
			if( read_bytes != fa_info.size ) {
				// that's bad... but we'll ignore it for now.
				// dunno if we should raise an exception here or not...
				try {
					strstream s;
					s << "error reading attribute \"" << fa_ent->d_name \
					  << "\": read " << read_bytes << ", expected " \
					  << fa_info.size << ends;
					PyErr_SetString( PyExc_IOError, s.str() );
				} catch ( ... ) {
					PyErr_SetString( PyExc_IOError, "error reading attribute" );
				}

				// Need to clean up before we do this...
				(void)fs_close_attr_dir( fa_dir );
				close( fd );

				return NULL;
			}

			// Swap the data around for fun and profit.
			if( flags & ATTR_BIG_ENDIAN ) {
				(void)swap_data( fa_info.type, ptr, fa_info.size,
								 B_SWAP_BENDIAN_TO_HOST );
			} else if( flags & ATTR_LITTLE_ENDIAN ) {
				(void)swap_data( fa_info.type, ptr, fa_info.size,
								 B_SWAP_LENDIAN_TO_HOST );
			}
			
			// Now build a Python object out of the attribute, stick it in
			// a tuple, and add it to the dictionary.
			PyObject *attr = NULL;

			switch( fa_info.type ) {
			case B_ASCII_TYPE:
			case B_CHAR_TYPE:
			case B_MIME_TYPE:
			case B_RAW_TYPE:
			case B_STRING_TYPE:
			case B_MIME_STRING_TYPE:	// in storage/Mime.h... *grumble*
				// convert to string
				if( ptr[read_bytes - 1] == '\0' && read_bytes > 1 ) {
					attr = PyString_FromString( ptr );
				} else {
					attr = PyString_FromStringAndSize( ptr, read_bytes );
				}

				if( attr == NULL ) {
					try {
						strstream s;
						s << "error converting attribute \"" << fa_ent->d_name \
						  << "\" to string" << ends;
						PyErr_SetString( PyExc_RuntimeError, s.str() );
					} catch ( ... ) {
						PyErr_SetString( PyExc_RuntimeError, "error converting attribute to string" );
					}
				}
				break;
				
			case B_BOOL_TYPE:
			case B_INT8_TYPE:
			case B_UINT8_TYPE:
			case B_INT16_TYPE:
			case B_UINT16_TYPE:
			case B_INT32_TYPE:
			case B_SIZE_T_TYPE:
			case B_SSIZE_T_TYPE:
			case B_UINT32_TYPE:
			case B_INT64_TYPE:
			case B_OFF_T_TYPE:
			case B_TIME_TYPE:
			case B_UINT64_TYPE:
				// convert to integer
				if( read_bytes == sizeof( char ) ) {				// 8-bit
					attr = PyInt_FromLong( (long)( ptr[0] ) );
				} else if( read_bytes == sizeof( short ) ) {		// 16-bit
					short x;
					memcpy( &x, ptr, sizeof( short ) );
					attr = PyInt_FromLong( (long)x );
				} else if( read_bytes == sizeof( long ) ) {			// 32-bit
					long x;
					memcpy( &x, ptr, sizeof( long ) );
					attr = PyInt_FromLong( x );
				} else if( read_bytes == sizeof( long long ) ) {	// 64-bit
					long long x;
					memcpy( &x, ptr, sizeof( long long ) );
					attr = PyLong_FromLongLong( x );
				} else {											// two-bit
					attr = NULL;
				}
				
				if( attr == NULL ) {
					try {
						strstream s;
						s << "error converting attribute \"" << fa_ent->d_name \
						  << "\" to integer" << ends;
						PyErr_SetString( PyExc_RuntimeError, s.str() );
					} catch ( ... ) {
						PyErr_SetString( PyExc_RuntimeError, "error converting attribute to integer" );
					}
				}
				break;

			case B_DOUBLE_TYPE:
				// convert to float
				if( read_bytes == sizeof( double ) ) {
					double x;
					memcpy( &x, ptr, sizeof( double ) );
					attr = PyFloat_FromDouble( x );
				} else {
					attr = NULL;
				}

				if( attr == NULL ) {
					try {
						strstream s;
						s << "error converting attribute \"" << fa_ent->d_name \
						  << "\" to float" << ends;
						PyErr_SetString( PyExc_RuntimeError, s.str() );
					} catch ( ... ) {
						PyErr_SetString( PyExc_RuntimeError, "error converting attribute to float" );
					}
				}
				break;
				
			case B_FLOAT_TYPE:
				// convert to float
				if( read_bytes == sizeof( float ) ) {
					float x;
					memcpy( &x, ptr, sizeof( float ) );
					attr = PyFloat_FromDouble( (double)x );
				} else {
					attr = NULL;
				}

				if( attr == NULL ) {
					try {
						strstream s;
						s << "error converting attribute \"" << fa_ent->d_name \
						  << "\" to float" << ends;
						PyErr_SetString( PyExc_RuntimeError, s.str() );
					} catch ( ... ) {
						PyErr_SetString( PyExc_RuntimeError, "error converting attribute to float" );
					}
				}
				break;
				
			case B_POINT_TYPE:
				// BPoint -> (x,y)
				attr = PyTuple_New( 2 );
				if( attr ) {
					BPoint *pt = static_cast<BPoint *>( (void *)ptr );
					PyTuple_SET_ITEM( attr, 0, PyFloat_FromDouble( (double)pt->x ) );
					PyTuple_SET_ITEM( attr, 1, PyFloat_FromDouble( (double)pt->y ) );
				}
				
				if( attr == NULL ) {
					try {
						strstream s;
						s << "error converting attribute \"" << fa_ent->d_name \
						  << "\" to tuple" << ends;
						PyErr_SetString( PyExc_RuntimeError, s.str() );
					} catch ( ... ) {
						PyErr_SetString( PyExc_RuntimeError, "error converting attribute to tuple" );
					}
				}
				break;
				
			case B_RECT_TYPE:
				// BRect -> (top,left,bottom,right)
				attr = PyTuple_New( 4 );
				if( attr ) {
					BRect *rect = static_cast<BRect *>( (void *)ptr );
					PyTuple_SET_ITEM( attr, 0, PyFloat_FromDouble( (double)rect->left ) );
					PyTuple_SET_ITEM( attr, 1, PyFloat_FromDouble( (double)rect->top ) );
					PyTuple_SET_ITEM( attr, 2, PyFloat_FromDouble( (double)rect->right ) );
					PyTuple_SET_ITEM( attr, 3, PyFloat_FromDouble( (double)rect->bottom ) );
				}

				if( attr == NULL ) {
					try {
						strstream s;
						s << "error converting attribute \"" << fa_ent->d_name \
						  << "\" to tuple" << ends;
						PyErr_SetString( PyExc_RuntimeError, s.str() );
					} catch ( ... ) {
						PyErr_SetString( PyExc_RuntimeError, "error converting attribute to tuple" );
					}
				}
				break;
				
			case B_REF_TYPE:
				// entry_ref -> pathname
				{
					BEntry ent( static_cast<entry_ref *>( (void *)ptr ) );
					if( ent.InitCheck() != B_OK ) {
						try {
							strstream s;
							s << "error getting filesystem entry for attribute \"" \
							  << fa_ent->d_name \
							  << "\": " << strerror( ent.InitCheck() ) << ends;
							PyErr_SetString( PyExc_RuntimeError, s.str() );
						} catch ( ... ) {
							PyErr_SetString( PyExc_RuntimeError, "error getting filesystem entry for attribute" );
						}

						break;
					}
					
					BPath path;
					status_t path_retval = ent.GetPath( &path );
					if( path_retval != B_OK ) {
						try {
							strstream s;
							s << "error getting path of filesystem entry for attribute \"" \
							  << fa_ent->d_name \
							  << "\": " << strerror( path_retval ) << ends;
							PyErr_SetString( PyExc_RuntimeError, s.str() );
						} catch ( ... ) {
							PyErr_SetString( PyExc_RuntimeError, "error getting path of filesystem entry for attribute" );
						}

						break;
					}
					
					attr = PyString_FromString( path.Path() );
				}
				
				if( attr == NULL ) {
					try {
						strstream s;
						s << "error converting entry_ref attribute \"" \
						  << fa_ent->d_name \
						  << "\" to string" << ends;
						PyErr_SetString( PyExc_RuntimeError, s.str() );
					} catch ( ... ) {
						PyErr_SetString( PyExc_RuntimeError, "error converting entry_ref attribute to string" );
					}
				}
				break;
				
			case B_RGB_COLOR_TYPE:
				// rgb_color -> (r,g,b,a)
				attr = PyTuple_New( 4 );
				if( attr ) {
					rgb_color *rgb = static_cast<rgb_color *>( (void *)ptr );
					PyTuple_SET_ITEM( attr, 0, PyInt_FromLong( (long)rgb->red ) );
					PyTuple_SET_ITEM( attr, 1, PyInt_FromLong( (long)rgb->green ) );
					PyTuple_SET_ITEM( attr, 2, PyInt_FromLong( (long)rgb->blue ) );
					PyTuple_SET_ITEM( attr, 3, PyInt_FromLong( (long)rgb->alpha ) );
				}
				
				if( attr == NULL ) {
					try {
						strstream s;
						s << "error converting attribute \"" << fa_ent->d_name \
						  << "\" to tuple" << ends;
						PyErr_SetString( PyExc_RuntimeError, s.str() );
					} catch ( ... ) {
						PyErr_SetString( PyExc_RuntimeError, "error converting attribute to tuple" );
					}
				}
				break;

			default:
				// unknown data
				attr = PyString_FromStringAndSize( ptr, read_bytes );
				
				if( attr == NULL ) {
					try {
						strstream s;
						s << "error converting attribute \"" << fa_ent->d_name \
						  << "\" to string" << ends;
						PyErr_SetString( PyExc_RuntimeError, s.str() );
					} catch ( ... ) {
						PyErr_SetString( PyExc_RuntimeError, "error converting attribute to string" );
					}
				}
				break;
			}

			// We're done with this, so discard it.
			free( ptr );

			// If we've got a valid attribute, let's add it to the dictionary.
			if( attr ) {
				PyObject *the_tuple = PyTuple_New( 2 );
				PyObject *the_name = PyString_FromString( fa_ent->d_name );
				PyObject *the_type = PyInt_FromLong( fa_info.type );

				if( the_tuple == NULL || the_name == NULL || the_type == NULL ) {
					try {
						strstream s;
						s << "error creating attribute tuple for \"" \
						  << fa_ent->d_name << "\"" << ends;
						PyErr_SetString( PyExc_RuntimeError, s.str() );
					} catch ( ... ) {
						PyErr_SetString( PyExc_RuntimeError, "error creating attribute tuple" );
					}
					
					// need to clean up
					return NULL;
				}
				
				PyTuple_SET_ITEM( the_tuple, 0, the_type );
				PyTuple_SET_ITEM( the_tuple, 1, attr );
				
				if( PyDict_SetItem( attributes, the_name, the_tuple ) == -1 ) {
					// error handling... 
					// leak-o-rama
					try {
						strstream s;
						s << "can't add attribute \"" << fa_ent->d_name \
						  << "\" to list" << ends;
						PyErr_SetString( PyExc_RuntimeError, s.str() );
					} catch ( ... ) {
						PyErr_SetString( PyExc_RuntimeError, "can't add attributes to list" );
					}
					return NULL;
				}
				
				attributes_added++;
			} else {
				// Some sort of cleanup...
				return NULL;
			}
		}

		// Get the next attribute's info.
		fa_ent = fs_read_attr_dir( fa_dir );
	}

	(void)fs_close_attr_dir( fa_dir );
	close( fd );

	Py_INCREF( attributes );
	return attributes;
}

// ----------------------------------------------------------------------
// Write a file attribute to the file/directory/symlink; if the data is a
// few things (like an rgb_color, BRect, etc.) it must be presented as a tuple.
// 
// args:
//  filename
//  attr_name
//  attr_type (as a string or integer)
//  attr_data (as an object; we'll figure out what it is)
//  flags = 0 (optional)

static PyObject *bfs_write_attr( PyObject *self, PyObject *args )
{
	// self isn't used for normal functions
	self = self;

	char *filename;
	char *attr_name;
	PyObject *attr_type_obj;
	PyObject *attr_data_obj;
	int flags = 0;

	// Could be B_READ_ONLY in BeOS > R4.5.
	int mode = B_WRITE_ONLY;

	if( PyArg_ParseTuple( args, "ssOO|i",
						  &filename,
						  &attr_name, &attr_type_obj, &attr_data_obj,
						  &flags ) ) {
		if( flags & ATTR_SYMLINK ) mode |= O_NOTRAVERSE;
		if( ( flags & ATTR_BIG_ENDIAN ) && ( flags & ATTR_LITTLE_ENDIAN ) ) {
			PyErr_SetString( PyExc_ValueError, 
							 "can't specify ATTR_BIG_ENDIAN and ATTR_LITTLE_ENDIAN, it's ust not right" );
			return NULL;
		}
	} else {
		PyErr_SetString( PyExc_TypeError, "invalid arguments" );
		return NULL;
	}

	// Is the attribute name too long?  Hope you don't have embedded NULs...
	if( strlen( attr_name ) > B_ATTR_NAME_LENGTH ) {
		PyErr_SetString( PyExc_OverflowError, "attribute name too long" );
		return NULL;
	}

	uint32 be_type_code = 0;
	
	if( PyInt_Check( attr_type_obj ) ) {			// integer version of B_*_TYPE
		be_type_code = (uint32)PyInt_AsLong( attr_type_obj );
	} else if( PyString_Check( attr_type_obj ) ) {	// string version
		if( PyString_Size( attr_type_obj ) != 4 ) {
			PyErr_SetString( PyExc_TypeError, "attribute type must be 4 characters" );
			return NULL;
		}

		char *type_str = PyString_AsString( attr_type_obj );
		be_type_code += (uint32)type_str[0] << 24;	// endian-safe? hmm...
		be_type_code += (uint32)type_str[1] << 16;	// brain isn't working...
		be_type_code += (uint32)type_str[2] << 8;
		be_type_code += (uint32)type_str[3];
	} else {										// error version
		PyErr_SetString( PyExc_TypeError, "attribute type must be specified as an integer or a string" );
		return NULL;
	}

	char *buffer = NULL;
	size_t buffer_size = 0;
	bool own_buffer = false;

	switch( be_type_code ) {
	case B_ASCII_TYPE:
	case B_CHAR_TYPE:
	case B_MIME_TYPE:
	case B_RAW_TYPE:
	case B_STRING_TYPE:
	case B_MIME_STRING_TYPE:
		// convert from string
		{
			char *str = PyString_AsString( attr_data_obj );
			if( NULL == str ) {
				PyErr_SetString( PyExc_TypeError, 
								 "couldn't convert data to string" );
			} else {
				buffer = str;
				switch( be_type_code ) {
				case B_CHAR_TYPE:
					// In case some smart-ass tries to make huge chars...
					buffer_size = 1;
					break;

				case B_STRING_TYPE:
					// In case some smart-ass tries to embed NULs...
					buffer_size = strlen( buffer );
					break;

				default:
					buffer_size = PyString_Size( attr_data_obj );
					break;
				}
			}
		}
		break;
				
	case B_BOOL_TYPE:
		// convert from whatever
		{
			bool val = ( PyObject_IsTrue( attr_data_obj ) ? true : false );
			buffer_size = sizeof( bool );
			buffer = (char *)malloc( buffer_size );
			if( buffer ) {
				own_buffer = true;
				memcpy( buffer, &val, buffer_size );
			}
		}
		break;

	case B_INT8_TYPE:
		// 8-bit value
		{
			long val = PyInt_AsLong( attr_data_obj );
			if( val < SCHAR_MIN || val > SCHAR_MAX ) {
				PyErr_SetString( PyExc_OverflowError, 
								 "value bigger than 8 bits" );
			} else {
				buffer_size = sizeof( int8 );
				buffer = (char *)malloc( buffer_size );
				if( buffer ) {
					own_buffer = true;
					int8 val8 = (int8)val;
					memcpy( buffer, &val8, buffer_size );
				}
			}
		}
		break;

	case B_UINT8_TYPE:
		// 8-bit value
		{
			unsigned long val = (unsigned long)PyInt_AsLong( attr_data_obj );
			if( val > UCHAR_MAX ) {
				PyErr_SetString( PyExc_OverflowError, 
								 "value bigger than 8 bits" );
			} else {
				buffer_size = sizeof( uint8 );
				buffer = (char *)malloc( buffer_size );
				if( buffer ) {
					own_buffer = true;
					uint8 val8 = (uint8)val;
					memcpy( buffer, &val8, buffer_size );
				}
			}
		}
		break;

	case B_INT16_TYPE:
		// 16-bit value
		{
			long val = PyInt_AsLong( attr_data_obj );
			if( val < SHRT_MIN || val > SHRT_MAX ) {
				PyErr_SetString( PyExc_OverflowError, 
								 "value bigger than 16 bits" );
			} else {
				buffer_size = sizeof( int16 );
				buffer = (char *)malloc( buffer_size );
				if( buffer ) {
					own_buffer = true;
					int16 val16 = (int16)val;
					memcpy( buffer, &val16, buffer_size );
				}
			}
		}
		break;

	case B_UINT16_TYPE:
		// 16-bit value
		{
			unsigned long val = (unsigned long)PyInt_AsLong( attr_data_obj );
			if( val > USHRT_MAX ) {
				PyErr_SetString( PyExc_OverflowError, 
								 "value bigger than 16 bits" );
			} else {
				buffer_size = sizeof( uint16 );
				buffer = (char *)malloc( buffer_size );
				if( buffer ) {
					own_buffer = true;
					uint16 val16 = (uint16)val;
					memcpy( buffer, &val16, buffer_size );
				}
			}
		}
		break;

	case B_INT32_TYPE:
	case B_SIZE_T_TYPE:
	case B_SSIZE_T_TYPE:
	case B_UINT32_TYPE:
	case B_TIME_TYPE:	// see bug #19990610-11859; am I 32-bit or 64-bit?
		// 32-bit values
		{
			long val = PyInt_AsLong( attr_data_obj );
			buffer_size = sizeof( int32 );
			buffer = (char *)malloc( buffer_size );
			if( buffer ) {
				own_buffer = true;
				memcpy( buffer, &val, buffer_size );
			}
		}
		break;

	case B_INT64_TYPE:
	case B_OFF_T_TYPE:
	case B_UINT64_TYPE:
		// 64-bit values
		{
			long long val = PyLong_AsLongLong( attr_data_obj );
			buffer_size = sizeof( int64 );
			buffer = (char *)malloc( buffer_size );
			if( buffer ) {
				own_buffer = true;
				memcpy( buffer, &val, buffer_size );
			}
		}
		break;

	case B_DOUBLE_TYPE:
		// convert from double
		{
			double val = PyFloat_AsDouble( attr_data_obj );
			buffer_size = sizeof( double );
			buffer = (char *)malloc( buffer_size );
			if( buffer ) {
				own_buffer = true;
				memcpy( buffer, &val, buffer_size );
			}
		}
		break;

	case B_FLOAT_TYPE:
		// convert from float
		{
			double val = PyFloat_AsDouble( attr_data_obj );
			if( val < (double)FLT_MIN || val > (double)FLT_MAX ) {
				PyErr_SetString( PyExc_OverflowError, 
								 "value bigger than 16 bits" );
			} else {
				float fval = (float)val;
				buffer_size = sizeof( float );
				buffer = (char *)malloc( buffer_size );
				if( buffer ) {
					own_buffer = true;
					memcpy( buffer, &fval, buffer_size );
				}
			}
		}
		break;
			
	case B_POINT_TYPE:
		// BPoint -> (x,y)
		if( !PyTuple_Check( attr_data_obj ) ) {
			PyErr_SetString( PyExc_TypeError, "BPoints are passed as tuples" );
		} else {
			PyObject *obj = PyTuple_GetItem( attr_data_obj, 0 );
			if( NULL == obj ) {
				PyErr_SetString( PyExc_IndexError, "can't get x from tuple" );
				break;
			}
			float x = (float)PyFloat_AsDouble( obj );

			obj = PyTuple_GetItem( attr_data_obj, 1 );
			if( NULL == obj ) {
				PyErr_SetString( PyExc_IndexError, "can't get y from tuple" );
				break;
			}
			float y = (float)PyFloat_AsDouble( obj );
			
			BPoint val( x, y );
			buffer_size = sizeof( BPoint );
			buffer = (char *)malloc( buffer_size );
			if( buffer ) {
				own_buffer = true;
				memcpy( buffer, &val, buffer_size );
			}
		}
		break;
			
	case B_RECT_TYPE:
		// BRect -> (left,top,right,bottom)
		if( !PyTuple_Check( attr_data_obj ) ) {
			PyErr_SetString( PyExc_TypeError, "BRects are passed as tuples" );
		} else {
			PyObject *obj = PyTuple_GetItem( attr_data_obj, 0 );
			if( NULL == obj ) {
				PyErr_SetString( PyExc_IndexError, "can't get left from tuple" );
				break;
			}
			float left = (float)PyFloat_AsDouble( obj );

			obj = PyTuple_GetItem( attr_data_obj, 1 );
			if( NULL == obj ) {
				PyErr_SetString( PyExc_IndexError, "can't get top from tuple" );
				break;
			}
			float top = (float)PyFloat_AsDouble( obj );
			
			obj = PyTuple_GetItem( attr_data_obj, 2 );
			if( NULL == obj ) {
				PyErr_SetString( PyExc_IndexError, "can't get right from tuple" );
				break;
			}
			float right = (float)PyFloat_AsDouble( obj );

			obj = PyTuple_GetItem( attr_data_obj, 3 );
			if( NULL == obj ) {
				PyErr_SetString( PyExc_IndexError, "can't get bottom from tuple" );
				break;
			}
			float bottom = (float)PyFloat_AsDouble( obj );
			
			BRect val( left, top, right, bottom );
			buffer_size = sizeof( BRect );
			buffer = (char *)malloc( buffer_size );
			if( buffer ) {
				own_buffer = true;
				memcpy( buffer, &val, buffer_size );
			}
		}
		break;
				
	case B_REF_TYPE:
		// entry_ref -> pathname
		PyErr_SetString( PyExc_NotImplementedError, "where the hell did you get an entry_ref from anyway?" );
		break;
			
	case B_RGB_COLOR_TYPE:
		// rgb_color -> (r,g,b,a)
		if( !PyTuple_Check( attr_data_obj ) ) {
			PyErr_SetString( PyExc_TypeError, "rgb_colors are passed as tuples" );
		} else {
			PyObject *obj = PyTuple_GetItem( attr_data_obj, 0 );
			if( NULL == obj ) {
				PyErr_SetString( PyExc_IndexError, "can't get red from tuple" );
				break;
			}
			long val = PyInt_AsLong( obj );
			if( val > UCHAR_MAX ) {
				PyErr_SetString( PyExc_OverflowError, 
								 "red value value greater than 255" );
				break;
			}

			uint8 red = (uint8)val;

			obj = PyTuple_GetItem( attr_data_obj, 1 );
			if( NULL == obj ) {
				PyErr_SetString( PyExc_IndexError, "can't get green from tuple" );
				break;
			}
			val = PyInt_AsLong( obj );
			if( val > UCHAR_MAX ) {
				PyErr_SetString( PyExc_OverflowError, 
								 "green value value greater than 255" );
				break;
			}

			uint8 green = (uint8)val;

			obj = PyTuple_GetItem( attr_data_obj, 1 );
			if( NULL == obj ) {
				PyErr_SetString( PyExc_IndexError, "can't get blue from tuple" );
				break;
			}
			val = PyInt_AsLong( obj );
			if( val > UCHAR_MAX ) {
				PyErr_SetString( PyExc_OverflowError, 
								 "blue value value greater than 255" );
				break;
			}

			uint8 blue = (uint8)val;

			obj = PyTuple_GetItem( attr_data_obj, 1 );
			if( NULL != obj ) {
				val = PyInt_AsLong( obj );
			} else {
				val = 255;
			}
			if( val > UCHAR_MAX ) {
				PyErr_SetString( PyExc_OverflowError, 
								 "alpha value value greater than 255" );
				break;
			}

			uint8 alpha = (uint8)val;

			rgb_color color = { red, green, blue, alpha };
			buffer_size = sizeof( rgb_color );
			buffer = (char *)malloc( buffer_size );
			if( buffer ) {
				own_buffer = true;
				memcpy( buffer, &color, buffer_size );
			}
		}
		break;

	default:
		// unknown data
		{
			char *str = PyString_AsString( attr_data_obj );
			if( NULL == str ) {
				PyErr_SetString( PyExc_TypeError, 
								 "couldn't convert data" );
			} else {
				buffer = str;
				buffer_size = PyString_Size( attr_data_obj );
			}
		}
		break;
	}

	if( NULL == buffer ) {
		return NULL;
	}

	// Swap the data around for fun and profit.
	if( flags & ATTR_BIG_ENDIAN ) {
		(void)swap_data( be_type_code, buffer, buffer_size,
						 B_SWAP_HOST_TO_BENDIAN );
	} else if( flags & ATTR_LITTLE_ENDIAN ) {
		(void)swap_data( be_type_code, buffer, buffer_size,
						 B_SWAP_HOST_TO_LENDIAN );
	}
			
	// fs_remove_attr() before trying to write it?
	int fd = open( filename, mode );
	if( fd < 0 ) {
		try {
			strstream s;
			s << "can't open file: " << filename \
			  << " (" << strerror( errno ) << ")" << ends;
			PyErr_SetString( PyExc_IOError, s.str() );
		} catch ( ... ) {
			PyErr_SetString( PyExc_IOError, strerror( errno ) );
		}
			return NULL;
	}

	ssize_t wrote = fs_write_attr( fd, attr_name, be_type_code, 0,
								   buffer, buffer_size );
	close( fd );

	if( wrote != (ssize_t)buffer_size ) {
		try {
			strstream s;
			s << "error writing attribute: " << attr_name \
			  << " (" << strerror( errno ) << ")" << ends;
			PyErr_SetString( PyExc_IOError, s.str() );
		} catch ( ... ) {
			PyErr_SetString( PyExc_IOError, strerror( errno ) );
		}

		return NULL;
	}

	if( own_buffer && buffer ) free( buffer );

	Py_INCREF( Py_None );
	return Py_None;
}

// ----------------------------------------------------------------------
// Remove an attribute from the file/directory/symlink.
//
// args:
//	filename
//	attr_name
//	flags = 0 (optional)

static PyObject *bfs_remove_attr( PyObject *self, PyObject *args )
{
	// self isn't used for normal functions
	self = self;

	char *filename;
	char *attr_name;
	int flags = 0;

	int mode = O_WRONLY;
	
	if( PyArg_ParseTuple( args, "ss|i",
						  &filename, 
						  &attr_name, 
						  &flags ) ) {
		if( flags & ATTR_SYMLINK ) mode |= O_NOTRAVERSE;
		if( ( flags & ATTR_BIG_ENDIAN ) && ( flags & ATTR_LITTLE_ENDIAN ) ) {
			PyErr_SetString( PyExc_ValueError, 
							 "can't specify ATTR_BIG_ENDIAN and ATTR_LITTLE_ENDIAN, it's ust not right" );
			return NULL;
		}
	} else {
		PyErr_SetString( PyExc_TypeError, "invalid arguments" );
		return NULL;
	}

	int fd = open( filename, mode );
	if( fd < 0 ) {
		try {
			strstream s;
			s << "can't open file: " << filename \
			  << " (" << strerror( errno ) << ")" << ends;
			PyErr_SetString( PyExc_IOError, s.str() );
		} catch ( ... ) {
			PyErr_SetString( PyExc_IOError, strerror( errno ) );
		}

		return NULL;
	}

	int retval = fs_remove_attr( fd, attr_name );
	if( retval != B_OK ) {
		try {
			strstream s;
			s << "can't remove attribute: " << attr_name \
			  << " (" << strerror( errno ) << ")" << ends;
			PyErr_SetString( PyExc_IOError, s.str() );
		} catch ( ... ) {
			PyErr_SetString( PyExc_IOError, strerror( errno ) );
		}

		close( fd );
		return NULL;
	}
	
	close( fd );

	Py_INCREF( Py_None );
	return Py_None;
}

// ----------------------------------------------------------------------
// List of functions defined in the module
static PyMethodDef fsattr_methods[] = {
	{
		"read_attrs",
		bfs_read_attrs,
		METH_VARARGS,
		"read_attrs( filename, flags = 0 )\n" \
		"\n" \
		"Reads the attributes for filename; returns a dictionary of tuples,\n" \
		"each tuple is ( type, data ) and the key is the attribute name.\n"\
		"\n" \
		"If flags is attr.SYMLINK, symbolic links WILL NOT be traversed;\n" \
		"you'll get the attribute data for the symlink, not the target.\n" \
		"If flags has attr.BIG_ENDIAN set, the data will be read from big-endian\n" \
		"format; if flags has attr.LITTLE_ENDIAN set, the data will be read from\n" \
		"little-endian format.  If both attr.BIG_ENDIAN and attr.LITTLE_ENDIAN are\n" \
		"set, a ValueError exception is raised." \
	},
	{
		"write_attr",
		bfs_write_attr,
		METH_VARARGS,
		"write_attr( filename, attr_name, attr_type, attr_data, flags = 0 )\n" \
		"\n" \
		"Write an attribute to filename.\n" \
		"\n" \
		"If flags has attr.SYMLINK set, symbolic links will NOT be traversed;\n" \
		"you'll set the attribute data on the symlink, not the target.\n" \
		"If flags has attr.BIG_ENDIAN set, the data will be written in big-endian\n" \
		"format; if flags has attr.LITTLE_ENDIAN set, the data will be written in\n" \
		"little-endian format.  If both attr.BIG_ENDIAN and attr.LITTLE_ENDIAN are\n" \
		"set, a ValueError exception is raised.\n" \
		"\n" \
		"attr_type can be a four-character string, or a number; you'll get\n" \
		"a TypeError exception if the data doesn't match the type in a\n" \
		"reasonable manner."
	},
	{
		"remove_attr",
		bfs_remove_attr,
		METH_VARARGS,
		"remove_attr( filename, attr_name, flags = 0 )\n" \
		"\n" \
		"Remove the specified attribute from filename.\n" \
		"\n" \
		"If flags is attr.SYMLINK, symbolic links WILL NOT be traversed;\n" \
		"you'll remove the attribute data for the symlink, not the target."
	},
	{ // sentinel
		NULL,	// name
		NULL,	// function
		0,		// flags
		""		// docstring
	}
};

// ----------------------------------------------------------------------
// Initialization function for the fsattr module
extern "C" DL_EXPORT(PyObject *) init_fsattr( void )
{
	// Create the module and add the functions
	PyObject *mod = Py_InitModule4( "_fsattr", fsattr_methods, 
									"BeFS file attribute functions:\n" \
									"\n" \
									"read_attrs - read the attributes for a file/directory/symlink\n" \
									"write_attr - write an attribute to a file/directory/symlink" \
									"remove_attr - remove an attribute for a file/directory/symlink\n",
									static_cast<PyObject *>( NULL ),
									PYTHON_API_VERSION );

	// Add some symbolic constants to the module
	PyObject *mdict = PyModule_GetDict( mod );
	PyObject *dict = PyDict_New();
	PyObject *attr_dict = PyDict_New();

	PyDict_SetItemString(mdict, "types", dict);
	PyDict_SetItemString(mdict, "attr", attr_dict);

	PyDict_SetItemString( attr_dict, "SYMLINK", PyInt_FromLong( ATTR_SYMLINK ) );
	PyDict_SetItemString( attr_dict, "BIG_ENDIAN", PyInt_FromLong( ATTR_BIG_ENDIAN ) );
	PyDict_SetItemString( attr_dict, "LITTLE_ENDIAN", PyInt_FromLong( ATTR_LITTLE_ENDIAN ) );

	// Why look, a whole bunch of untested object constructors...
	PyDict_SetItemString( dict, "B_AFFINE_TRANSFORM_TYPE", PyInt_FromLong( B_AFFINE_TRANSFORM_TYPE ) );
	PyDict_SetItemString( dict, "B_ALIGNMENT_TYPE", PyInt_FromLong( B_ALIGNMENT_TYPE ) );
	PyDict_SetItemString( dict, "B_ANY_TYPE", PyInt_FromLong( B_ANY_TYPE ) );
	PyDict_SetItemString( dict, "B_ATOM_TYPE", PyInt_FromLong( B_ATOM_TYPE ) );
	PyDict_SetItemString( dict, "B_ATOMREF_TYPE", PyInt_FromLong( B_ATOMREF_TYPE ) );
	PyDict_SetItemString( dict, "B_BOOL_TYPE", PyInt_FromLong( B_BOOL_TYPE ) );
	PyDict_SetItemString( dict, "B_CHAR_TYPE", PyInt_FromLong( B_CHAR_TYPE ) );
	PyDict_SetItemString( dict, "B_COLOR_8_BIT_TYPE", PyInt_FromLong( B_COLOR_8_BIT_TYPE ) );
	PyDict_SetItemString( dict, "B_DOUBLE_TYPE", PyInt_FromLong( B_DOUBLE_TYPE ) );
	PyDict_SetItemString( dict, "B_FLOAT_TYPE", PyInt_FromLong( B_FLOAT_TYPE ) );
	PyDict_SetItemString( dict, "B_GRAYSCALE_8_BIT_TYPE", PyInt_FromLong( B_GRAYSCALE_8_BIT_TYPE ) );
	PyDict_SetItemString( dict, "B_INT16_TYPE", PyInt_FromLong( B_INT16_TYPE ) );
	PyDict_SetItemString( dict, "B_INT32_TYPE", PyInt_FromLong( B_INT32_TYPE ) );
	PyDict_SetItemString( dict, "B_INT64_TYPE", PyInt_FromLong( B_INT64_TYPE ) );
	PyDict_SetItemString( dict, "B_INT8_TYPE", PyInt_FromLong( B_INT8_TYPE ) );
	PyDict_SetItemString( dict, "B_LARGE_ICON_TYPE", PyInt_FromLong( B_LARGE_ICON_TYPE ) );
	PyDict_SetItemString( dict, "B_MEDIA_PARAMETER_GROUP_TYPE", PyInt_FromLong( B_MEDIA_PARAMETER_GROUP_TYPE ) );
	PyDict_SetItemString( dict, "B_MEDIA_PARAMETER_TYPE", PyInt_FromLong( B_MEDIA_PARAMETER_TYPE ) );
	PyDict_SetItemString( dict, "B_MEDIA_PARAMETER_WEB_TYPE", PyInt_FromLong( B_MEDIA_PARAMETER_WEB_TYPE ) );
	PyDict_SetItemString( dict, "B_MESSAGE_TYPE", PyInt_FromLong( B_MESSAGE_TYPE ) );
	PyDict_SetItemString( dict, "B_MESSENGER_TYPE", PyInt_FromLong( B_MESSENGER_TYPE ) );
	PyDict_SetItemString( dict, "B_MIME_STRING_TYPE", PyInt_FromLong( B_MIME_STRING_TYPE ) );
	PyDict_SetItemString( dict, "B_MIME_TYPE", PyInt_FromLong( B_MIME_TYPE ) );
	PyDict_SetItemString( dict, "B_MINI_ICON_TYPE", PyInt_FromLong( B_MINI_ICON_TYPE ) );
	PyDict_SetItemString( dict, "B_MONOCHROME_1_BIT_TYPE", PyInt_FromLong( B_MONOCHROME_1_BIT_TYPE ) );
	PyDict_SetItemString( dict, "B_NETWORK_ADDRESS_TYPE", PyInt_FromLong( B_NETWORK_ADDRESS_TYPE ) );
	PyDict_SetItemString( dict, "B_OBJECT_TYPE", PyInt_FromLong( B_OBJECT_TYPE ) );
	PyDict_SetItemString( dict, "B_OFF_T_TYPE", PyInt_FromLong( B_OFF_T_TYPE ) );
	PyDict_SetItemString( dict, "B_PATTERN_TYPE", PyInt_FromLong( B_PATTERN_TYPE ) );
	PyDict_SetItemString( dict, "B_POINT_TYPE", PyInt_FromLong( B_POINT_TYPE ) );
	PyDict_SetItemString( dict, "B_POINTER_TYPE", PyInt_FromLong( B_POINTER_TYPE ) );
	PyDict_SetItemString( dict, "B_PROPERTY_INFO_TYPE", PyInt_FromLong( B_PROPERTY_INFO_TYPE ) );
	PyDict_SetItemString( dict, "B_RAW_TYPE", PyInt_FromLong( B_RAW_TYPE ) );
	PyDict_SetItemString( dict, "B_RECT_TYPE", PyInt_FromLong( B_RECT_TYPE ) );
	PyDict_SetItemString( dict, "B_REF_TYPE", PyInt_FromLong( B_REF_TYPE ) );
	PyDict_SetItemString( dict, "B_RGB_32_BIT_TYPE", PyInt_FromLong( B_RGB_32_BIT_TYPE ) );
	PyDict_SetItemString( dict, "B_RGB_COLOR_TYPE", PyInt_FromLong( B_RGB_COLOR_TYPE ) );
	PyDict_SetItemString( dict, "B_SIZE_T_TYPE", PyInt_FromLong( B_SIZE_T_TYPE ) );
	PyDict_SetItemString( dict, "B_SIZE_TYPE", PyInt_FromLong( B_SIZE_TYPE ) );
	PyDict_SetItemString( dict, "B_SSIZE_T_TYPE", PyInt_FromLong( B_SSIZE_T_TYPE ) );
	PyDict_SetItemString( dict, "B_STRING_LIST_TYPE", PyInt_FromLong( B_STRING_LIST_TYPE ) );
	PyDict_SetItemString( dict, "B_STRING_TYPE", PyInt_FromLong( B_STRING_TYPE ) );
	PyDict_SetItemString( dict, "B_TIME_TYPE", PyInt_FromLong( B_TIME_TYPE ) );
	PyDict_SetItemString( dict, "B_UINT16_TYPE", PyInt_FromLong( B_UINT16_TYPE ) );
	PyDict_SetItemString( dict, "B_UINT32_TYPE", PyInt_FromLong( B_UINT32_TYPE ) );
	PyDict_SetItemString( dict, "B_UINT64_TYPE", PyInt_FromLong( B_UINT64_TYPE ) );
	PyDict_SetItemString( dict, "B_UINT8_TYPE", PyInt_FromLong( B_UINT8_TYPE ) );
	PyDict_SetItemString( dict, "B_VECTOR_ICON_TYPE", PyInt_FromLong( B_VECTOR_ICON_TYPE ) );
	PyDict_SetItemString( dict, "B_XATTR_TYPE", PyInt_FromLong( B_XATTR_TYPE ) );

	return mod;
}
