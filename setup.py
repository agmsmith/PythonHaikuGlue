#!/bin/python

import sys
libs = ["be"]
if sys.platform == "zeta":
	libs += ["zeta", "stdc++.r4"]

from distutils.core import setup, Extension

modules_list = [
	Extension('haikuglue.storage._find_directory',
		['ext/storage/_find_directory.cpp'],
		extra_compile_args=['-Wno-multichar'],
		extra_link_args=['-nostart', '-Wl,-soname=_find_directory.so'],
		libraries=libs),
	Extension('haikuglue.storage._fsquery',
		['ext/storage/_fsquery.cpp'],
		extra_link_args=['-nostart', '-Wl,-soname=_fsquery.so'],
		libraries=libs),
	Extension('haikuglue.storage._fsattr',
		['ext/storage/_fsattr.cpp'],
		extra_compile_args=['-Wno-multichar'],
		extra_link_args=['-nostart', '-Wl,-soname=_fsattr.so'],
		libraries=libs)]


setup(name='HaikuGlue',
	  version='0.2',
	  description='Haiku OS API Glue Module',
	  author='Mikael Jansson',
	  author_email='apps@mikael.jansson.be',
	  maintainer='Alexander G. M. Smith',
	  maintainer_email='agmsmith@ncf.ca',
	  url='https://github.com/agmsmith/PythonHaikuGlue',
	  packages=['haikuglue',
	  			'haikuglue.storage'],
	  package_dir={'haikuglue': 'src'},
	  ext_modules=modules_list)
