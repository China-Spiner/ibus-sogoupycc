CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

IF(NOT DEFINED SHARE_INSTALL_PREFIX)
  SET(PKGDATADIR "${CMAKE_INSTALL_PREFIX}/share/ibus-sogoupycc")
ELSE()
  SET(PKGDATADIR "${SHARE_INSTALL_PREFIX}/ibus-sogoupycc")
ENDIF()

ADD_EXECUTABLE(ibus-sogoupycc PinyinUtility.cpp;LuaBinding.cpp;PinyinDatabase.cpp;PinyinCloudClient.cpp;engine.cpp;XUtility.cpp;main.cpp)

# Archlinux, OS X use 'lua' as pkg-config name
# While debian/ubuntu uses 'lua5.1'
# FreeBSD may use 'lua-5.1'
# we will detect which one to use

FIND_PACKAGE(PkgConfig)
PKG_SEARCH_MODULE(LUA51 REQUIRED lua5.1 lua-5.1 lua)
PKG_CHECK_MODULES(REQPKGS REQUIRED ibus-1.0>=1.2.0;ibus-1.0<=1.2.98;glib-2.0>=2.22;gthread-2.0>=2.22;dbus-1>=1.2;sqlite3;gtk+-2.0;gdk-2.0)

SET_SOURCE_FILES_PROPERTIES(${SRCS} COMPILE_FLAGS "-pthread -DPKGDATADIR=${PKGDATADIR}")
SET_TARGET_PROPERTIES(ibus-sogoupycc PROPERTIES LINK_FLAGS "-s")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_DIR};${REQPKGS_INCLUDE_DIRS};${LUA51_INCLUDE_DIRS})
LINK_DIRECTORIES(${REQPKGS_LIBRARY_DIRS};${LUA51_LIBRARY_DIRS})
TARGET_LINK_LIBRARIES(ibus-sogoupycc ${REQPKGS_LIBRARIES};${LUA51_LIBRARIES})

INSTALL(TARGETS ibus-sogoupycc DESTINATION ${PKGDATADIR}/engine)
