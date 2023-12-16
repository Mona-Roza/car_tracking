
cmake_minimum_required(VERSION 3.16)

# Setting cmake message
message("Mona Roza's Project CMAKE")

# Adding some extra component directories
set(EXTRA_COMPONENT_DIRS ../libs)

#Esp reads this variable and puts into application.bin, so we need to set twice
#Both here and both below!
set(PROJECT_VER "0.0.1")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

#For bin file name use version info as well, so the output will look "CAR_TRACKING-0-0-0.bin"
project(CAR_TRACKING-0-0-1 VERSION 0.0.1 DESCRIPTION "CAR_TRACKING-0-0-1")

# Adding some compiling options for reading logs easily
idf_build_set_property(COMPILE_OPTIONS "-fdiagnostics-color=always" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wmissing-field-initializers" APPEND)
