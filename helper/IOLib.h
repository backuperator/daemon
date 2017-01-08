/**
 * Functions that must be exported by an external, dynamically linked (loaded
 * at runtime) library that provides I/O services to a block-based sequential
 * access device, such as tape.
 *
 * The library is given a chance to initialize its global state at load time,
 * and each "session" with a drive is also explicitly initialized. Calls can
 * expect to be passed a valid context parameter (defined as a void pointer)
 * that the library may use to hold its per-session state to avoid global
 * variables.
 *
 * Several additional "getter" methods can be used to get global information
 * about the IO environment, such as which hardware is present, to what degree
 * parallelism can be accomplished (i.e. if a tape library is connected, how
 * many drives does it have? How many tapes can it hold?). These functions will
 * essentially enumerate whatever hardware is present.
 *
 *
 * NOTE: While this API is structured toward supporting tape drives and loaders,
 * the storage backend must not necessarily be one - virtual tape libraries,
 * or even a raw file backend, can easily be implemented, since the lowest
 * common demoninator is sequential access.
 *
 * NOTE: When dealing with errors, 0 indicates success, a positive value
 * indicates a system error, and a negative number indicates an error internal
 * to the loaded IO library.
 */
#ifndef IOLIB_H
#define IOLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "IOLib_types.h"

//////////////////////// Initialization and Destructors ////////////////////////
/**
 * Initializes the library function pointers by loading the library. This will
 * raise an exception if it cannot find the library/symbols cannot be resolved.
 */
extern "C++" void iolibLoadLib();

/**
 * Global library initializer.
 */
typedef iolib_error_t (*_iolib_init_t)(void);
IOLIB_EXTERN _iolib_init_t iolibInit;

/**
 * Global library destructor. This can be used by the library to close any
 * residual file handles, relinquish locks held on devices, and so forth. It is
 * guaranteed to be called before the program exits normally.
 */
typedef iolib_error_t (*_iolib_exit_t)(void);
IOLIB_EXTERN _iolib_exit_t iolibExit;

/**
 * Frees an IOLib string.
 */
typedef void (*_iolib_string_free_t)(iolib_string_t);
IOLIB_EXTERN _iolib_string_free_t iolibStringFree;


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
typedef int (*_iolib_enumerate_devices_t)(iolib_library_t *, int, iolib_error_t *);
IOLIB_EXTERN _iolib_enumerate_devices_t iolibEnumerateDevices;

/**
 * Frees all library structures previously inserted into the specified array.
 */
typedef void (*_iolib_enumerate_devices_free_t)(iolib_library_t *, int);
IOLIB_EXTERN _iolib_enumerate_devices_free_t iolibEnumerateDevicesFree;

//////////////////////////////// Drive Handling ////////////////////////////////
/**
 * Returns a string that describes this tape drive's capabilities. This is just
 * intended for display to the user more than anything.
 */
typedef iolib_string_t (*_iolib_drive_get_name_t)(iolib_drive_t);
IOLIB_EXTERN _iolib_drive_get_name_t iolibDriveGetName;

/**
 * Gets the drive's status, populating the specified struct.
 */
typedef iolib_error_t (*_iolib_drive_get_status_t)(iolib_drive_t, iolib_drive_status_t *);
IOLIB_EXTERN _iolib_drive_get_status_t iolibDriveGetStatus;

/**
 * Returns the drive's current logical block position.
 */
typedef off_t (*_iolib_drive_get_position_t)(iolib_drive_t, iolib_error_t *);
IOLIB_EXTERN _iolib_drive_get_position_t iolibDriveGetPosition;

/**
 * Seeks the drive to the specified logical block position. The drive must NOT
 * be pre-occupied performing any other operation.
 */
typedef iolib_error_t (*_iolib_drive_set_position_t)(iolib_drive_t, off_t);
IOLIB_EXTERN _iolib_drive_set_position_t iolibDriveSeekToPosition;

/**
 * Determines the drive's current operation, if such information is currently
 * available from the drive.
 */
typedef iolib_drive_operation_t (*_iolib_drive_get_op_t)(iolib_drive_t, iolib_error_t *);
IOLIB_EXTERN _iolib_drive_get_op_t iolibDriveGetCurrentOperation;

/**
 * Rewinds the tape to the beginning. This call will block until the rewind
 * operation has completed.
 */
typedef iolib_error_t (*_iolib_drive_rewind_t)(iolib_drive_t);
IOLIB_EXTERN _iolib_drive_rewind_t iolibDriveRewind;

/**
 * Ejects the tape from the drive. Note that this call should only be used if
 * the drive is idle.
 */
typedef iolib_error_t (*_iolib_drive_eject_t)(iolib_drive_t);
IOLIB_EXTERN _iolib_drive_eject_t iolibDriveEject;

/**
 * Locks the medium in the drive. This is useful during I/O operations that
 * should not be interrupted due to an eject command from an operator control
 * panel on a tape library, or to prevent inadvertent ejection of the tape.
 *
 * To lock the medium, pass true for the second argument; false unlocks it.
 */
