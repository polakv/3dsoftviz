PROJECT(3DVisual)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET (CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

# Added path to directory with FindCuda cmake script 
SET(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}/dependencies/Source/osgCompute-1.1/CMakeModules/cuda")     

# Where to install module parts:
SET (INSTALL_BIN . CACHE PATH "Directory for binaries.")
SET (INSTALL_LIB . CACHE PATH "Directory for external libraries.")
#~ SET (INSTALL_BIN bin CACHE PATH "Directory for binaries.")
#~ SET (INSTALL_LIB lib CACHE PATH "Directory for external libraries.")
SET (INSTALL_INC include CACHE PATH "Directory for library headers.")	
SET (INSTALL_DATA . CACHE PATH "Directory the package can store documentation, tests or other data in.")
SET (INSTALL_DOC ${INSTALL_DATA}/doc CACHE PATH "Recommended directory to install documentation into.")
SET (INSTALL_ETC ${INSTALL_DATA}/etc CACHE PATH "Other files.")
SET (INSTALL_TEST ${INSTALL_DATA}/test CACHE PATH "Test files.")
SET (INSTALL_EXAMPLE ${INSTALL_DATA}/example CACHE PATH "Recommended directory to install examples into.")

### Find libraries ###

# Where to search
IF (WIN32)
	SET (CMAKE_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/Windows/include
				${CMAKE_INCLUDE_PATH})
	SET (CMAKE_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/Windows/lib
				${CMAKE_LIBRARY_PATH})
	MESSAGE("LIB PATH:"${CMAKE_LIBRARY_PATH})
ELSEIF(UNIX)
	SET (CMAKE_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/Linux/include
				${CMAKE_INCLUDE_PATH})

	SET (CMAKE_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/Linux/lib
				${CMAKE_LIBRARY_PATH})
ENDIF()

# Find Qt
FIND_PACKAGE(Qt4 REQUIRED)
SET(QT_USE_QTOPENGL 1)
SET(QT_USE_QTSQL 1)
SET(QT_USE_QTWEBKIT 1)
SET(QT_USE_QTXML 1)
SET(QT_USE_QTNETWORK 1)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${QT_INCLUDE_DIR} )

# Find OpenGL
FIND_PACKAGE(OpenGL)

# Find OpenSceneGraph
FIND_PACKAGE(OpenSceneGraph COMPONENTS 
  osgGA 
  osgUtil 
  OpenThreads 
  osg 
  osgAnimation 
  osgDB
  osgFX
  osgManipulator
  osgParticle
  osgShadow
  osgSim
  osgTerrain
  osgText
  osgVolume
  osgWidget
  osgViewer
) 
MESSAGE ("osg libraries:: ${OPENSCENEGRAPH_LIBRARIES}")

# set specific argument to nvcc compiler for paralel nsight debugging
SET (CUDA_NVCC_USER_OPTIONS "-arch sm_20 -maxrregcount=60 --ptxas-options=-v" CACHE	STRING "Set additional user specific compiler options to Cuda nvcc")

# Find Cuda
FIND_PACKAGE(Cuda)

# Include headers
INCLUDE_DIRECTORIES(
  ${OPENGL_INCLUDE_DIR}
  ${OPENSCENEGRAPH_INCLUDE_DIRS} 
  ${CUDA_INCLUDE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_INCLUDE_PATH}
  ${CMAKE_CURRENT_BINARY_DIR} "./"
)

# Static libraries headers
INCLUDE_DIRECTORIES(
	  ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/Source/libnoisesrc-1.0.0/src
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/Source/osgCompute-1.1/include
)

# Source files
FILE(GLOB_RECURSE SRC  "src/*.cpp")
FILE(GLOB_RECURSE INCL "include/*.h")

# .h subor, ktory obsahuje Q_OBJECT
SET(SOURCES_H 
	./include/OsgQtBrowser/QGraphicsViewAdapter.h 
	./include/QOSG/CoreWindow.h
	./include/QOSG/TreeModel.h
	./include/Util/Cleaner.h
	./include/QOSG/OptionsWindow.h
	./include/QOSG/LoadGraphWindow.h
	./include/QOSG/SelectLayoutWindow.h
	./include/QOSG/CheckBoxList.h
	./include/QOSG/qtcolorpicker.h
	./include/Viewer/PickHandler.h
)

