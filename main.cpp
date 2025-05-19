#include <algorithm>
#include <bitset>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

constexpr char formatFileName[] = "format.txt";
constexpr uint32_t toBigEndian(const uint32_t value) {
	return
		((value & 0x000000FF) << 24) |
		((value & 0x0000FF00) << 8) |
		((value & 0x00FF0000) >> 8) |
		((value & 0xFF000000) >> 24);
}
constexpr uint16_t hMapTitleLength = 16;
constexpr uint16_t hMapDescriptionLength = 200;

struct HMapInfo {
	uint8_t version;
	uint8_t difficulty;
	uint8_t size;
	std::bitset<6> colors;
	std::bitset<6> humans;
	std::bitset<6> computers;
	uint8_t victoryCondition;
	uint8_t lossCondition;
	std::string title;
	std::string description;
};

uint8_t get8(std::ifstream& file) {
	uint8_t value;
	file.read(reinterpret_cast<char*>(&value), sizeof(value));
	return value;
}

uint16_t getLE16(std::ifstream& file) {
	uint16_t value;
	file.read(reinterpret_cast<char*>(&value), sizeof(value));
	return value;
}

uint32_t getBE32(std::ifstream& file) {
	uint32_t value;
	file.read(reinterpret_cast<char*>(&value), sizeof(value));
	return toBigEndian(value);
}

std::string getString(std::ifstream& file, const size_t length) {
	std::string content;
	content.resize(length);
	for (size_t i = 0; i < length; i++) {
		char ch = ' ';
		file.read(&ch, sizeof(char));
		content[i] = ch;
		if (ch == '\0') {
			content.resize(i);
			break;
		}
	}
	return content;
}

void moveTo(std::ifstream& file, const size_t pos) {
	file.seekg(pos, std::ios::beg);
}

uint8_t getVersion(const std::string& filePath) {
	const size_t pos = filePath.find(".");
	if (pos == std::string::npos)
		throw std::runtime_error("Invalid filename, no extension.");

	std::string fileExtension = filePath.substr(pos + 1);
	std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), [](unsigned char ch){
		return std::tolower(ch);
	});

	if (fileExtension == "mp2")
		return 0;
	if (fileExtension == "mpx")
		return 1;
	if (fileExtension == "hxc")
		return 2;
	if (fileExtension == "fh2m")
		return 3;	
	else
		throw std::runtime_error("Invalid file extension.");
}

uint8_t getMapSize(const uint8_t mapWidth) {
	switch (mapWidth) {
		case 36:
			return 0;
		case 72:
			return 1;
		case 108:
			return 2;
		case 144:
			return 3;
		default:
			throw std::runtime_error("Invalid map size.");
	}
}

std::bitset<6> getColors(std::ifstream& file) {
	std::bitset<6> colors;
	for (uint8_t i = 0; i < 6; i++)
		colors[i] = get8(file) == 1;
	return colors;
}

HMapInfo readHMap(std::ifstream& file, const std::string& filePath) {
	constexpr uint32_t signature = 0x5C000000;
	if (getBE32(file) != signature)
		throw std::runtime_error("File signature is incorrect.");

	HMapInfo hMapInfo;



	hMapInfo.difficulty = getLE16(file);
	hMapInfo.size = getMapSize(get8(file));

	get8(file); // skipping height

	hMapInfo.colors = getColors(file);
	hMapInfo.humans = getColors(file);
	hMapInfo.computers = getColors(file);

	hMapInfo.victoryCondition = get8(file);
	moveTo(file, 35); // skipping other stuff
	hMapInfo.lossCondition = get8(file);

	moveTo(file, 58);
	hMapInfo.title = getString(file, hMapTitleLength);
	moveTo(file, 118);
	hMapInfo.description = getString(file, hMapDescriptionLength);

	return hMapInfo;
}

std::string colorsToString(const std::bitset<6> colors) {
	std::string colorsString = "[";
	colorsString.reserve(14);
	for (uint8_t i = 0; i < 6; i++) {
		colorsString += colors[i] ? "1" : "0";
		if (i < 5)
			colorsString += ",";
	}
	colorsString += "]";
	return colorsString;
}

void replaceProperty(std::string& content, const std::string& toReplace, const std::string& replacement) {
	while (true) {
		size_t pos = content.find(toReplace);
		if (pos == std::string::npos)
			break;
		content.replace(pos, toReplace.length(), replacement);
	}
}

void displayHMapInfo(const HMapInfo& hMapInfo, const std::string& filePath) {
	std::ifstream formatFile(formatFileName);
	if (!formatFile.is_open())
		throw std::runtime_error("Couldn't open the format.txt file.");

	std::ostringstream buffer;
	buffer << formatFile.rdbuf();
	std::string content = buffer.str();

	replaceProperty(content, "{version}", std::to_string(hMapInfo.version));
	replaceProperty(content, "{difficulty}", std::to_string(hMapInfo.difficulty));
	replaceProperty(content, "{size}", std::to_string(hMapInfo.size));
	replaceProperty(content, "{colors}", colorsToString(hMapInfo.colors));
	replaceProperty(content, "{humans}", colorsToString(hMapInfo.humans));
	replaceProperty(content, "{computers}", colorsToString(hMapInfo.computers));
	replaceProperty(content, "{victoryCondition}", std::to_string(hMapInfo.victoryCondition));
	replaceProperty(content, "{lossCondition}", std::to_string(hMapInfo.lossCondition));
	replaceProperty(content, "{title}", hMapInfo.title);
	replaceProperty(content, "{description}", hMapInfo.description);
	std::cout << content << std::endl;
}

int main(int argc, char const *argv[]) {
	bool isError = false;
	try {
		if (argc < 2)
			throw std::runtime_error("Not enough arguments given.");

		for (int i = 1; i < argc; i++) {
			const std::string filePath = argv[i];
			std::ifstream file(filePath, std::ios::binary);
			if (!file.is_open()) 
				throw std::runtime_error("Couldn't open the map file.");

			HMapInfo hMapInfo = readHMap(file, filePath);
			displayHMapInfo(hMapInfo, filePath);
		}

	} catch (const std::exception& error) {
		isError = true;
		std::cerr << error.what() << std::endl;
	}

	std::cout << "Press enter to exit." << std::endl;
	std::cin.get();
	return isError;
}