typedef iolib_error_t (*_iolib_drive_lock_medium_t)(iolib_drive_t, bool);
IOLIB_EXTERN _iolib_drive_lock_medium_t iolibDriveLockMedium;


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
typedef size_t (*_iolib_drive_write_t)(iolib_drive_t, void *, size_t, bool, iolib_error_t *);
IOLIB_EXTERN _iolib_drive_write_t iolibDriveWrite;

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
typedef iolib_error_t (*_iolib_drive_write_filemark_t)(iolib_drive_t);
IOLIB_EXTERN _iolib_drive_write_filemark_t iolibDriveWriteFileMark;

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
typedef size_t (*_iolib_drive_read_t)(iolib_drive_t, void *, size_t, iolib_error_t *);
IOLIB_EXTERN _iolib_drive_read_t iolibDriveRead;

/**
 * Checks whether the drive has encountered the end of the medium (EOM) yet.
 */
typedef bool (*_iolib_drive_is_at_end_t)(iolib_drive_t, iolib_error_t *);
IOLIB_EXTERN _iolib_drive_is_at_end_t iolibDriveIsEOM;


/////////////////////////////// Loader Handling ////////////////////////////////
/**
 * Returns a string that describes this loaders's capabilities. This is just
 * intended for display to the user more than anything.
 */
typedef iolib_string_t (*_iolib_loader_get_name_t)(iolib_loader_t);
IOLIB_EXTERN _iolib_loader_get_name_t iolibLoaderGetName;

/**
 * Returns the number of storage elements of the given type that a certain
 * loader has.
 */
typedef size_t (*_iolib_loader_get_num_elements_t)(iolib_loader_t, iolib_storage_element_type_t, iolib_error_t *);
IOLIB_EXTERN _iolib_loader_get_num_elements_t iolibLoaderGetNumElements;

/**
 * Force the specified loader to perform an inventory of all tapes. This will
 * update the loader's internal status information. If the loader has the
 * capability to read tape labels (i.e. via a barcode label) that will be done.
 */
typedef iolib_error_t (*_iolib_loader_do_inventory_t)(iolib_loader_t);
IOLIB_EXTERN _iolib_loader_do_inventory_t iolibLoaderPerformInventory;

/**
 * Moves the tape in the first storage element to that in the second. This call
 * will block while the move is taking place.
 */
typedef iolib_error_t (*_iolib_loader_move_t)(iolib_loader_t, iolib_storage_element_t, iolib_storage_element_t);
IOLIB_EXTERN _iolib_loader_move_t iolibLoaderMove;

/**
 * Exchanges the media in the first storage element with that in the second.
 * This call will block while the move is taking place.
 *
 * NOTE: This may not be supported on many loaders.
 */
typedef iolib_error_t (*_iolib_loader_exchange_t)(iolib_loader_t, iolib_storage_element_t, iolib_storage_element_t);
IOLIB_EXTERN _iolib_loader_exchange_t iolibLoaderExchange;

/**
 * Populates an array of storage element structure types with information about
 * n number of storage elements of a given type in the loader. The array
 * specified must be able to hold at least n elements.
 *
 * NOTE: Storage element objects are shared objects, i.e. every invocation will
 * return the same objects. Do not attempt to access/free these elements, as
 * this may cause undefined behavior.
 */
typedef iolib_error_t (*_iolib_loader_get_elements_t)(iolib_loader_t, iolib_storage_element_type_t, iolib_storage_element_t *, size_t);
IOLIB_EXTERN _iolib_loader_get_elements_t iolibLoaderGetElements;

/////////////////////////// Storage Element Handling ///////////////////////////
/**
 * Returns the logical address of the storage element. This is specific to the
 * loader in use, and how it correlates to a physical slot is undefined.
 */
typedef off_t (*_iolib_element_get_address_t)(iolib_storage_element_t, iolib_error_t *);
IOLIB_EXTERN _iolib_element_get_address_t iolibElementGetAddress;

/**
 * Get some flags that describe this storage element. This can be used to
 * determine what this element supports, whether it has media in it, and so
 * forth.
 *
 * NOTE: Flags are logically ORed together.
 */
typedef iolib_storage_element_flags_t (*_iolib_element_get_flags_t)(iolib_storage_element_t, iolib_error_t *);
IOLIB_EXTERN _iolib_element_get_flags_t iolibElementGetFlags;

/**
 * Return the volume label of the specified element, if applicable. If no label
 * is available on the given medium (or the element is empty), NULL is returned.
 */
typedef iolib_string_t (*_iolib_element_get_label_t)(iolib_storage_element_t);
IOLIB_EXTERN _iolib_element_get_label_t iolibElementGetLabel;


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
typedef iolib_session_t (*_iolib_open_session_t)(iolib_library_t *, iolib_error_t *);
IOLIB_EXTERN _iolib_open_session_t iolibOpenSession;

/**
 * Closes a previously opened session; this will free any memory allocated,
 * relinquish locks on devices, and stops any ongoing I/O.
 */
typedef iolib_error_t (*_iolib_close_session_t)(iolib_session_t *);
IOLIB_EXTERN _iolib_close_session_t iolibCloseSession;



#ifdef __cplusplus
}
#endif // #ifdef __cplusplus
#endif // #ifndef IOLIB_H
