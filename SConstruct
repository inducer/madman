import os
import glob

opts = Options(	"my_options.py")

opts.Add("qt_directory", "Path to Qt directory", "not specified")

opts.Add("xmms_config_program", "Path to xmms-config", "not specified")
opts.Add("glib_config_program", "Path to glib-config", "not specified")

opts.Add("taglib_include", "Include path for taglib", "/usr/include/taglib")
opts.Add("taglib_library", "Library path for taglib", "not specified")

opts.Add(BoolOption("with_m4a","Enable m4a support.  Requires mp4v2/faac",1))
opts.Add("mp4_include", "Include path for mp4.h", "not specified")
opts.Add("mp4_library", "Library path for mp4v2", "not specified")

opts.Add("my_cc", "The C Compiler you want to use", "not specified")
opts.Add("my_cxx", "The C++ Compiler you want to use", "not specified")

opts.Add("prefix", "The root installation path", "/usr/local")
opts.Add("install_to", "Where to actually install to", "$prefix")

opts.Add("add_c_include_dirs", "A comma-separated list of include paths", "not specified")
opts.Add("add_lib_dirs", "A comma-separated list of library paths", "not specified")

opts.Add(BoolOption("maintainer_mode", "Whether you want to use some special ingredients", 0))

env = Environment(
    ENV = {
      'PATH' : os.environ[ 'PATH' ],
      'HOME' : os.environ[ 'HOME' ] # required for distcc
    }, options = opts)
env.SConsignFile(".scons-signatures-database")
env.Replace(CPPPATH = [], LIBS = [], LIBPATH = [], CPPDEFINES = [],
    CXXFLAGS = "", CCFLAGS = "")
Help(opts.GenerateHelpText(env))

if env["add_c_include_dirs"] != "not specified":
  env["CPPPATH"] += env["add_c_include_dirs"].split(",")
if env["add_lib_dirs"] != "not specified":
  env["LIBPATH"] += env["add_lib_dirs"].split(",")



# Tool functions --------------------------------------------------------------
def RunProgramAndGetResult(commandline):
  file = os.popen(commandline, "r")
  result = file.read().rstrip()
  exit_code = file.close()
  if exit_code is not None:
    raise RuntimeError, "Program exited with non-zero exit code."
  return result




def DoWithVariables(variables, prefix, what):
  saved_variables = { }
  for name in variables.keys():
    saved_variables[ name ] = env[ name ][:]
    if isinstance(variables[ name ], list):
      env[ name ].extend(variables[ name ])
    else:
      env[ name ].append(variables[ name ])

  result = what()
  
  for name in saved_variables.keys():
    env[ name ] = saved_variables[ name ]
    env[ prefix+name ] = variables[ name ]

  return result




def AttemptLinkWithVariables(context, variables, code, extension, prefix):
  return DoWithVariables(variables, prefix, lambda: context.TryLink(code, extension))




def SplitIncludePath(istring):
  split_string = istring.split()
  result = []
  for i in split_string:
    if i[0:2] == "-I":
      i = i[2:]
    result.append(i)
  return result





def SplitLibPath(lstring):
  split_string = lstring.split()
  result = []
  for i in split_string:
    if i[0:2] == "-L":
      i = i[2:]
      result.append(i)
  return result




def SplitLibs(lstring):
  split_string = lstring.split()
  result = []
  for i in split_string:
    if i[0:2] == "-l":
      i = i[2:]
      result.append(i)
  return result




# My personal checks ----------------------------------------------------------
def CheckForQtAt(context, qtdir, variant, libdir):
  context.Message("Checking for %s at %s (with %s) " % (variant, qtdir, libdir))
  result = AttemptLinkWithVariables(context,
      { "LIBS": variant, "LIBPATH": qtdir+"/"+libdir, 
        "CPPPATH": qtdir + '/include' },
                                    """
                                    #include <qapplication.h>
                                    int main(int argc, char **argv) { 
                                    QApplication qapp(argc, argv);
                                    return 0;
                                    }
                                    """, ".cpp", "QT_")
  context.Result(result)
  return result




def CheckForQt(context):
  potential_qt_dirs = [
    "/usr/share/qt3", # Debian unstable
    "/usr/share/qt",
    "/usr",
    "/usr/local",
    "/usr/lib/qt3", # Suse
    "/usr/lib/qt" ]

  if os.environ.has_key('QTDIR'):
    potential_qt_dirs.insert(0, os.environ[ 'QTDIR' ])
  
  if env[ 'qt_directory' ] != "not specified":
    potential_qt_dirs.insert(0, env[ 'qt_directory' ])

  for libdir in ["lib", "lib64"]:
    for qtdir in potential_qt_dirs:
      for variant in ["qt", "qt-mt"]:
        if CheckForQtAt(context, qtdir, variant, libdir):
          context.env.Replace(QTDIR = qtdir)
          context.env.Replace(QT_LIBPATH = qtdir + "/" + libdir)
          context.env.Replace(QT_LIBRARY_NAME = variant)
          return 1
  return 0




