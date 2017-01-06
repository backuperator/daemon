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


/**
 * Global library initializer.
 *
 * Returns 0 if successful, an error code otherwise.
 */
typedef int (*_iolib_init_t)(void);
extern _iolib_init_t iolib_init;


/**
 * Opens a session on the given library. This attempts to acquire a lock on the
 * underlying devices, preventing other programs from using them. I/O can then
 * be performed against the drives, and the loader may be controlled.
 *
 * Returns a non-NULL value (which must be passed to all future calls to
 * identify this session) if successful, NULL otherwise. Any errors will be set
 * in the optional int pointer. paramter.
 */
typedef int (*_iolib_open_session_t)(iolib_library_t *, int *);
extern _iolib_open_session_t iolib_open_session;


#endif
