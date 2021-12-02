from SCons.Script import *

def FindSISO(env):
	#
	# enable the use of the Silicon Software SDK for using the CoaXPress framegrabbers
	# and the JAI cameras.
	#
	env.Append(CPPDEFINES=["USE_SISO"])
	env.Append(CPPPATH=["/opt/frameGrabbers/SiSo-Runtime-5.7.0/include/"] )
	env.Append(LIBPATH=["/opt/frameGrabbers/SiSo-Runtime-5.7.0/lib64/"] )
	env.Append(LIBS=["fglib5", "siso_genicam"] )

def FindGTKMM(env):
	#
	# enable the use of gtkmm for the GUI stuff.
	#
	env.ParseConfig("pkg-config gtkmm-3.0 --cflags --libs");

def SetReconConfig(env):
	FindSISO(env)
	FindGTKMM(env)
