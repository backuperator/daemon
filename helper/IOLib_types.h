/**
 * Various types for the IOLib
 */
#ifndef IOLIB_TYPES_H
#define IOLIB_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

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

/////////////////////////////// Type Definitions ///////////////////////////////
/**
 * An error code in IOLib. This type will be zero if no error occurred with the
 * operation, negative if the error is internal to the IOLib, or positive if
 * the error is a system error. (In the case of a system error, the error code
 * should mirror errno.)
 */
typedef int iolib_error_t;

/// Indicates that the end of the media has been reached.
#define IOLIB_ERROR_EOM 	-90000

/**
 * IOLib strings; these are really just char * pointers, but have a special type
 * to indicate that they belong to the IOLib, and must be freed when the caller
 * is done using them.
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

    // Undefined other status
    kDriveStatusOther
} iolib_drive_operation_t;

/**
 * A generalized status structure for a tape drive. Some data may be very
 * specific to the physical drive itself, but we make an attempt to provide at
 * least relatively coarse generalized information.
 */
typedef struct {
    // Device status register
    iolib_drive_operation_t deviceStatus;
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
 * Types of storage elements that a loader may have.
 */
typedef enum {
	// Medium transport element (picker)
	kStorageElementTransport = (1 << 0),
	// Storage element (slot)
	kStorageElementSlot = (1 << 1),
	// Import/export element (portal) - mailslots fall in this category
	kStorageElementPortal = (1 << 2),
	// Data transfer element (drive)
	kStorageElementDrive = (1 << 3),

	// Any type of storage element
	kStorageElementAny = (kStorageElementTransport | kStorageElementSlot |
						  kStorageElementPortal | kStorageElementDrive)
} iolib_storage_element_type_t;

/**
 * Various flags that describe a storage element.
 */
typedef enum {
    /// No flags (this is mostly there to make C++ shut the fuck up)
    kStorageElementNoFlags = 0,

	/// The element contains a tape.
	kStorageElementFull = (1 << 0),
	/// The medium was inserted by the operator (i.e. mailslot)
	kStorageElementPlacedByOperator = (1 << 1),
	/// The barcode on the medium could not be read.
	kStorageElementInvalidLabel = (1 << 2),
	/// Medium can be accessed by the picker
	kStorageElementAccessible = (1 << 3),

	/// Element supports medium exporting
	kStorageElementSupportsExport = (1 << 8),
	/// Element supports importing
	kStorageElementSupportsImport = (1 << 9)
} iolib_storage_element_flags_t;

#ifdef __cplusplus
inline iolib_storage_element_flags_t operator|(iolib_storage_element_flags_t a, iolib_storage_element_flags_t b) {
    return static_cast<iolib_storage_element_flags_t>(static_cast<int>(a) | static_cast<int>(b));
}
inline iolib_storage_element_flags_t operator&(iolib_storage_element_flags_t a, iolib_storage_element_flags_t b) {
    return static_cast<iolib_storage_element_flags_t>(static_cast<int>(a) & static_cast<int>(b));
}
#endif

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
    iolib_string_t name;
    // Location of this library, such as "SCSI0:2" or "SAS500277a4100c4e21"
    iolib_string_t id;

    // Number of tape drives in this library
    size_t numDrives;
    // Pointer to drive objects
    iolib_drive_t drives[IOLIB_LIBRARY_MAX_DRIVES];

    // Number of loaders in the drive
    size_t numLoaders;
    // Pointer to loader objects
    iolib_loader_t loaders[IOLIB_LIBRARY_MAX_LOADERS];
} iolib_library_t;

#endif
