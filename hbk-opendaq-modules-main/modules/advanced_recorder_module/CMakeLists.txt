hbk_module()

# I haven't nailed down precisely what's happening, but sometimes there are CMake targets for each
# Boost component - even header-only ones - and they MUST be used to correctly find the headers.
# Other times there are no such targets and they MUST NOT be used. It seems to depend on several
# factors: the Boost version, platform (Windows vs. Linux), and whether or not openDAQ has fetched
# Boost before, i.e. on CMake runs after the first. To handle all cases, we'll just link to the
# targets if and only if they exist. This is a hack until someone smarter than, or with more time
# than me, figures out what's really going on.
set(BOOST_DEPENDENCIES Boost::headers)
if(TARGET Boost::crc)
    list(APPEND BOOST_DEPENDENCIES Boost::crc)
endif()
if(TARGET Boost::endian)
    list(APPEND BOOST_DEPENDENCIES Boost::endian)
endif()
if(TARGET Boost::uuid)
    list(APPEND BOOST_DEPENDENCIES Boost::uuid)
endif()

target_link_libraries(${MODULE_NAME}
    PUBLIC
        ${BOOST_DEPENDENCIES}
)
