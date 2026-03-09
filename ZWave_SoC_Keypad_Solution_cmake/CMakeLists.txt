# Define minimal required version of CMake.
cmake_minimum_required(VERSION "3.25")

# Project definition
project(ZWave_SoC_Keypad_Solution)

include(ExternalProject)

ExternalProject_Add(zwave_soc_keypad
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../zwave_soc_keypad/cmake_gcc
  BINARY_DIR ${CMAKE_CURRENT_LIST_DIR}/../zwave_soc_keypad/cmake_gcc/build
  CMAKE_ARGS --preset project
  BUILD_COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
  INSTALL_COMMAND ""
  BUILD_ALWAYS true
  USES_TERMINAL_BUILD 1
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../zwave_soc_keypad/cmake_gcc
)
ExternalProject_Add(bootloader-storage-internal-single-zwave-ota
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../bootloader-storage-internal-single-zwave-ota/cmake_gcc
  BINARY_DIR ${CMAKE_CURRENT_LIST_DIR}/../bootloader-storage-internal-single-zwave-ota/cmake_gcc/build
  CMAKE_ARGS --preset project
  BUILD_COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
  INSTALL_COMMAND ""
  BUILD_ALWAYS true
  USES_TERMINAL_BUILD 1
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../bootloader-storage-internal-single-zwave-ota/cmake_gcc
)

ExternalProject_Add_StepDependencies(bootloader-storage-internal-single-zwave-ota build zwave_soc_keypad)


set(post_build_command ${POST_BUILD_EXE} postbuild "./ZWave_SoC_Keypad_Solution.slpb")


add_custom_target(postbuild ALL
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
    COMMAND ${post_build_command}
)

add_dependencies(postbuild zwave_soc_keypad bootloader-storage-internal-single-zwave-ota)

# BEGIN_SIMPLICITY_STUDIO_METADATA=eJydUl1v00AQ/CuWVaH2wb4U3oKhQjEPFnaDcCI+ZOl0OW/To+db4z3XBcR/ZxM3TlHpAzze3uzM7O78DMuseJ9ni2z1WZardZotZbnM16tseSmLZfo2D+dhcnHX2OAWOjLoXlXheTyrwgCcxtq4LRfelIssq8KL15VLihJt7xlIAXc5mt+RYci19+1ciGEY4uFFjN1WPJ/NzsWnIi/1NTQqMo68chqYmTvmDi9VA9QqDSMgR638qP+Ai4xVG4o1NoJIHKRj0NgxE/sJgoTuq/sXvw+oAuvewge4go5nAQpMzeRkdTxgd7OXjmvlVRWKqTUtUrgyzuz6n8bfw7mhRfKb3ti69NBOZf44Fk9O2X2jXA3dWTDhg2ffevQvT04P7s/El4/qFmSJC/kOvreqltO4ZNvNiE/EY7VE/N1F0nb4FbQPHG+aJ/kx7PgJtbzZ8/Ml7HHpYk0cANGovjFOiRR134DzJEawPPiULWrxmEo8KbtB9BYVjx+Rx05tgbPgoXPKRsT5shDt6SLcrfa/Lf2bzHRx8cfJx0CJB4lKxDHw/Ax//QakiCun=END_SIMPLICITY_STUDIO_METADATA