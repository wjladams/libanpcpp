# Example programs. Each source file becomes its own runnable executable that
# prints inputs, intermediate matrices, and results for one ANP model.

set(ANPCPP_EXAMPLES
  tree134
  network23
  hamburger_std
  benefits_costs_subnet
  ratings_demo
  export_sample_models
)

foreach(example ${ANPCPP_EXAMPLES})
  add_executable(${example} ${example}.cpp)
  target_link_libraries(${example} PRIVATE anpcpp::anpcpp)
  target_include_directories(${example} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
