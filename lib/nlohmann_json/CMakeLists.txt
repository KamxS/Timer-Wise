message(STATUS "Fetching Json Lib")

set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(json)