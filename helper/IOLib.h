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

#include <stdint.h>
#include <stddef.h>

#define IOLIB_EXTERN extern

/////////////////////////////////// Tuneables //////////////////////////////////
/**
 * GENERAL NOTE ON TUNEABLES:
 *
 * After changing any of these tuneables, the library intended to be used must
 * also be re-compiled, as struct sizes will change. There's no mechanisms in
 * place to detect this - a library with tuneables different than what is
 * specified here will simply fail in rather spectacular ways.
 */

/**
 * Maximum number of drives per library; this defines the size of the drive
 * pointer array.
 */
#define IOLIB_LIBRARY_MAX_DRIVES        16
/**
 * Maximum number of loaders per library.
 */
#define IOLIB_LIBRARY_MAX_LOADERS       4
/**
 * Maximum number of storage elements per library. Note that each storage
 * element usually corresponds to a magazine, and holds a number of tapes.
 */
#define IOLIB_LIBRARY_MAX_ELEMENTS      8


/////////////////////////////// Type Definitions ///////////////////////////////
/**
 * An error code in IOLib. This type will be zero if no error occurred with the
 * operation, negative if the error is internal to the IOLib, or positive if
 * the error is a system error. (In the case of a system error, the error code
 * should mirror errno.)
 */
typedef int iolib_error_t;

/**
 * IOLib strings; these are really just char * pointers, but have a special type
 * to indicate that they belong to the IOLib, and must be freed using it.
 */
typedef char* iolib_string_t;

/**
 * Opaque type for a session.
 */
typedef void* iolib_session_t;

/**
 * Type for the opaque pointer to a drive object. The object should only be
 * manipulated through methods in the IOLib.
 */
typedef void* iolib_drive_t;

/**
 * A list of several operations that a tape drive could be performing at a given
 * time. This list is definitely non-exhaustive.
 */
typedef enum {
    kDriveStatusUnknown = -1,
    /// Doing absolutely nothing
    kDriveStatusIdle,

    /// Writing user-supplied data
    kDriveStatusWritingData,
    /// Writing metadata, such as file marks
    kDriveStatusWritingMetadata,
    /// Erasing
    kDriveStatusErasing,
    /// Reading data from tape
    kDriveStatusReading,

    /// Seeking forwards
    kDriveStatusSeekingForwards,
    /// Seeking backwards
    kDriveStatusSeekingBackwards,
    /// Full speed rewind
    kDriveStatusRewinding,

    /// Retensioning
    kDriveStatusRetensioning,
    /// Loading a new tape into the drive
    kDriveStatusLoading,
    /// Unloading a tape currently in the drive
    kDriveStatusUnloading,
} iolib_drive_operation_t;

/**
 * A generalized status structure for a tape drive. Some data may be very
 * specific to the physical drive itself, but we make an attempt to provide at
 * least relatively coarse generalized information.
 */
typedef struct {
    // Device status register
    uint16_t deviceStatus;
    // Error register
    uint16_t deviceError;


    // Total number of bytes written to the drive
    size_t bytesWritten;
    // Number of bytes written that resulted in an error condition
    size_t bytesWrittenError;
    // Total number of bytes read from drive
    size_t bytesRead;
    // Number of bytes read that resulted in an error condition
    size_t bytesReadError;
} iolib_drive_status_t;

/**
 * Opaque pointer to a loader object. This object should only be manipulated
 * through the methods in the IOLib.
 */
typedef void* iolib_loader_t;

/**
 * Opaque pointer to a storage element object. The object should only be
 * manipulated via the methods in the IOLib.
 */
typedef void* iolib_storage_element_t;

/**
 * A library is the device that the IOLib will enumerate. It doesn't correspond
 * exactly to physical devices, but instead serves as a convenient 'wrapper'
 * around physical devices.
 *
 * Physical devices are 'abstracted' away by exposing the concept of a tape
 * library - even if the devices aren't physically a library. Each library has
 * associated with it one or more drives, and zero or more loaders, as well as
 * zero or more storage elements. (For this purpose, drives are considered as
 * adding a single storage element - their tape slot - to the system.) Storage
 * elements can contain one or more tapes, and each tape can have some metadata
 * associated with it, such as an identifier (i.e. a barcode,) type, raw
 * capacity, and so forth.
 *
 * For example, a standalone desktop tape drive would be considered to have one
 * drive, no loaders, and no storage elements. However, a system such as the
 * HP Autoloader 1/8 G2 would have one drive, one loader, and eight storage
 * elements.
 */
typedef struct {
    // A descriptive name for the library, if available
    const iolib_string_t name;
    // Location of this library, such as "SCSI0:2" or "SAS500277a4100c4e21"
    const iolib_string_t location;

    // Number of tape drives in this library
    size_t numDrives;
    // Pointer to drive objects
    iolib_drive_t* drives[IOLIB_LIBRARY_MAX_DRIVES];

    // Number of loaders in the drive
    size_t numLoaders;
    // Pointer to loader objects
    iolib_loader_t* loaders[IOLIB_LIBRARY_MAX_LOADERS];

    // Number of storage elements
    size_t numStorageElements;
    // Pointer to storage element objects
    iolib_storage_element_t *elements[IOLIB_LIBRARY_MAX_ELEMENTS];
} iolib_library_t;


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
typedef int (*_iolib_enumerate_devices_t)(iolib_library_t *, int, int *);
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
 * Writes the status of the specified tape drive into the supplied buffer.
 */
typedef iolib_error_t (*_iolib_drive_get_status_t)(iolib_drive_t, iolib_drive_status_t *);
IOLIB_EXTERN _iolib_drive_get_status_t iolibDriveGetStatus;

/**
 * Places the drive's current logical block position in the specified variable.
 */
typedef iolib_error_t (*_iolib_drive_get_position_t)(iolib_drive_t, size_t *);
IOLIB_EXTERN _iolib_drive_get_position_t iolibDriveGetPosition;

/**
 * Seeks the drive to the specified logical block position. The drive must NOT
 * be pre-occupied performing any other operation.
 */
typedef iolib_error_t (*_iolib_drive_set_position_t)(iolib_drive_t, size_t);
IOLIB_EXTERN _iolib_drive_set_position_t iolibDriveSeekToPosition;

/**
 * Determines the drive's current operation, if such information is currently
 * available from the drive.
 */
typedef iolib_error_t (*_iolib_drive_get_op_t)(iolib_drive_t, iolib_drive_operation_t *);
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

/////////////////////////////// Loader Handling ////////////////////////////////


/////////////////////////// Storage Element Handling ///////////////////////////


//////////////////////////////// Tape Handling /////////////////////////////////


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
