add_executable(KINGVON WIN32 main.cc videoplayer.cc resource.rc)
target_link_libraries(KINGVON PRIVATE mfplat mf mfuuid strmiids shlwapi ole32)