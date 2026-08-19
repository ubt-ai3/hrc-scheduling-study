find_package(Sphinx REQUIRED)
find_package(Doxygen)


# doxygen settings can be set here, prefixed with "DOXYGEN_"
set(DOXYGEN_GENERATE_XML YES)
set(DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/docs/_build")
string(APPEND DOXYGEN_ALIASES ";rst=\\verbatim embed:rst:leading-asterisk")
string(APPEND DOXYGEN_ALIASES ";endrst=\\endverbatim")



set(DOCUMENTED_HEADERS   ${PROJECT_SOURCE_DIR}/src/abstract_robot.h)

doxygen_add_docs(
  build_docs
  ${DOCUMENTED_HEADERS}
  COMMENT "Generate API-documents for NoteSearch."
)


set(SPHINX_SOURCE ${PROJECT_SOURCE_DIR}/docs)
set(SPHINX_BUILD ${PROJECT_SOURCE_DIR}/docs/_build)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

# Only regenerate Sphinx when:
# - Doxygen has rerun
# - Our doc files have been updated
# - The Sphinx config has been updated
add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
                   COMMAND 
                   ${SPHINX_EXECUTABLE} -b html
                   ${SPHINX_SOURCE} ${SPHINX_BUILD}
                   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/docs
                   DEPENDS
                   # Other docs files you want to track should go here (or in some variable)
                   ${CMAKE_CURRENT_SOURCE_DIR}/index.rst
				   ${CMAKE_CURRENT_SOURCE_DIR}/pilot_study.rst
                   MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
                   COMMENT "Generating documentation with Sphinx")


# Nice named target so we can run the job easily
add_custom_target(Sphinx ALL DEPENDS ${SPHINX_INDEX_FILE} ${DOCUMENTED_HEADERS})
add_dependencies(Sphinx build_docs)