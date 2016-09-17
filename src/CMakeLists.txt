set (EXECUTABLE_OUTPUT_PATH "../..")
# source file
add_executable(gnome-arcade app.c config.c mame.c pref.c rom.c ui.c util.c view.c www.c uipref.c rescan.c joy.c ssaver.c blacklist.c filedownloader.c inforom.c vlc.c)

# inclue file
include_directories(../header)

target_link_libraries(gnome-arcade ${GTK3_LIBRARIES} ${PC_LIBEVDEV_LIBRARIES} ${LIBVLC_LIBRARIES})

