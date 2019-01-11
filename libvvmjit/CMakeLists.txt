get_filename_component(VVMJIT_INCLUDE_DIR ../include ABSOLUTE)

set(SOURCES
	${VVMJIT_INCLUDE_DIR}/vvm.h
	JIT.cpp				JIT.h
	Arith256.cpp		Arith256.h
	Array.cpp			Array.h
	BasicBlock.cpp		BasicBlock.h
	Cache.cpp			Cache.h
						Common.h
	Compiler.cpp		Compiler.h
	CompilerHelper.cpp	CompilerHelper.h
	Endianness.cpp		Endianness.h
	ExecStats.cpp		ExecStats.h
	Ext.cpp				Ext.h
	GasMeter.cpp		GasMeter.h
	Instruction.cpp		Instruction.h
	Memory.cpp			Memory.h
	Optimizer.cpp		Optimizer.h
	RuntimeManager.cpp	RuntimeManager.h
	Type.cpp			Type.h
	Utils.cpp			Utils.h
)
source_group("" FILES ${SOURCES})

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fvisibility=hidden")
	if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--exclude-libs,ALL") # Do not export symbols from dependies, mostly LLVM libs
	endif()
endif()

if(VVMJIT_VERSION_MAJOR EQUAL 0)
	set(VVMJIT_SOVERSION "0.${VVMJIT_VERSION_MINOR}")
else()
	set(VVMJIT_SOVERSION ${VVMJIT_VERSION_MAJOR})
endif()


string(COMPARE EQUAL "${LLVM_ENABLE_ASSERTIONS}" "ON" LLVM_DEBUG)
configure_file(BuildInfo.h.in ${CMAKE_CURRENT_BINARY_DIR}/gen/BuildInfo.gen.h)

add_library(vvmjit ${SOURCES} gen/BuildInfo.gen.h)
# Explicit dependency on llvm to download LLVM header files.
add_dependencies(vvmjit LLVM::JIT)
get_target_property(LLVM_COMPILE_DEFINITIONS LLVM::JIT INTERFACE_COMPILE_DEFINITIONS)
if (LLVM_COMPILE_DEFINITIONS)
	target_compile_definitions(vvmjit PRIVATE ${LLVM_COMPILE_DEFINITIONS})
