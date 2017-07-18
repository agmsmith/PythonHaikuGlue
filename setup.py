#!/bin/env python

import sys
libs = ["be"]
if sys.platform == "zeta":
	libs += ["zeta", "stdc++.r4"]

from distutils.core import setup, Extension

modules = [
	Extension('beos.storage._find_directory',
		['ext/storage/_find_directory.cpp'],
		# XXX: Ugly hackery, this should really be added to distutils...
		extra_link_args=['-nostart', '-Wl,-soname=_find_directory.so'],
		libraries=libs),
	Extension('beos.storage._fsquery',
		['ext/storage/_fsquery.cpp'],
		# XXX: Ugly hackery, this should really be added to distutils...
		extra_link_args=['-nostart', '-Wl,-soname=_fsquery.so'],
		libraries=libs),
	Extension('beos.storage._fsattr',
		['ext/storage/_fsattr.cpp'],
		# XXX: Ugly hackery, this should really be added to distutils...
		extra_link_args=['-nostart', '-Wl,-soname=_fsattr.so'],
		libraries=libs)]


setup(name='BeOS',
	  version='0.1.0',
	  description='BeOS Modulels',
	  author='Mikael Jansson',
	  author_email='apps@mikael.jansson.be',
	  url='http://mikael.jansson.be',
	  packages=['beos',
	  			'beos.storage'],
	  package_dir={'beos': 'src'},
	  ext_modules=modules)
