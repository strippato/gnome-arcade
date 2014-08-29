G_SLICE=always-malloc valgrind --tool=memcheck --show-reachable=yes --leak-check=full --leak-resolution=high --num-callers=20 ./gnome-arcade 


