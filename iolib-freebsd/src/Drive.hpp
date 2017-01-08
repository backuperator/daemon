/**
 * Tape drive handler class
 */
#ifndef DRIVE_HPP
#define DRIVE_HPP

namespace iolibbsd {

class Drive {
	public:
		Drive(const char *, const char *);
		~Drive();

	private:
		const char *devSa, *devPass;

};

} // namespace iolibbsd

#endif
