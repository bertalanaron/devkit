#include <devkit/io/file_dialog.h>

#include <nfd.h>

std::optional<std::filesystem::path> dk::io::FileDialog::openFile(
    std::optional<std::string>           opt_defaultPath, 
    std::optional<std::vector<filter_t>> opt_filters)
{
    initialize();

    nfdu8char_t*          outPath = nullptr;
    nfdopendialogu8args_t args    = {0};

    // Set default path
    if (opt_defaultPath.has_value())
        args.defaultPath = opt_defaultPath.value().data();

    // Add filters
    std::vector<nfdu8filteritem_t> filters;
    if (opt_filters.has_value()) {
        
        for (const auto& filter : opt_filters.value())
            filters.push_back(nfdu8filteritem_t{ filter.first.data(), filter.second.data() });

        args.filterList = filters.data();
        args.filterCount = filters.size();
    }

    // Get result
    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);

    // Validate
    if (validate(&result))
    {
        std::filesystem::path result = outPath;
        NFD_FreePathU8(outPath);
        return result;
    }
    else
        return std::nullopt;
}

std::optional<std::filesystem::path> dk::io::FileDialog::selectFolder(
    std::optional<std::string> opt_defaultPath)
{
    initialize();

    nfdu8char_t*          outPath = nullptr;
    nfdpickfolderu8args_t args    = {0};

    // Set default path
    if (opt_defaultPath.has_value())
        args.defaultPath = opt_defaultPath.value().data();

    nfdresult_t result = NFD_PickFolderU8_With(&outPath, &args);

    // Validate
    if (validate(&result))
    {
        std::filesystem::path result = outPath;
        NFD_FreePathU8(outPath);
        return result;
    }
    else
        return std::nullopt;
}

dk::io::FileDialog::FileDialog()
    : SingletonBase()
{
    NFD_Init();
}

void dk::io::FileDialog::initialize()
{
    FileDialog::instance();
}

bool dk::io::FileDialog::validate(void* result)
{
    if (*reinterpret_cast<nfdresult_t*>(result) == NFD_OKAY)
    {
        return true;
    }
    else if (*reinterpret_cast<nfdresult_t*>(result) == NFD_CANCEL)
        return false;
    else {
        spdlog::error("NativeFileDialog error: {}", NFD_GetError());
        std::terminate();
    }
}
