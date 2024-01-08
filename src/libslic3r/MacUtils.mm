#import "ProfilesSharingUtils.hpp"

#import <Foundation/Foundation.h>

namespace Slic3r {

// ProfilesSharingUtils.hpp
std::string GetDataDir()
{
	NSURL* url = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory
												 inDomain:NSUserDomainMask
												 appropriateForURL:nil create:NO error:nil];

	return std::string([(CFStringRef)url.path UTF8String]);
}

}
