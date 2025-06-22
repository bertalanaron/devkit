#include <fstream>
#include <filesystem>

#include <devkit/io/asset_manager.h>
#include <gtest/gtest.h>


struct Text {
	std::string value;

	static Text load(const char* path) {
		std::ifstream ifs(path);
		if (ifs.is_open())
			throw std::runtime_error("");

		std::stringstream buffer;
		buffer << ifs.rdbuf();

		return Text{ .value = buffer.str() };
	}

	void update(const char* path) {
		*this = load(path);
	}
};

void clearDirectory(const std::string& path) {
	namespace fs = std::filesystem;
	if (!fs::exists(path)) return;
	for (const auto& entry : fs::directory_iterator(path)) {
		fs::remove_all(entry.path());
	}
}

void createFileWithTextContent(const std::string& path, const std::string& value) {
	std::filesystem::create_directories(std::filesystem::path(path).parent_path());

	std::ofstream ofs(path);
	if (!ofs.is_open())
		throw std::runtime_error("Could not create file: " + path);
	ofs << value;
}


namespace io_tests {

TEST(AssetManager, smoke) {
	using namespace dk;

	const char* helloTxtContents = "Hello World!";
	clearDirectory("test/");
	createFileWithTextContent("test/hello.txt", helloTxtContents);

	ASSERT_NO_THROW(
		io::AssetManager assetManager;

		assetManager.type<Text>("txt", Text::load, &Text::update);
		assetManager.loadFrom("test/");

		Text& text = assetManager.get<Text>("test/hello");
		ASSERT_EQ(text.value, helloTxtContents);
	);
}

}
