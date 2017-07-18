__doc__ = """\
BeOS Module for Python

Copyright (C) 2005 by Mikael Jansson <apps@mikael.jansson.be>"""

class Enum(dict):
	def __init__(self, other={}):
		dict.__init__(self, other)
	__getattr__ = dict.__getitem__
