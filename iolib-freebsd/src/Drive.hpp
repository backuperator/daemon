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

		iolib_string_t getDeviceFile();

		iolib_error_t getDriveStatus(iolib_drive_status_t *);
		iolib_drive_operation_t getDriveOp();

		off_t getLogicalBlkPos();
		iolib_error_t seekToLogicalBlkPos(off_t);

		iolib_error_t rewind();
		iolib_error_t eject();

		iolib_error_t writeFileMark();
		iolib_error_t skipFileMark();

		size_t writeTape(void *, size_t, iolib_error_t *);
		size_t readTape(void *, size_t, iolib_error_t *);

	private:
		int saUnitNumber;

		const char *devSa, *devSaCtl, *devPass;
        int fdSa = -1, fdSaCtl = -1, fdPass = -1;
        std::atomic_int fdSaRefs, fdSaCtlRefs, fdPassRefs;

		size_t maxBlockSz;


		void _determineUnitNumber();
		void _createCtrlDevice();
		void _getMaxIOSize();

		void _openSa();
		void _openSaCtl();
		void _closeSa();
		void _closeSaCtl();

		iolib_drive_operation_t _mtioToNativeStatus(short);
};

} // namespace iolibbsd

#endif
