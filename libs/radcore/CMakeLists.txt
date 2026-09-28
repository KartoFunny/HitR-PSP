set(RADCORE_SOURCES
	"src/raddebugcommunication/targetx.cpp"
	"src/raddebugconsole/consoleclient.cpp"
	"src/raddebug/debug.cpp"
	"src/raddispatch/dispatcher.cpp"
	"src/radfile/common/cementer.cpp"
	"src/radfile/common/drive.cpp"
	"src/radfile/common/drivethread.cpp"
	"src/radfile/common/filecache.cpp"
	"src/radfile/common/file.cpp"
	"src/radfile/common/filesystem.cpp"
	"src/radfile/common/instancedrive.cpp"
	"src/radfile/common/platformdrives.cpp"
	"src/radfile/common/radfile.cpp"
	"src/radfile/common/remotedrive.cpp"
	"src/radfile/common/requests.cpp"
	"src/radkey/radkey.cpp"
	"src/radmemory/align.cpp"
	"src/radmemory/binallocator.cpp"
	"src/radmemory/dlheap.cpp"
	"src/radmemory/externalmemoryheap.cpp"
	"src/radmemory/externalmemoryobject.cpp"
	"src/radmemory/memorymanager.cpp"
	"src/radmemory/memoryobject.cpp"
	"src/radmemory/memorypool.cpp"
	"src/radmemorymonitor/memmonitorclient.cpp"
	"src/radmemorymonitor/memmonitorclienthelp.cpp"
	"src/radmemory/platalloc.cpp"
	"src/radmemory/staticheap.cpp"
	"src/radmemory/trackingheap.cpp"
	"src/radmemory/twowayallocator.cpp"
	"src/radobjectbtree/objectavltree.cpp"
	"src/radobjectbtree/objectbtree.cpp"
	"src/radobjectlist/dynamicarray.cpp"
	"src/radobjectlist/objectlist.cpp"
	"src/radobjectlist/weakcallbackwrapper.cpp"
	"src/radobjectlist/weakinterfacewrapper.cpp"
	"src/radobject/object.cpp"
	"src/radplatform/platform.cpp"
	"src/radprofiler/profiler2.cpp"
	"src/radprofiler/profiler.cpp"
	"src/radprofiler/profilesample.cpp"
	"src/radremotecommand/functionlist.cpp"
	"src/radremotecommand/remotecommandserver.cpp"
	"src/radremotecommand/remotecommandtarget.cpp"
	"src/radstats/simplestat.cpp"
	"src/radstats/statmanager.cpp"
	"src/radstring/string.cpp"
	"src/radtextdisplay/textdisplay.cpp"
	"src/radthread/mutex.cpp"
	"src/radthread/semaphore.cpp"
	"src/radthread/system.cpp"
	"src/radthread/thread.cpp"
	"src/radtime/stopwatch.cpp"
	"src/radtime/time.cpp"
)

# Платформо-специфичные файлы
if(PSP)
	# PSP: пропускаем Windows/SDL-специфичные файлы.
	# Добавляем свои заглушки для memory space и radController.
	list(APPEND RADCORE_SOURCES
		"src/radmemory/memoryspacepsp.cpp"
		"src/psp_controller_stubs.cpp"
	)
elseif(VITA)
	list(APPEND RADCORE_SOURCES
		"src/radfile/vita/vitadrive.cpp"
		"src/radcontroller/sdlcontroller.cpp"
		"src/radmemory/memoryspacewin32.cpp"
		"src/radstacktrace/win32/stacktrace.cpp"
	)
else()
	list(APPEND RADCORE_SOURCES
		"src/radfile/sdl/sdldrive.cpp"
		"src/radcontroller/sdlcontroller.cpp"
		"src/radmemory/memoryspacewin32.cpp"
		"src/radstacktrace/win32/stacktrace.cpp"
	)
endif()

if(SRR2_MICROPROFILE)
	list(APPEND RADCORE_SOURCES "src/radprofiler/microprofile.cpp")
endif()

add_library(radcore STATIC ${RADCORE_SOURCES})

target_include_directories(radcore
	PUBLIC "inc/"
	PRIVATE "src/pch/"
	PUBLIC "${CMAKE_SOURCE_DIR}/code/"
)

# SDL: только для не-PSP платформ
if(NOT PSP)
	target_link_libraries(radcore ${SDL_LIBRARY})
endif()
