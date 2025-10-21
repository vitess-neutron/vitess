#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "NCrystal::NCrystal" for configuration "Release"
set_property(TARGET NCrystal::NCrystal APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(NCrystal::NCrystal PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/NCrystal.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS NCrystal::NCrystal )
list(APPEND _IMPORT_CHECK_FILES_FOR_NCrystal::NCrystal "${_IMPORT_PREFIX}/lib/NCrystal.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
