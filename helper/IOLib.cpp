#include "IOLib.h"

#include <glog/logging.h>
#include <dlfcn.h>

static void *lib = NULL;
static void *resolveSymbol(const char *name);

// Define variables for all of the functions.
_iolib_init_t iolibInit;
_iolib_exit_t iolibExit;
_iolib_string_free_t iolibStringFree;

_iolib_enumerate_devices_t iolibEnumerateDevices;
_iolib_enumerate_devices_free_t iolibEnumerateDevicesFree;

_iolib_drive_get_name_t iolibDriveGetName;
_iolib_drive_get_uuid_t iolibDriveGetUuid;
_iolib_drive_get_devfile_t iolibDriveGetDevFile;
_iolib_drive_get_status_t iolibDriveGetStatus;
_iolib_drive_get_position_t iolibDriveGetPosition;
_iolib_drive_set_position_t iolibDriveSeekToPosition;
_iolib_drive_get_op_t iolibDriveGetCurrentOperation;
_iolib_drive_rewind_t iolibDriveRewind;
_iolib_drive_skip_file_t iolibDriveSkipFile;
_iolib_drive_eject_t iolibDriveEject;
_iolib_drive_lock_medium_t iolibDriveLockMedium;
_iolib_drive_write_t iolibDriveWrite;
_iolib_drive_write_filemark_t iolibDriveWriteFileMark;
_iolib_drive_read_t iolibDriveRead;
_iolib_drive_is_at_end_t iolibDriveIsEOM;

_iolib_loader_get_name_t iolibLoaderGetName;
_iolib_loader_get_uuid_t iolibLoaderGetUuid;
_iolib_loader_get_devfile_t iolibLoaderGetDevFile;
_iolib_loader_get_num_elements_t iolibLoaderGetNumElements;
_iolib_loader_do_inventory_t iolibLoaderPerformInventory;
_iolib_loader_move_t iolibLoaderMove;
_iolib_loader_exchange_t iolibLoaderExchange;
_iolib_loader_get_elements_t iolibLoaderGetElements;

_iolib_element_get_address_t iolibElementGetAddress;
_iolib_element_get_uuid_t iolibElementGetUuid;
_iolib_element_get_flags_t iolibElementGetFlags;
_iolib_element_get_label_t iolibElementGetLabel;
_iolib_element_get_type_t iolibElementGetType;

_iolib_open_session_t iolibOpenSession;
_iolib_close_session_t iolibCloseSession;

// Generates an entry in the symbol loading table
#define IOLIB_RESOLVE_FUNC(name) *((void **) &name) = resolveSymbol(#name)

/**
 * Initializes the library function pointers by loading the library. This will
 * raise an exception if it cannot find the library/symbols cannot be resolved.
 */
void iolibLoadLib() {
    // Load the library
    lib = dlopen("./iolib.so", RTLD_NOW);
    CHECK(lib != NULL) << "Could not load iolib.so: " << dlerror();

    // Attempt to resolve functions.
    IOLIB_RESOLVE_FUNC(iolibInit);
    IOLIB_RESOLVE_FUNC(iolibExit);
    IOLIB_RESOLVE_FUNC(iolibStringFree);

    IOLIB_RESOLVE_FUNC(iolibEnumerateDevices);
    IOLIB_RESOLVE_FUNC(iolibEnumerateDevicesFree);

    IOLIB_RESOLVE_FUNC(iolibDriveGetName);
	IOLIB_RESOLVE_FUNC(iolibDriveGetUuid);
	IOLIB_RESOLVE_FUNC(iolibDriveGetDevFile);
    IOLIB_RESOLVE_FUNC(iolibDriveGetStatus);
    IOLIB_RESOLVE_FUNC(iolibDriveGetPosition);
    IOLIB_RESOLVE_FUNC(iolibDriveSeekToPosition);
    IOLIB_RESOLVE_FUNC(iolibDriveGetCurrentOperation);
    IOLIB_RESOLVE_FUNC(iolibDriveRewind);
    IOLIB_RESOLVE_FUNC(iolibDriveSkipFile);
    IOLIB_RESOLVE_FUNC(iolibDriveEject);
    IOLIB_RESOLVE_FUNC(iolibDriveLockMedium);
    IOLIB_RESOLVE_FUNC(iolibDriveWrite);
    IOLIB_RESOLVE_FUNC(iolibDriveWriteFileMark);
    IOLIB_RESOLVE_FUNC(iolibDriveRead);
    IOLIB_RESOLVE_FUNC(iolibDriveIsEOM);

    IOLIB_RESOLVE_FUNC(iolibLoaderGetName);
	IOLIB_RESOLVE_FUNC(iolibLoaderGetUuid);
	IOLIB_RESOLVE_FUNC(iolibLoaderGetDevFile);
    IOLIB_RESOLVE_FUNC(iolibLoaderGetNumElements);
    IOLIB_RESOLVE_FUNC(iolibLoaderPerformInventory);
    IOLIB_RESOLVE_FUNC(iolibLoaderMove);
    IOLIB_RESOLVE_FUNC(iolibLoaderExchange);
    IOLIB_RESOLVE_FUNC(iolibLoaderGetElements);

    IOLIB_RESOLVE_FUNC(iolibElementGetAddress);
	IOLIB_RESOLVE_FUNC(iolibElementGetUuid);
    IOLIB_RESOLVE_FUNC(iolibElementGetFlags);
    IOLIB_RESOLVE_FUNC(iolibElementGetLabel);
	IOLIB_RESOLVE_FUNC(iolibElementGetType);

    IOLIB_RESOLVE_FUNC(iolibOpenSession);
    IOLIB_RESOLVE_FUNC(iolibCloseSession);
}

/**
 * Attempts to fetch the address of the specified symbol.
 */
static void *resolveSymbol(const char *name) {
    void *ptr = dlsym(lib, name);
    CHECK(ptr != NULL) << "Couldn't resolve " << name << ": " << dlerror();

    return ptr;
}
