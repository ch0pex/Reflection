add_subdirectory(non-reflected)
add_subdirectory(reflected)

function(add_sim_executable name)
    add_executable(${name} main.cpp)
    target_link_libraries(${name} PRIVATE sim::${name}-lib)
endfunction()

add_sim_executable(non-reflected-aos)
add_sim_executable(non-reflected-soa)
add_sim_executable(reflected-aos-soa)
#add_sim_executable(reflected-mixed)
