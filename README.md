# PythonHaikuGlue
Glue functions for the Python language that let you call Haiku OS C++ functions.

Initially the glue is just for some file system APIs, mostly the attribute access ones.

It's an update of BeOSmodule by Mikael Jansson and Chris Herborth, last touched in 2005.  In 2017 Chris Herborth gave permission to release it under the MIT license.

## Installing

After getting the source code, run this Python command to compile and install it to the Haiku non-packaged directory:

python setup.py install

To uninstall, do a desktop search for a "haikuglue" directory and remove it from likely places (usually "site-packages" is somewhere in the path).  Supposedly the command "pip uninstall haikuglue" would also work, but it doesn't.
