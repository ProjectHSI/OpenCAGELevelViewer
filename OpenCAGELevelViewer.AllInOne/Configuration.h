#pragma once

#include <cstdint>
#include <type_traits>

namespace OpenCAGELevelViewer {
	namespace AllInOne {
		namespace Configuration {
			constexpr size_t profileNameLength = 64;
			constexpr size_t gamePathLength = 255;
			constexpr size_t profileLength = 5;

			struct Version {
				uint16_t major;
				uint16_t minor;
				uint16_t patch;

				bool operator==(const Version &other) const = default;
			};

			struct Profile {
				bool exists; // data may be garbage otherwise
				//char profileName[profileNameLength];
				char gamePath[gamePathLength];
			};

			struct Configuration {
				Version version;

				float fontSize;
				int vsync;

				Profile profile[profileLength];
				//uintptr_t 
			};

			static_assert(std::is_trivially_constructible < Configuration >::value);
			static_assert(std::is_standard_layout < Configuration >::value);

			extern Configuration configuration;

			void load();
			void save();
		}
	}
}