# toto makro spracuje Q_OBJECT a vygeneruje novy cpp subor, ktory bude dostupny v ${SOURCES_H_MOC} premennej
QT4_WRAP_CPP( SOURCES_H_MOC ${SOURCES_H} )

# collect cuda kernel sources
SET(MY_CUDA_SOURCE_FILES
  ./src/Gpu/LayoutKernel.cu
)

# Use the WRAP_CUDA macro.
WRAP_CUDA( CUDA_FILES ${MY_CUDA_SOURCE_FILES} )

SET(SRC
    ${SRC}
    ${SOURCES_H_MOC}
    ${CUDA_FILES}
    ${MY_CUDA_SOURCE_FILES}
)  

ADD_EXECUTABLE(3DVisual ${INCL} ${SRC})

SOURCE_GROUP(\\src main.cpp)
SOURCE_GROUP(\\src\\Viewer "^.*Viewer/.*$")
SOURCE_GROUP(\\src\\Core "^.*Core/.*$")
SOURCE_GROUP(\\src\\Data "^.*Data/.*$")
SOURCE_GROUP(\\src\\Manager "^.*Manager/.*$")
SOURCE_GROUP(\\src\\Importer "^.*Importer/.*$")
SOURCE_GROUP(\\src\\Model "^.*Model/.*$")
SOURCE_GROUP(\\src\\Util "^.*Util/.*$")
SOURCE_GROUP(\\src\\Layout "^.*Layout/.*$")
SOURCE_GROUP(\\src\\QOSG "^.*QOSG/.*$")
SOURCE_GROUP(\\src\\OsgQtBrowser "^.*OsgQtBrowser/.*$")
SOURCE_GROUP(\\src\\Noise "^.*Noise/.*$")
SOURCE_GROUP(\\src\\Gpu "^.*Gpu/.*$")

SOURCE_GROUP(\\headers\\Viewer "^.*Viewer/.*h$")
SOURCE_GROUP(\\headers\\Core "^.*Core/.*h$")
SOURCE_GROUP(\\headers\\Data "^.*Data/.*h$")
SOURCE_GROUP(\\headers\\Manager "^.*Manager/.*h$")
SOURCE_GROUP(\\headers\\Importer "^.*Importer/.*h$")
SOURCE_GROUP(\\headers\\Model "^.*Model/.*h$")
SOURCE_GROUP(\\headers\\Util "^.*Util/.*h$")
SOURCE_GROUP(\\headers\\Layout "^.*Layout/.*h$")
SOURCE_GROUP(\\headers\\QOSG "^.*QOSG/.*h$")
SOURCE_GROUP(\\headers\\Noise "^.*Noise/.*h$")
SOURCE_GROUP(\\headers\\OsgQtBrowser "^.*OsgQtBrowser/.*h$")
SOURCE_GROUP(\\headers\\Gpu "^.*Gpu/.*h$")

SOURCE_GROUP(\\MOC "^.*moc_.*$")

# Build dependencies
ADD_SUBDIRECTORY(dependencies/Source)

TARGET_LINK_LIBRARIES(3DVisual	
  ${QT_LIBRARIES}
  ${OPENGL_LIBRARIES} 
  ${OPENSCENEGRAPH_LIBRARIES} 
  ${OSGVIEWER_LIBRARIES}
  ${CUDA_RUNTIME_LIBRARY}
  osgCompute
  osgCuda
  noise
)

#~ INSTALL(TARGETS 3DVisual DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/_INSTALL/Debug CONFIGURATIONS Debug) 
#~ INSTALL(TARGETS 3DVisual DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/_install/Release CONFIGURATIONS Release) 
#~ INSTALL(TARGETS 3DVisual RUNTIME DESTINATION ${INSTALL_BIN}) 

#~ INSTALL(FILES ${IGRAPH_LIBRARIES} DESTINATION ${INSTALL_DATA})

#~ INSTALL(DIRECTORY img DESTINATION ${INSTALL_DATA} PATTERN ".svn" EXCLUDE)
#~ INSTALL(DIRECTORY img DESTINATION ${CMAKE_CURRENT_BINARY_DIR} PATTERN ".svn" EXCLUDE)
