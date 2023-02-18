#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cassert>

class IOHelper
{
public:
	
	static const int searchDepth = 4;
	static inline char assetsFolderName[] = "assets";
	static const int sizeofAssetsFolderName = sizeof(assetsFolderName) / sizeof(char);
	// ex : ../../%assetsFolderName%/
	static std::string assetsFolder;

	static void getAssetsFolder();
};