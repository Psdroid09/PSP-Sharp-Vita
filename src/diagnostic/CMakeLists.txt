cmake_minimum_required(VERSION 3.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake"
        CACHE PATH "VitaSDK toolchain file")
  else()
    message(FATAL_ERROR "Set VITASDK to your VitaSDK installation.")
  endif()
endif()

project(PSPSharpProbe C)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -std=gnu99")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")

add_executable(PSPSharpProbe main.c)
target_link_libraries(PSPSharpProbe
  taihen_stub
  SceLibKernel_stub
  SceIofilemgr_stub
  SceLibc_stub_weak
)
set_target_properties(PSPSharpProbe PROPERTIES LINK_FLAGS "-nostdlib")

vita_create_self(PSPSharpProbe.suprx PSPSharpProbe
  UNSAFE
  CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/PSPSharpProbe.yml"
)
