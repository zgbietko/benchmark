add_library(sid_mkb sis_mkb_fem_intf.c sis_mkb_intf.c sih_mkb.h)

target_link_libraries(sid_mkb lsd_mkb utd_system)