def CheckForConfigGetterPackage(context, name):
  context.Message('Checking for %s... ' % name)
  config_getter = "%s-config" % name

  if env[ '%s_config_program' % name ] != "not specified":
    config_getter = env[ '%s_config_program' % name ]
  
  uppercase_name = name.upper()
  try:
    cflags = RunProgramAndGetResult("%s --cflags" % config_getter)
  except RuntimeError:
    context.Result(("*** ERROR *** %s is not on the path or can't be run as expected.\n"+
	"Please make sure %s-dev or %s-devel is installed") % (config_getter, name, name))
    return 0
  try:
    libs = RunProgramAndGetResult("%s --libs" % config_getter)
  except RuntimeError:
    context.Result(("*** ERROR *** %s is not on the path or can't be run as expected.\n"+
	"Please make sure %s-dev or %s-devel is installed") % (config_getter, name, name))
    return 0
  context.env[ uppercase_name + "_CPPPATH" ] = SplitIncludePath(cflags)
  context.env[ uppercase_name + "_LIBPATH" ] = SplitLibPath(libs)
  context.env[ uppercase_name + "_LIBS" ] = SplitLibs(libs)

  context.Result(1)
  return 1




def CheckForSimpleLibrary(name, header, libraries, call = "main();", language = "C++", serious = True):
  varprefix = name.upper() + "_"
  
  vars = { "CPPPATH" : [], "LIBPATH": [] }
  if env[ "%s_include" % name ] != "not specified":
    vars[ "CPPPATH" ] = [ env[ "%s_include" % name ] ]


  error_title = "ERROR"
  if not serious:
    error_title = "WARNING"

  if not DoWithVariables(vars, varprefix, lambda: conf.CheckCXXHeader(header)):
    print "*** %s *** The %s header for the %s package does not seem to be available." \
          % (error_title, header, name)
    print "You may call this build script with %s_include=<your path> as an argument to fix this." \
          % name
    print "Please make sure %s-dev or %s-devel is installed." % (name, name)
    print "Also check config.log for details on the failure."
    return 0

  if env[ "%s_library" % name ] != "not specified":
    vars[ "LIBPATH" ] = [ env[ "%s_library" % name ] ]

  previous_libs = env[ 'LIBS' ]
  library = libraries[ 0 ]
  vars[ "LIBS" ] = libraries[ 1: ]

  if not DoWithVariables(vars, varprefix, 
      lambda: conf.CheckLibWithHeader(library, header, language, call, False)):
    print "*** %s *** The %s library does not seem to be available." \
          % (error_title, name)
    print "You may call this build script with %s_library=<your path> as an argument to fix this." \
          % name
    print "Please make sure %s-dev or %s-devel is installed." % (name, name)
    print "Also check config.log for details on the failure."
    return 0

  env[ varprefix + "LIBS" ] = libraries
  env[ 'LIBS' ] = previous_libs

  return 1




def CheckForXmms(context):
  return CheckForConfigGetterPackage(context, "xmms")
def CheckForGlib(context):
  return CheckForConfigGetterPackage(context, "glib")




# Check subroutines -----------------------------------------------------------
def CheckForTagLib():
  return CheckForSimpleLibrary("taglib", "taglib.h", [ "tag", "z" ])
def CheckForMP4V2(serious):
  return CheckForSimpleLibrary("mp4","mp4.h",["mp4v2"], serious = serious)



# Configure -------------------------------------------------------------------
conf = Configure(env, custom_tests = { 
    "CheckForQt" : CheckForQt,
    "CheckForXmms": CheckForXmms,
    "CheckForGlib": CheckForGlib,
    })

if not conf.CheckForQt():
  Exit(1)
if not conf.CheckForXmms():
  Exit(1)
if not conf.CheckForGlib():
  Exit(1)

if not CheckForTagLib():
  print "*********************************************************"
  print "NOTE: madman has switched ID3 libraries yet again."
  print "  We don't use id3lib any more."
  print "  We don't use libid3tag from Underbit any more."
  print "  Instead, we're using Scott Wheeler's TagLib now."
  print "If you are trying to use id3lib and it doesn't work, this is why."
  print "*********************************************************"
  Exit(1)

if env["with_m4a"] and not CheckForMP4V2(serious = False):
  print "*** WARNING *** Disabling m4a support."
  env["with_m4a"] = 0

env = conf.Finish()



# Build! ----------------------------------------------------------------------
Export("env")

current_version = file("current-version", "r").read().strip()
env[ "CPPDEFINES" ].append(("MADMAN_VERSION", current_version))
env.Replace(CXXSUFFIX = ".cpp")

if env[ 'my_cc' ] != "not specified":
  env.Replace(CC = env[ 'my_cc' ])
if env[ 'my_cxx' ] != "not specified":
  env.Replace(CXX = env[ 'my_cxx' ])

if env["maintainer_mode"]:
  builddir_name = ",build/maintmode"
  print "*** NOTICE *** Enabling maintainer mode."

  madman_sources = []
  madman_sources.extend(glob.glob("*/*.h"))
  madman_sources.extend(glob.glob("*/*.cpp"))
  #env.Command("tags", madman_sources, "ctags -R || true")

  env["CCFLAGS"] += " -fmessage-length=0 -g -Wall"
  env["CXXFLAGS"] += " -fmessage-length=0 -g -Wall"
  env["CPPDEFINES"].append(("MADMAN_DEBUG", "1"))
else:
  builddir_name = ",build/release"
  env[ "CPPDEFINES"].append(("NDEBUG", "1"))

if env["with_m4a"]:
  env["CPPDEFINES"].append(("WITH_M4A","1"))

plugin_list = glob.glob(os.path.join("plugins", "[a-z]*"))
plugin_list = filter(lambda x: x.find("example") == -1, plugin_list)
env.Install("$install_to/lib/madman/plugins", plugin_list)

env.Alias("install", "$install_to/bin")
env.Alias("install", "$install_to/lib/madman/plugins")

for dir in ["expat", "utility", "database", "httpd", "designer/images", "designer",
            "ui", "main"]:
  SConscript("%s/SConscript" % dir, build_dir = "%s/%s" % (builddir_name, dir), duplicate = 0)
