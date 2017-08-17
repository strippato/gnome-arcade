#  BUILD:
# cd gnome-arcade
# meson builddir
# cd builddir
# ninja
#
# RUN:
# cp gnome-arcade ..
# cd ..
# ./gnome-arcade


project ('gnome-arcade', 'c', version : '0.188', license : 'GPL3')

# CARGS
#cargs = ['-Wall', '-O2', '-march=native']
#cargs = ['-Wall', '-O3', '-march=native']
#cargs = ['-Os', '-march=native', '-fomit-frame-pointer', '-flto', '-fwhole-program', '-frename-registers']
cargs = ['-Ofast', '-march=native', '-fomit-frame-pointer', '-flto', '-fwhole-program', '-frename-registers']

# DEPENDENCIES
gtkdep = dependency ('gtk+-3.0', version : '>=3.20')
evdevdep = dependency ('libevdev', version : '>=1.4.5')
vlcdep = dependency ('libvlc', version : '>=3.0')

dep = [gtkdep, evdevdep, vlcdep]

# GNOME-ARCADE SOURCE
src = [ 'src/app.c',        
		'src/blacklist.c', 
		'src/config.c',
		'src/filedownloader.c',
		'src/inforom.c',
		'src/joy.c',
		'src/mame.c',
		'src/pref.c',
		'src/rescan.c',
		'src/rom.c',
		'src/ssaver.c',
		'src/ui.c',
		'src/uipref.c',
		'src/util.c',
		'src/view.c',
		'src/vlc.c',
		'src/www.c' 
 	  ]

# GNOME-ARCADE INCLUDE
incdir = include_directories ('header')

# BUILD
executable ('gnome-arcade', sources : src, dependencies : dep, include_directories : incdir, c_args : cargs)
