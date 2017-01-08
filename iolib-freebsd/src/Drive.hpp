/**
 * Tape drive handler class
 */
#ifndef DRIVE_HPP
#define DRIVE_HPP

#include <fcntl.h>

namespace iolibbsd {

class Drive {
	public:
		Drive(const char *, const char *);
		~Drive();

	private:
		const char *devSa, *devPass;
        int fdSa = -1, fdPass = -1;
        std::atomic_int fdSaRefs, fdPassRefs;


		void _openSa();
		void _closeSa();
};

} // namespace iolibbsd

#endif
