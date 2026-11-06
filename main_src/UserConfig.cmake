###    USER SETTINGS  START    ###

#
# DATA PROTECTION:
# --no-dup
# --reddi
# --eddi
#
# DATA PROTECTION MODIFIERS:
# --coarse-grained
# --multiple-errbb
#
# CONTROL PROTECTION:
# --no-cfc
# --rasm
# --inter-rasm
set(ASPIS_CONFIG --no-dup --no-cfc)
# set(ASPIS_CONFIG --eddi --coarse-grained --multiple-errbb --rasm)

# Below settings can be customized
# User needs to edit it manually as per their needs.
###    DO NOT ADD OR REMOVE VARIABLES FROM THIS SECTION    ###
# -----------------------------------------
# Add any compiler definitions, they will be added as extra definitions
# Example : Adding VERBOSE=1 will pass -DVERBOSE=1 to the compiler.
set(USER_COMPILE_DEFINITIONS
""
)

# Undefine any previously specified compiler definitions, either built in or provided with a -D option
# Example : Adding MY_SYMBOL will pass -UMY_SYMBOL to the compiler.
set(USER_UNDEFINED_SYMBOLS
"__clang__"
)


# Add any directories below, they will be added as extra include directories.
# Example 1: Adding /proj/data/include will pass -I/proj/data/include.
# Example 2: Adding ../../common/include will consider the path as relative to this component directory.
# Example 3: Adding ${CMAKE_SOURCE_DIR}/data/include to add data/include from this project.


set(USER_INCLUDE_DIRECTORIES
	"${CMAKE_CURRENT_LIST_DIR}/inc"
)

#Add any source below, they will be added as Compile sources.
#Example 1: Adding /proj/data/helloworld.c will pass /proj/data/helloworld.c
#Example 2: Adding ../../common/helloworld.c will consider the path as relative to this component directory
#Example 3: Adding ${MY_ENV}/data/helloworld.c are expanded using project-specific environment settings.
set(USER_COMPILE_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/main.c"
    "${CMAKE_CURRENT_LIST_DIR}/lib/common/crc.c"
    "${CMAKE_CURRENT_LIST_DIR}/lib/common/EigenVector.c"
    "${CMAKE_CURRENT_LIST_DIR}/lib/common/Hessenberg.c"
    "${CMAKE_CURRENT_LIST_DIR}/lib/common/Householder.c"
    "${CMAKE_CURRENT_LIST_DIR}/lib/common/Jacobi.c"
    "${CMAKE_CURRENT_LIST_DIR}/lib/common/Matrix.c"
    "${CMAKE_CURRENT_LIST_DIR}/lib/common/MatrixView.c"
    "${CMAKE_CURRENT_LIST_DIR}/lib/common/Schur.c"
    "${CMAKE_CURRENT_LIST_DIR}/lib/common/Utils.c"
    "${CMAKE_CURRENT_LIST_DIR}/lib/GenerateAngle/libgenerateangle.c"
    "${CMAKE_CURRENT_LIST_DIR}/lib/EstimateAngle/libestimateangle.c"
    "${CMAKE_CURRENT_LIST_DIR}/lib/CalculateCoeff/libcoefficients.c"
)

# -----------------------------------------
