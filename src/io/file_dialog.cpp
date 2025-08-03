#include <devkit/io/file_dialog.h>

#include <nfd.h>

template <typename Args>
    requires requires(Args args) {
        { args.filterList };
        { args.filterCount };
    }
void tryApplyFilters(Args& args, std::optional<std::vector<std::pair<std::string, std::string>>>& opt_filters)
{
    using Filter = std::remove_const_t<std::remove_pointer_t<std::decay_t<decltype(args.filterList)>>>;

    std::vector<Filter> filters;
    if (!opt_filters.has_value())
        return;

    for (const auto& filter : opt_filters.value())
        filters.emplace_back(Filter{ filter.first.data(), filter.second.data() });

    args.filterList  = filters.data();
    args.filterCount = filters.size();
}

template <typename Args>
    requires requires(Args args) {
        { args.defaultPath };
    }
void trySetDefaultPath(Args& args, std::optional<std::string>& opt_defaultPath)
{
    if (!opt_defaultPath.has_value())
        return;

    args.defaultPath = opt_defaultPath.value().data();
}

bool isResultValid(const nfdresult_t& result)
{
    if (result == NFD_OKAY)
        return true;

    if (result == NFD_CANCEL)
        return false;

    spdlog::error("NativeFileDialog error: {}", NFD_GetError());
    std::terminate();
}

std::optional<std::filesystem::path> tryGetPath(const nfdresult_t& status, nfdu8char_t* result)
{
    if (isResultValid(status))
    {
        std::filesystem::path path = result;
        NFD_FreePathU8(result);
        return path;
    }
    else {
        if (result)
            NFD_FreePathU8(result);
        return std::nullopt;
    }
}

std::optional<std::filesystem::path> dk::io::FileDialog::openFile(
    std::optional<std::string>           opt_defaultPath, 
    std::optional<std::vector<filter_t>> opt_filters)
{
    initialize();

    // Setup arguments
    nfdopendialogu8args_t args = {0};
    trySetDefaultPath(args, opt_defaultPath);
    tryApplyFilters(args, opt_filters);

    // Execute and validate
    nfdu8char_t* path   = nullptr;
    nfdresult_t  status = NFD_OpenDialogU8_With(&path, &args);
    return tryGetPath(status, path);
}

std::optional<std::filesystem::path> dk::io::FileDialog::selectFolder(
    std::optional<std::string> opt_defaultPath)
{
    initialize();

    // Setup arguments
    nfdpickfolderu8args_t args = { 0 };
    trySetDefaultPath(args, opt_defaultPath);

    // Execute and validate
    nfdu8char_t* path   = nullptr;
    nfdresult_t  status = NFD_PickFolderU8_With(&path, &args);
    return tryGetPath(status, path);
}

std::optional<std::filesystem::path> dk::io::FileDialog::saveFile(
    std::optional<std::string>           opt_defaultPath, 
    std::optional<std::vector<filter_t>> opt_filters)
{
    initialize();

    // Setup arguments
    nfdsavedialogu8args_t args = {0};
    trySetDefaultPath(args, opt_defaultPath);
    tryApplyFilters(args, opt_filters);

    // Execute and validate
    nfdu8char_t* path   = nullptr;
    nfdresult_t  status = NFD_SaveDialogU8_With(&path, &args);
    return tryGetPath(status, path);
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
