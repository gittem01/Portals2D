#include <IOHelper.h>

std::string IOHelper::assetsFolder;

void IOHelper::getAssetsFolder()
{
	if (searchDepth < 1)
	{
		assert("number of trials is smaller than 1");
	}

	char pathString[3 * searchDepth + 1 + sizeofAssetsFolderName];

	for (int currentTry = 0; currentTry < searchDepth; currentTry++)
	{
		int pathStringSize = currentTry * 3 + sizeofAssetsFolderName;
		pathString[pathStringSize] = '\0';

		for (int i = 0; i < currentTry; i++)
		{
			pathString[i * 3 + 0] = '.';
			pathString[i * 3 + 1] = '.';
			pathString[i * 3 + 2] = '/';
		}

		for (int i = 0; i < sizeofAssetsFolderName - 1; i++)
		{
			pathString[currentTry * 3 + i] = assetsFolderName[i];
		}
		pathString[currentTry * 3 + sizeofAssetsFolderName - 1] = '/';

		if (std::filesystem::exists(pathString))
		{
			assetsFolder = std::string(pathString, pathStringSize);
			return;
		}
	}

	printf("Could not find assets folder");
	assert(false);
}