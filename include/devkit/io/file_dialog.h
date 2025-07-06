#pragma once
#include <devkit/common/utils.h>

namespace dk::io {

class FileDialog
	: private common::SingletonBase<FileDialog>
{
public:
	using filter_t = std::pair<std::string, std::string>;

	static std::vector<filter_t> makeFilters(const std::same_as<filter_t> auto&... filters)
	{ return std::vector<filter_t>{ filters... }; }

	// @param opt_filters - optional filters in { name, extensions } pairs. 
	// extensions is a list of file extensions separated by a comma (e.g.: "c,cpp,cc")
	// 
	// @returns the path selected by the user, or nullopt if the user canceled
	static std::optional<std::filesystem::path> openFile(
		std::optional<std::string>           opt_defaultPath = std::nullopt, 
		std::optional<std::vector<filter_t>> opt_filters     = std::nullopt);

	// @returns the path selected by the user, or nullopt if the user canceled
	static std::optional<std::filesystem::path> selectFolder(
		std::optional<std::string>           opt_defaultPath = std::nullopt);

private:
	FileDialog();

	static void initialize();

	static bool validate(void* result);

	friend common::SingletonBase<FileDialog>;
};

} // dk::io
