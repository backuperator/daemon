/**
 * Tape drive handler class
 */
#ifndef DRIVE_HPP
#define DRIVE_HPP

#include <fcntl.h>

#include <IOLib_types.h>

namespace iolibbsd {

class Drive {
	public:
		Drive(const char *, const char *);
		~Drive();

		iolib_error_t getDriveStatus(iolib_drive_status_t *);
		iolib_drive_operation_t getDriveOp();

		off_t getLogicalBlkPos();
		iolib_error_t seekToLogicalBlkPos(off_t);

		iolib_error_t rewind();
		iolib_error_t eject();

		iolib_error_t writeFileMark();

	private:
		const char *devSa, *devPass;
        int fdSa = -1, fdPass = -1;
        std::atomic_int fdSaRefs, fdPassRefs;

		size_t maxBlockSz;


		void _determineMaxIOSize();
		void _openSa();
		void _closeSa();

		iolib_drive_operation_t _mtioToNativeStatus(short);
};

} // namespace iolibbsd

#endif
