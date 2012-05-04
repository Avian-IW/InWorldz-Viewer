# -*- cmake -*-

# - Find XMLRPC-EPI
# Find the XMLRPC-EPI includes and library
# This module defines
#  XMLRPCEPI_INCLUDE_DIR, where to find jpeglib.h, etc.
#  XMLRPCEPI_LIBRARIES, the libraries needed to use XMLRPC-EPI.
#  XMLRPCEPI_FOUND, If false, do not try to use XMLRPC-EPI.
# also defined, but not for general use are
#  XMLRPCEPI_LIBRARY, where to find the XMLRPC-EPI library.

FIND_PATH(XMLRPCEPI_INCLUDE_DIR xmlrpc-epi/xmlrpc.h
/usr/local/include
/usr/include
)

SET(XMLRPCEPI_NAMES ${XMLRPCEPI_NAMES} xmlrpc-epi)
FIND_LIBRARY(XMLRPCEPI_LIBRARY
  NAMES ${XMLRPCEPI_NAMES}
  PATHS /usr/lib64 /usr/local/lib64 /usr/lib /usr/local/lib
  )

IF (XMLRPCEPI_LIBRARY AND XMLRPCEPI_INCLUDE_DIR)
    SET(XMLRPCEPI_LIBRARIES ${XMLRPCEPI_LIBRARY})
    SET(XMLRPCEPI_FOUND "YES")
ELSE (XMLRPCEPI_LIBRARY AND XMLRPCEPI_INCLUDE_DIR)
  SET(XMLRPCEPI_FOUND "NO")
ENDIF (XMLRPCEPI_LIBRARY AND XMLRPCEPI_INCLUDE_DIR)


IF (XMLRPCEPI_FOUND)
   IF (NOT XMLRPCEPI_FIND_QUIETLY)
      MESSAGE(STATUS "Found XMLRPC-EPI: ${XMLRPCEPI_LIBRARIES}")
   ENDIF (NOT XMLRPCEPI_FIND_QUIETLY)
ELSE (XMLRPCEPI_FOUND)
   IF (XMLRPCEPI_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find XMLRPC-EPI library")
   ENDIF (XMLRPCEPI_FIND_REQUIRED)
ENDIF (XMLRPCEPI_FOUND)

# Deprecated declarations.
SET (NATIVE_XMLRPCEPI_INCLUDE_PATH ${XMLRPCEPI_INCLUDE_DIR} )
GET_FILENAME_COMPONENT (NATIVE_XMLRPCEPI_LIB_PATH ${XMLRPCEPI_LIBRARY} PATH)

MARK_AS_ADVANCED(
  XMLRPCEPI_LIBRARY
  XMLRPCEPI_INCLUDE_DIR
  )
