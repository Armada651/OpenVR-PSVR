project('psvr_driver', 'cpp')

libusb = dependency('libusb-1.0')
threads = dependency('threads')
deps = [
    libusb,
    threads,
]

sources = [
    'driver_psvr/DeviceDriver.cpp',
    'driver_psvr/DeviceDriver.h',
    'driver_psvr/DriverLog.cpp',
    'driver_psvr/DriverLog.h',
    'driver_psvr/PSVR.h',
    'driver_psvr/driver_psvr.cpp',
    'driver_psvr/ServerDriver.cpp',
    'driver_psvr/ServerDriver.h',
    'driver_psvr/Watchdog.cpp',
    'driver_psvr/Watchdog.h',
    'MadgwickAHRS/MadgwickAHRS.cpp',
    'MadgwickAHRS/MadgwickAHRS.h',
]
includes = include_directories([
    'driver_psvr/',
    'MadgwickAHRS/',
    'openvr/headers/',
])

shared_library('coucou', sources, include_directories: includes, dependencies: deps)
