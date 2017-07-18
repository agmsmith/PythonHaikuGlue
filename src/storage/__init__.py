"""beos.storage package."""

from beos import Enum

import _find_directory
import _fsattr
import _fsquery

# constants
directory_which = Enum(_find_directory.directory_which)
types = Enum(_fsattr.types)
attr = Enum(_fsattr.attr)

# functions
find_directory = _find_directory.find_directory
query = _fsquery.query
read_attrs = _fsattr.read_attrs
write_attr = _fsattr.write_attr
remove_attr = _fsattr.remove_attr
