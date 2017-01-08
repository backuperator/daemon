#include <IOLib_Types.h>
#include <IOLib_lib.h>

#include <glog/logging.h>

//////////////////////// Initialization and Destructors ////////////////////////
/**
 * Global library initializer.
 */
IOLIB_EXPORT iolib_error_t iolibInit(void) {
    LOG(INFO) << "Initializing iolib-freebsd";

    // TODO: do things here
    return 0;
}

/**
 * Global library destructor. This can be used by the library to close any
 * residual file handles, relinquish locks held on devices, and so forth. It is
 * guaranteed to be called before the program exits normally.
 */
IOLIB_EXPORT iolib_error_t iolibExit(void) {
    LOG(INFO) << "Exiting iolib-freebsd";

    // TODO: do things here
    return 0;
}

/**
 * Frees an IOLib string.
 */
IOLIB_EXPORT void iolibStringFree(iolib_string_t string) {
    free(string);
}

///////////////////////////// Hardware Enumeration /////////////////////////////
/**
 * Enumerates the tape libraries found in the system by the library. This
 * function should be called with the location of a buffer into which the
 * library structs should be written.
 *
 * Returns a positive number (including zero) up to `max` number of libraries
 * that were found in the system. If an error occurs, -1 is returned, and the
 * error value is written in the optional int pointer.
 */
IOLIB_EXPORT int iolibEnumerateDevices(iolib_library_t *lib, int max, iolib_error_t *outErr) {
    // TODO: Implement this
    return -1;
}

/**
 * Frees all library structures previously inserted into the specified array.
 */
IOLIB_EXPORT void iolibEnumerateDevicesFree(iolib_library_t *lib, int num) {
    // TODO: implement
}

//////////////////////////////// Drive Handling ////////////////////////////////
/**
 * Returns a string that describes this tape drive's capabilities. This is just
 * intended for display to the user more than anything.
 */
IOLIB_EXPORT iolib_string_t iolibDriveGetName(iolib_drive_t drive) {
    // TODO: implement
    return NULL;
}

/**
 * Writes the status of the specified tape drive into the supplied buffer.
 */
IOLIB_EXPORT iolib_error_t iolibDriveGetStatus(iolib_drive_t drive, iolib_drive_status_t *outStatus) {
    // TODO: implement
    return -1;
}

/**
 * Places the drive's current logical block position in the specified variable.
 */
IOLIB_EXPORT iolib_error_t iolibDriveGetPosition(iolib_drive_t drive, size_t *outPos) {
    // TODO: implement
    return -1;
}

/**
 * Seeks the drive to the specified logical block position. The drive must NOT
 * be pre-occupied performing any other operation.
 */
IOLIB_EXPORT iolib_error_t iolibDriveSeekToPosition(iolib_drive_t drive, size_t block) {
    // TODO: implement
    return -1;
}

/**
 * Determines the drive's current operation, if such information is currently
 * available from the drive.
 */
IOLIB_EXPORT iolib_error_t iolibDriveGetCurrentOperation(iolib_drive_t drive, iolib_drive_operation_t *outOp) {
    // TODO: implement
    return -1;
}

/**
 * Rewinds the tape to the beginning. This call will block until the rewind
 * operation has completed.
 */
IOLIB_EXPORT iolib_error_t iolibDriveRewind(iolib_drive_t drive) {
    // TODO: implement
    return -1;
}

/**
 * Ejects the tape from the drive. Note that this call should only be used if
 * the drive is idle.
 */
IOLIB_EXPORT iolib_error_t iolibDriveEject(iolib_drive_t drive) {
    // TODO: implement
    return -1;
}

/**
 * Locks the medium in the drive. This is useful during I/O operations that
 * should not be interrupted due to an eject command from an operator control
 * panel on a tape library, or to prevent inadvertent ejection of the tape.
 *
 * To lock the medium, pass true for the second argument; false unlocks it.
 */
IOLIB_EXPORT iolib_error_t iolibDriveLockMedium(iolib_drive_t drive, bool lockFlag) {
    // TODO: implement
    return -1;
}


/**
 * Performs a write operation on the tape, starting at the drive's current
 * logical position. Note that this operation may be split up into smaller write
 * operations to account for the underlying system's capabilities.
 *
 * NOTE: A file mark will only be written at the end of the write if requested.
 * Otherwise, it must be manually written using the iolibDriveWriteFileMark
 * function.
 *
 * Returns the actual number of bytes that were written. If an error occurred,
 * this value is negative; if no data was written, 0 is returned. If less data
 * than was requested was written, the number returned is less than the number
 * requested.
 *
 * Any error codes are supplied through the optional error pointer.
 */
IOLIB_EXPORT size_t iolibDriveWrite(iolib_drive_t drive, void *buf, size_t len,
                                    bool writeFileMark, iolib_error_t *outErr) {
    // TODO: implement
    return -1;
}

/**
 * Writes a file mark to tape at the current logical block position, marking the
 * end of a tape record.
 *
 * NOTE: On many systems, this will actually write two file marks, which
 * indicates the end of tape. The drive is then positioned directly prior to the
 * second file mark. This means that any subsequent (without seeking) writes
 * will overwrite the file mark, while reads will return 0 bytes, but instead
 * raise an "end-of-device" notification.
 */
IOLIB_EXPORT iolib_error_t iolibDriveWriteFileMark(iolib_drive_t drive) {
    // TODO: implement
    return -1;
}

