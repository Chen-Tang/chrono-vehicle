SET(MODEL_NAME slider_crank)
SET(MODEL_FILES
  slider_crank.cpp
  )

SET(LIBRARIES
    ${CHRONOENGINE_LIBRARIES}
    ChronoVehicle_Utils
  )

ADD_EXECUTABLE(${MODEL_NAME}  ${MODEL_FILES})
SOURCE_GROUP(""  FILES ${MODEL_FILES})

SET_TARGET_PROPERTIES(${MODEL_NAME} PROPERTIES
  FOLDER models
  COMPILE_FLAGS "${CH_BUILDFLAGS}"
  LINK_FLAGS "${CH_LINKERFLAG_EXE}"
  )

TARGET_LINK_LIBRARIES(${MODEL_NAME} ${LIBRARIES})

INSTALL(TARGETS ${MODEL_NAME} DESTINATION bin)



