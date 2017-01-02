/**
 * A single file in a backup job. This serves as a small encapsulation around
 * its file path, name, and some metadata, and is mostly used to create chunks.
 */
#include <string>
#include <ctime>

#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>

class BackupFile {
	public:
		BackupFile(boost::filesystem::path);
		~BackupFile();

		int fetchMetadata();

	protected:
		boost::filesystem::path path;

	private:
		boost::uuids::uuid uuid;

		bool hasMetadata = false;

		std::time_t last_modified;

		uint32_t mode;
		uint32_t owner, group;

		std::size_t size;
};
