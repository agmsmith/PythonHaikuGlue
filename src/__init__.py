__doc__ = """Haiku OS Glue Module for Python, version 0.2

This top level module doesn't do much, see the storage submodule for more.

Copyright (C) 2005 by Mikael Jansson <apps@mikael.jansson.be>"""

class Enum(dict):
	"""An internal use function for turning a dictionary into an Enum."""
	def __init__(self, other={}):
		dict.__init__(self, other)
	__getattr__ = dict.__getitem__
