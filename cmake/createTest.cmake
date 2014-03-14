
#######################################
#
# createTest.cmake helps creating Tests
#  
# Usage:
#   - Integrate into your CMakeLists.txt
#   - Fill the LibraryDir, IncludeDir and Library lists
#   - Add your test via the createTest command
#
#  Variables
#    Boost_INCLUDE_DIRS	- Include directory for Boost
#    Boost_LIBRARIES    - Libraries (shared) of Boost (needed for linking)
#
#######################################


macro(createTest NAME)
    include_directories(${INCLUDEDIRS})
    link_directories(${LIBRARYDIRS})
    add_executable( ${PROJECT_NAME}_test_${NAME} ${ADDITIONALSOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/${NAME}TestSuite.cpp)
    target_link_libraries( ${PROJECT_NAME}_test_${NAME}
                           ${LIBRARIES})
    add_test(${NAME} ${PROJECT_NAME}_test_${NAME} )
    
    install(TARGETS ${PROJECT_NAME}_test_${NAME}
        RUNTIME DESTINATION ${PROJECT_NAME}/test)
endmacro(createTest NAME)