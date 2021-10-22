from SCons.Script import *

def FindSiSo(env):
	# SiSo: Silicon Software
	# We make an image source and camera manager for using our JAI cameras
	# through Silicon Software framegrabbers, as well as a timing board.
	env.Append(CPPDEFINES=["USE_SISO"])
	env.Append(CPPPATH=["/opt/frameGrabbers/SiSo-Runtime-5.7.0/include/"] )
	env.Append(LIBPATH=["/opt/frameGrabbers/SiSo-Runtime-5.7.0/lib64/"] )
	env.Append(LIBS=["fglib5", "siso_genicam"] )
	

def SetFittingConfig(env):
	FindSiSo(env)

