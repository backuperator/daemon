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
 * Opaque type for a session.
 */
typedef void* iolib_session_t;

/**
 * Type for the opaque pointer to a drive object. The object should only be
 * manipulated through methods in the IOLib.
 */
typedef void* iolib_drive_t;

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
    char *name;
    // Location of this library, such as "SCSI0:2" or "SAS500277a4100c4e21"
    char *location;

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
 * Global library initializer.
 *
 * Returns 0 if successful, an error code otherwise.
 */
typedef int (*_iolib_init_t)(void);
IOLIB_EXTERN _iolib_init_t iolib_init;

/**
 * Global library destructor. This can be used by the library to close any
 * residual file handles, relinquish locks held on devices, and so forth. It is
 * guaranteed to be called before the program exits normally.
 *
 * Returns 0 if successful, an error code otherwise.
 */
typedef int (*_iolib_exit_t)(void);
IOLIB_EXTERN _iolib_exit_t iolib_exit;


///////////////////////////// Hardware Enumeration /////////////////////////////
/**
 * Enumerates the tape libraries found in the system by the library. This
 * function should be called with the location of a buffer into which the
 * library structs should be written.
 *
 * Returns a positive number (including zero) up to `max` number of libraries
 * that were found in the system. If an error occurs, a negative number is
 * returned, and the error value is written in the optional int pointer.
 */
typedef int (*_iolib_enumerate_devices_t)(iolib_library_t *, int, int *);
IOLIB_EXTERN _iolib_enumerate_devices_t iolib_enumerate_devices;


/////////////////////////////// Session Handling ///////////////////////////////
/**
 * Opens a session on the given library. This attempts to acquire a lock on the
 * underlying devices, preventing other programs from using them. I/O can then
 * be performed against the drives, and the loader may be controlled.
 *
 * Returns a non-NULL value (which must be passed to all future calls to
 * identify this session) if successful, NULL otherwise. Any errors will be set
 * in the optional int pointer. paramter.
 */
typedef iolib_session_t (*_iolib_open_session_t)(iolib_library_t *, int *);
IOLIB_EXTERN _iolib_open_session_t iolib_open_session;

/**
 * Closes a previously opened session; this will free any memory allocated,
 * relinquish locks on devices, and stops any ongoing I/O.
 *
 * Returns 0 if successful, an error code otherwise.
 */
typedef int (*_iolib_close_session_t)(iolib_session_t *);
IOLIB_EXTERN _iolib_close_session_t iolib_close_session;



#ifdef __cplusplus
}
#endif // #ifdef __cplusplus
#endif // #ifndef IOLIB_H
