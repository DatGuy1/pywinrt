#pragma once

namespace pywinrt
{
    namespace stdfs = std::filesystem;

    inline void write_pybase_h(stdfs::path const& folder)
    {
        writer w;
        write_license(w);
        w.write(strings::pybase);
        w.flush_to_file(folder / "pybase.h");
    }

    inline void write_namespace_h(stdfs::path const& folder, std::string_view const& ns, std::set<std::string> const& needed_namespaces, cache::namespace_members const& members)
    {
        writer w;
        w.current_namespace = ns;

        for (auto&& needed_ns : needed_namespaces)
        {
            w.needed_namespaces.insert(needed_ns);
        }

        auto filename = w.write_temp("py.%.h", ns);

        auto segments = get_dotted_name_segments(ns);

        w.write("\nnamespace py::proj::%\n{", bind_list("::", segments));
        {
            writer::indent_guard g{ w };
            settings.filter.bind_each<write_pinterface_decl>(members.interfaces)(w);
        }
        w.write("}\n");

        w.write("\nnamespace py::impl::%\n{", bind_list("::", segments));
        {
            writer::indent_guard g{ w };
            settings.filter.bind_each<write_delegate_callable_wrapper>(members.delegates)(w);
            settings.filter.bind_each<write_pinterface_impl>(members.interfaces)(w);
        }
        w.write("}\n");

        w.write("\nnamespace py::wrapper::%\n{\n", bind_list("::", segments));
        {
            writer::indent_guard g{ w };
            settings.filter.bind_each<write_python_wrapper_alias>(members.classes)(w);
            settings.filter.bind_each<write_python_wrapper_alias>(members.interfaces)(w);
            settings.filter.bind_each<write_python_wrapper_alias>(members.structs)(w);
        }
        w.write("}\n");

        w.write("\nnamespace py\n{\n");
        {
            writer::indent_guard g{ w };
            settings.filter.bind_each<write_get_python_type_specialization>(members.classes)(w);
            settings.filter.bind_each<write_get_python_type_specialization>(members.interfaces)(w);
            settings.filter.bind_each<write_get_python_type_specialization>(members.structs)(w);
            settings.filter.bind_each<write_pinterface_type_mapper>(members.interfaces)(w);
            settings.filter.bind_each<write_delegate_type_mapper>(members.delegates)(w);
            settings.filter.bind_each<write_struct_converter_decl>(members.structs)(w);
        }
        w.write("}\n");

        w.swap();

        write_license(w);
        {
            auto format = R"(#pragma once

#include "pybase.h"
)";
            w.write(format);
        }

        w.write_each<write_include>(w.needed_namespaces);

        {
            auto format = R"(
#include <winrt/%.h>
)";
            w.write(format, ns);
        }

        w.flush_to_file(folder / filename);
    }

    inline auto write_namespace_cpp(stdfs::path const& folder, std::string_view const& ns, cache::namespace_members const& members)
    {
        writer w;
        w.current_namespace = ns;
        auto filename = w.write_temp("py.%.cpp", ns);

        write_license(w);
        w.write("#include \"pybase.h\"\n");
        w.write("#include \"py.%.h\"\n", ns);

        settings.filter.bind_each<write_winrt_type_specialization_storage>(members.classes)(w);
        settings.filter.bind_each<write_winrt_type_specialization_storage>(members.interfaces)(w);
        settings.filter.bind_each<write_winrt_type_specialization_storage>(members.structs)(w);

        if (ns == "Windows.Foundation")
        {
            w.write(strings::custom_struct_convert);
        }
        settings.filter.bind_each<write_struct_convert_functions>(members.structs)(w);

        auto segments = get_dotted_name_segments(ns);
        w.write("\n\nnamespace py::cpp::%\n{", bind_list("::", segments));
        {
            writer::indent_guard g{ w };

            settings.filter.bind_each<write_inspectable_type>(members.classes)(w);
            settings.filter.bind_each<write_inspectable_type>(members.interfaces)(w);
            settings.filter.bind_each<write_struct>(members.structs)(w);
            write_namespace_initialization(w, ns, members);
        }
        w.write("} // py::cpp::%\n", bind_list("::", segments));

        write_namespace_module_init_function(w, ns);

        w.flush_to_file(folder / filename);
        return std::move(w.needed_namespaces);
    }

    inline void write_winrt_module_cpp(stdfs::path const& folder)
    {
        writer w;

        write_license(w);
        w.write(strings::winrt_module);

        auto filename = w.write_temp("_winrt.cpp");
        w.flush_to_file(folder / filename);
    }

    void write_namespace_cpp_filename(writer& w, std::string const& ns)
    {
        w.write("\"./%/src/py.%.cpp\"", settings.module, ns);
    }

    
    inline void write_setup_py(stdfs::path const& folder, std::vector<std::string> const& /*namespaces*/)
    {
        writer w;

        write_license(w, "#");
        w.write(strings::setup, settings.module, settings.module, settings.module);
        w.flush_to_file(folder / "setup.py");
    }

    inline void write_package_py_typed(stdfs::path const& folder)
    {
        writer w;

        w.flush_to_file(folder / "py.typed");
    }

    inline void write_winrt_pyi(stdfs::path const& folder)
    {
        writer w;

        write_license(w, "#");
        w.write(strings::_winrt);
        w.flush_to_file(folder / "_winrt.pyi");
    }

    inline void write_package_dunder_init_py(stdfs::path const& folder)
    {
        writer w;

        write_license(w, "#");
        w.write(strings::package_init, settings.module);
        w.flush_to_file(folder / "__init__.py");
    }

    inline void write_namespace_dunder_init_py(stdfs::path const& folder, std::string_view const& module_name, std::set<std::string> const& needed_namespaces, std::string_view const& ns, cache::namespace_members const& members)
    {
        writer w;
        w.current_namespace = ns;
        
        write_license(w, "#");

        if (settings.filter.includes(members.enums))
        {
            w.write("import enum\n");
            w.write("\n");
        }

        w.write("import %\n", module_name);

        w.write("\n_ns_module = %._import_ns_module(\"%\")\n", module_name, ns);

        w.write_each<write_python_import_namespace>(needed_namespaces);
        settings.filter.bind_each<write_python_enum>(members.enums)(w);
        w.write("\n");
        settings.filter.bind_each<write_python_import_type>(members.structs)(w);
        settings.filter.bind_each<write_python_import_type>(members.classes)(w);
        settings.filter.bind_each<write_python_import_type>(members.interfaces)(w);

        w.flush_to_file(folder / "__init__.py");
    }

    inline void write_namespace_dunder_init_pyi(stdfs::path const& folder, std::set<std::string> const& needed_namespaces, std::string_view const& ns, cache::namespace_members const& members)
    {
        writer w;
        w.current_namespace = ns;

        write_license(w, "#");

        if (settings.filter.includes(members.enums))
        {
            w.write("import enum\n");
        }

        w.write("import typing\n");
        w.write("import uuid\n"); // TODO: uuid is not always used
        w.write("\n");

        w.write("import @._winrt as _winrt", settings.module);

        w.write_each<write_python_import_namespace>(needed_namespaces);
        settings.filter.bind_each<write_python_enum>(members.enums)(w);
        w.write("\n");
        settings.filter.bind_each<write_python_typings>(members.structs)(w);
        settings.filter.bind_each<write_python_typings>(members.classes)(w);
        settings.filter.bind_each<write_python_typings>(members.interfaces)(w);
        settings.filter.bind_each<write_python_type_alias>(members.delegates)(w);

        w.flush_to_file(folder / "__init__.pyi");
    }
}
