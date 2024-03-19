project (optee_app_boot_successful C)

set (SRC host/main.c)

add_executable (${PROJECT_NAME} ${SRC})

target_link_libraries (${PROJECT_NAME} PRIVATE teec)

install (TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
