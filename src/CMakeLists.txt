set (EXECUTABLE_OUTPUT_PATH "../..")
# source file
add_executable(gnome-arcade app.c config.c mame.c pref.c rom.c ui.c util.c view.c www.c uipref.c)

# inclue file
include_directories(../header)

target_link_libraries(gnome-arcade ${GTK3_LIBRARIES})

