#include <devkit/common/utils.h>

#include <boost/dll.hpp>

std::filesystem::path dk::common::executable_path()
{
    return boost::dll::program_location().string();
}

void nlohmann_extension::smart_dump(const nlohmann::json& j, std::ostream& os, int indent, int indent_step, int threshold)
{
	auto write_indent = [&](int i) {
		for (int k = 0; k < i; ++k) os.put(' ');
		};

	if (j.is_primitive()) {
		os << j.dump();
	}
	else if (j.is_array()) {
		std::string compact = j.dump(-1);
		if ((int)compact.size() <= threshold) {
			os << compact;
		}
		else {
			os << "[\n";
			for (size_t i = 0; i < j.size(); ++i) {
				write_indent(indent + indent_step);
				smart_dump(j[i], os, indent + indent_step, indent_step, threshold);
				if (i + 1 < j.size()) os << ",";
				os << "\n";
			}
			write_indent(indent);
			os << "]";
		}
	}
	else if (j.is_object()) {
		std::string compact = j.dump(-1);
		if ((int)compact.size() <= threshold) {
			os << compact;
		}
		else {
			os << "{\n";
			auto it = j.begin();
			while (it != j.end()) {
				write_indent(indent + indent_step);
				os << "\"" << it.key() << "\": ";
				smart_dump(it.value(), os, indent + indent_step, indent_step, threshold);
				if (++it != j.end()) os << ",";
				os << "\n";
			}
			write_indent(indent);
			os << "}";
		}
	}
}

std::string nlohmann_extension::smart_dump(const nlohmann::json& j, int indent, int indent_step, int threshold)
{
	std::ostringstream oss;
	smart_dump(j, oss, indent, indent_step, threshold);
	return oss.str();
}
