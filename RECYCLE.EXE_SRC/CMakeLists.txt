add_executable(RECYCLE WIN32 recycle.c itemmovectx.c resource.rc)
target_link_libraries(RECYCLE PRIVATE Comctl32)