endif()
get_target_property(LLVM_INCLUDE_DIRECTORIES LLVM::JIT INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(vvmjit SYSTEM PRIVATE ${LLVM_INCLUDE_DIRECTORIES})
target_include_directories(vvmjit PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gen)
target_include_directories(vvmjit PUBLIC ${VVMJIT_INCLUDE_DIR})

target_link_libraries(vvmjit PRIVATE LLVM::JIT)
set_target_properties(vvmjit PROPERTIES
	VERSION ${VVMJIT_VERSION}
	SOVERSION ${VVMJIT_SOVERSION}
	FOLDER "libs"
)

include(GNUInstallDirs)
install(TARGETS vvmjit
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY ${VVMJIT_INCLUDE_DIR}/
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# This section configures vvmjit-standalone -- a static library containing
# vvmjit and all its static dependencies.

# This function tries to get a list of all static library dependencies of
# a target. It probably does not handle all cases to be generic, but works for
# LLVM case.
function(get_link_libraries OUTPUT_LIST TARGET)
	get_target_property(IMPORTED ${TARGET} IMPORTED)
	list(APPEND VISITED_TARGETS ${TARGET})
	if (IMPORTED)
		get_target_property(LIBS ${TARGET} INTERFACE_LINK_LIBRARIES)
	else()
		get_target_property(LIBS ${TARGET} LINK_LIBRARIES)
	endif()
	set(LIB_FILES "")
	foreach(LIB ${LIBS})
		if (TARGET ${LIB})
			list(FIND VISITED_TARGETS ${LIB} VISITED)
			if (${VISITED} EQUAL -1)
				get_target_property(LIB_FILE ${LIB} LOCATION)
				get_link_libraries(LINK_LIB_FILES ${LIB})
				list(APPEND LIB_FILES ${LIB_FILE} ${LINK_LIB_FILES})
			endif()
		elseif (IS_ABSOLUTE ${LIB})
			# Here LIB is a full path to a (hopefuly static) library. We don't
			# want system libs (usually dynamic) here.
			list(APPEND LIB_FILES ${LIB})
		endif()
	endforeach()
	set(VISITED_TARGETS ${VISITED_TARGETS} PARENT_SCOPE)
	list(REMOVE_DUPLICATES LIB_FILES)
	set(${OUTPUT_LIST} ${LIB_FILES} PARENT_SCOPE)
endfunction()

# When building static lib add additional target vvmjit-standalone --
# an archive containing all LLVM dependencies in a single file.
get_target_property(_vvmjit_type vvmjit TYPE)
if (_vvmjit_type STREQUAL STATIC_LIBRARY)
	get_link_libraries(VVMJIT_LINK_LIBRARIES vvmjit)
	set(VVMJIT_STANDALONE_FILE ${CMAKE_STATIC_LIBRARY_PREFIX}vvmjit-standalone${CMAKE_STATIC_LIBRARY_SUFFIX})
	if (MSVC)
		set(_dummy_file ${CMAKE_CURRENT_BINARY_DIR}/vvmjit-standalone.c)
		if (NOT EXISTS ${_dummy_file})
			file(WRITE ${_dummy_file} "// vvmjit-standalone dummy file")
		endif()
		add_library(vvmjit-standalone STATIC EXCLUDE_FROM_ALL ${_dummy_file})
		add_dependencies(vvmjit-standalone vvmjit)
		string(REPLACE ";" " " FLAGS "${VVMJIT_LINK_LIBRARIES}")
		set(FLAGS "${CMAKE_CFG_INTDIR}/vvmjit.lib ${FLAGS}")
		set_target_properties(vvmjit-standalone PROPERTIES STATIC_LIBRARY_FLAGS "${FLAGS}")
		install(TARGETS vvmjit-standalone
				RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
				LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
				ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
	elseif (APPLE)
		add_custom_command(OUTPUT ${VVMJIT_STANDALONE_FILE}
						   COMMAND libtool -static -o ${VVMJIT_STANDALONE_FILE} $<TARGET_FILE:vvmjit> ${VVMJIT_LINK_LIBRARIES}
						   VERBATIM)
		add_custom_target(vvmjit-standalone DEPENDS ${VVMJIT_STANDALONE_FILE})
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${VVMJIT_STANDALONE_FILE} DESTINATION ${CMAKE_INSTALL_LIBDIR} OPTIONAL)
	elseif (CMAKE_AR)
		# For platforms using ar a linker scripts is created and used to create
		# the standalone library.

		string(REPLACE ";" "\\naddlib " AR_SCRIPT ";${VVMJIT_LINK_LIBRARIES}")
		set(AR_SCRIPT "create ${VVMJIT_STANDALONE_FILE}\\naddlib $<TARGET_FILE:vvmjit>${AR_SCRIPT}\\nsave\\nend\\n")
		set(AR_SCRIPT_FILE ${CMAKE_STATIC_LIBRARY_PREFIX}vvmjit-standalone.ar-script)

		# Generate the linker script.
		add_custom_command(OUTPUT ${AR_SCRIPT_FILE}
				COMMAND printf ${AR_SCRIPT} > ${AR_SCRIPT_FILE}
				DEPENDS $<TARGET_FILE:vvmjit>
				VERBATIM)

		# Execute the script.
		add_custom_command(OUTPUT ${VVMJIT_STANDALONE_FILE}
				COMMAND ${CMAKE_AR} -M < ${AR_SCRIPT_FILE}
				MAIN_DEPENDENCY ${AR_SCRIPT_FILE}
				VERBATIM)

		add_custom_target(vvmjit-standalone DEPENDS ${VVMJIT_STANDALONE_FILE})

		# The "thin" library is also provided. It is smaller that the standalone one
		# but cannot be redistributed.
		set(VVMJIT_STANDALONE_THIN_FILE ${CMAKE_STATIC_LIBRARY_PREFIX}vvmjit-standalone-thin${CMAKE_STATIC_LIBRARY_SUFFIX})

		add_custom_command(OUTPUT ${VVMJIT_STANDALONE_THIN_FILE}
				COMMAND ${CMAKE_AR} cTq ${VVMJIT_STANDALONE_THIN_FILE} $<TARGET_FILE:vvmjit> ${VVMJIT_LINK_LIBRARIES})

		add_custom_target(vvmjit-standalone-thin ALL DEPENDS ${VVMJIT_STANDALONE_THIN_FILE})
		# FIXME: Looks it will be better to create vvmjit-standalone as a library with costum steps instead of custom taget.
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${VVMJIT_STANDALONE_FILE} DESTINATION ${CMAKE_INSTALL_LIBDIR} OPTIONAL)
	endif()

	# Export the full path to vvmjit-standalone library to be used in tests.
	# TODO: It would be easier if vvmjit-standalone was a library target, but it
	#       works for now.
	set(VVMJIT_STANDALONE_LIB ${CMAKE_CURRENT_BINARY_DIR}/${VVMJIT_STANDALONE_FILE} PARENT_SCOPE)
endif()