/**
 * Performs a read operation on the tape, starting at the drive's current
 * logical position. Note that the operation may be completed as several smaller
 * blocks, to take into account the underlying system's capabilities.
 *
 * Returns the actual number of bytes that were read. This can be negative if an
 * error occurred, between 0 and (`max` - 1) if a file mark was encountered. If
 * exactly as many bytes as requested were read, one should not assume that
 * more data is available; a file mark may be the next thing read.
 *
 * Any error codes are supplied through the optional error pointer.
 */
IOLIB_EXPORT size_t iolibDriveRead(iolib_drive_t drive, void *buf,
                                   size_t len, iolib_error_t *outErr) {
    // TODO: implement
    return -1;
}

/**
 * Checks whether the drive has encountered the end of the medium (EOM) yet.
 */
IOLIB_EXPORT bool iolibDriveIsEOM(iolib_drive_t drive, iolib_error_t *outErr) {
    // TODO: implement
    return -1;
}


/////////////////////////////// Loader Handling ////////////////////////////////
/**
 * Returns a string that describes this loaders's capabilities. This is just
 * intended for display to the user more than anything.
 */
IOLIB_EXPORT iolib_string_t iolibLoaderGetName(iolib_loader_t loader) {
    // TODO: implement
    return "<<< UNIMPLIMENTED >>>";
}

/**
 * Returns the number of storage elements of the given type that a certain
 * loader has.
 */
IOLIB_EXPORT size_t iolibLoaderGetNumElements(iolib_loader_t loader,
                                              iolib_storage_element_type_t type,
                                              iolib_error_t *outErr) {
    // TODO: implement
    return -1;
}

/**
 * Force the specified loader to perform an inventory of all tapes. This will
 * update the loader's internal status information. If the loader has the
 * capability to read tape labels (i.e. via a barcode label) that will be done.
 */
IOLIB_EXPORT iolib_error_t iolibLoaderPerformInventory(iolib_loader_t loader) {
    // TODO: implement
    return -1;
}

/**
 * Moves the tape in the first storage element to that in the second. This call
 * will block while the move is taking place.
 */
IOLIB_EXPORT iolib_error_t iolibLoaderMove(iolib_loader_t loader,
                                           iolib_storage_element_t src,
                                           iolib_storage_element_t dest) {
    // TODO: implement
    return -1;
}

/**
 * Exchanges the media in the first storage element with that in the second.
 * This call will block while the move is taking place.
 *
 * NOTE: This may not be supported on many loaders.
 */
IOLIB_EXPORT iolib_error_t iolibLoaderExchange(iolib_loader_t loader,
                                               iolib_storage_element_t src,
                                               iolib_storage_element_t dest) {
    // TODO: implement
    return -1;
}

/**
 * Populates an array of storage element structure types with information about
 * n number of storage elements of a given type in the loader. The array
 * specified must be able to hold at least n elements.
 *
 * NOTE: Storage element objects are shared objects, i.e. every invocation will
 * return the same objects. Do not attempt to access/free these elements, as
 * this may cause undefined behavior.
 */
IOLIB_EXPORT iolib_error_t iolibLoaderGetElements(iolib_loader_t loader,
                                                  iolib_storage_element_type_t type,
                                                  iolib_storage_element_t *out,
                                                  size_t outLen) {
    // TODO: implement
    return -1;
}

/////////////////////////// Storage Element Handling ///////////////////////////
/**
 * Returns the logical address of the storage element. This is specific to the
 * loader in use, and how it correlates to a physical slot is undefined.
 */
IOLIB_EXPORT off_t iolibElementGetAddress(iolib_storage_element_t element, iolib_error_t *outErr) {
    // TODO: implement
    return -1;
}

/**
 * Get some flags that describe this storage element. This can be used to
 * determine what this element supports, whether it has media in it, and so
 * forth.
 *
 * NOTE: Flags are logically ORed together.
 */
IOLIB_EXPORT iolib_storage_element_flags_t iolibElementGetFlags(iolib_storage_element_t element, iolib_error_t *outErr) {
    // TODO: implement
    return (iolib_storage_element_flags_t) 0;
}

/**
 * Return the volume label of the specified element, if applicable. If no label
 * is available on the given medium (or the element is empty), NULL is returned.
 */
IOLIB_EXPORT iolib_string_t iolibElementGetLabel(iolib_storage_element_t element) {
    // TODO: Implement this
    return "<<< UNIMPLIMENTED >>>";
}


/////////////////////////////// Session Handling ///////////////////////////////
/**
 * Opens a session on the given library. This attempts to acquire a lock on the
 * underlying devices, preventing other programs from using them. I/O can then
 * be performed against the drives, and the loader may be controlled.
 *
 * Essentially, opening a session sets up all structures and data internally
 * needed to take full control over all devices in a given library.
 *
 * Returns a non-NULL value (which must be passed to all future calls to
 * identify this session) if successful, NULL otherwise. Any errors will be set
 * in the optional pointer.
 */
IOLIB_EXPORT iolib_session_t iolibOpenSession(iolib_library_t *lib, iolib_error_t *outErr) {
    // TODO: Implement this
    return NULL;
}

/**
 * Closes a previously opened session; this will free any memory allocated,
 * relinquish locks on devices, and stops any ongoing I/O.
 */
IOLIB_EXPORT iolib_error_t iolibCloseSession(iolib_session_t session) {
    // TODO: Implement this
    return -1;
}