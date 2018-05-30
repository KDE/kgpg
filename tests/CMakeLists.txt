find_package(Qt5Test REQUIRED)
set(test_LIBS kgpg_lib Qt5::Test)

set(COMMON common.h common.cpp)

function(create_test name)
	add_executable(${name} ${ARGN} ${COMMON})
	target_link_libraries(${name} ${test_LIBS})
	add_test(NAME ${name} COMMAND ${name})
	set_property(TEST ${name} PROPERTY ENVIRONMENT
		"XDG_CONFIG_HOME=${CMAKE_BINARY_DIR}/tests")
	set_property(TEST ${name} PROPERTY RUN_SERIAL TRUE) #since gpg config home is shared between each test
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

set(GpgConf .gnupg/gpg.conf)
set(KgpgConf kgpgrc)
set(Keys
	keys/kgpgtest_BA7695F3C550DF14.asc
	keys/kgpgtest_BA7695F3C550DF14.pass
	keys/kgpgtest_BA7695F3C550DF14_pub.asc
	keys/signed_bad_sig
	keys/signed_text
	keys/encrypted_text.txt
	keys/encrypted_text_hide_key_id.txt
)

add_custom_target(test-data)
add_custom_command(TARGET test-data  PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E
	copy_directory ${CMAKE_SOURCE_DIR}/tests/.gnupg ${CMAKE_BINARY_DIR}/tests/gnupg)

add_custom_command(TARGET test-data  PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E
	copy_directory ${CMAKE_SOURCE_DIR}/tests/keys ${CMAKE_BINARY_DIR}/tests/keys)

add_custom_command(TARGET test-data  PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E
	copy ${CMAKE_SOURCE_DIR}/tests/kgpgrc
	${CMAKE_BINARY_DIR}/tests/kgpgrc)
