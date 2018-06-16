find_package(Qt5Test REQUIRED)
set(test_LIBS kgpg_lib Qt5::Test)

set(COMMON common.h common.cpp)

function(create_test name)
	add_executable(${name} ${ARGN} ${COMMON})
	target_link_libraries(${name} ${test_LIBS})
	add_test(NAME ${name} COMMAND ${name})
	add_dependencies(${name} test-data)
endfunction()

create_test(kgpg-import kgpgimport.h kgpgimport.cpp)
create_test(kgpg-verify kgpgverify.h kgpgverify.cpp)
create_test(kgpg-change-trust kgpgchangetrust.h kgpgchangetrust.cpp)
create_test(kgpg-disable kgpgchangedisable.h kgpgchangedisable.cpp)
create_test(kgpg-interface kgpginterface.h kgpginterface.cpp)
create_test(kgpg-encrypt kgpgencrypt.h kgpgencrypt.cpp)
create_test(kgpg-decrypt kgpgdecrypt.h kgpgdecrypt.cpp)
create_test(kgpg-del-key kgpgdelkey.h kgpgdelkey.cpp)
create_test(kgpg-add-photo kgpgaddphoto.h kgpgaddphoto.cpp)
create_test(kgpg-add-uid kgpgadduid.h kgpgadduid.cpp)
create_test(kgpg-export kgpgexport.h kgpgexport.cpp)

add_custom_target(test-data)

add_custom_command(TARGET test-data  PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E
	copy_directory ${CMAKE_SOURCE_DIR}/tests/keys ${CMAKE_BINARY_DIR}/tests/keys)
