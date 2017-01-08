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

		iolib_drive_operation_t getDriveOp();

	private:
		const char *devSa, *devPass;
        int fdSa = -1, fdPass = -1;
        std::atomic_int fdSaRefs, fdPassRefs;


		void _openSa();
		void _closeSa();

		iolib_drive_operation_t _mtioToNativeStatus(short);
};

} // namespace iolibbsd

#endif
