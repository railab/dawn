# boards/common/CMakeLists.txt

# Only create the library if any fake drivers are enabled
if(CONFIG_DAWN_FAKE_DRIVERS)
  nuttx_add_library(board_common)

  target_include_directories(board_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  add_subdirectory(src)
endif()
