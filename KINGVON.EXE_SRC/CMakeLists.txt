add_executable(KINGVON WIN32 kingvon.c keyblock.c videoplayer.c resource.rc)
target_link_libraries(KINGVON PRIVATE mfplat mf mfplay mfuuid shlwapi)