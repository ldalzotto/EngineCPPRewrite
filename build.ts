
import * as path from 'path';
import * as child_process from 'child_process';
import * as fs from 'fs';


enum BuildType
{
    DEBUG = <any>"DEBUG",
    RELEASE_DEBUG = <any>"RELEASE_DEBUG",
    RELEASE = <any>"RELEASE"
};

enum CompilerType
{
    GCC = <any>"GCC",
    MSVC = <any>"MSVC"
};

class CompileTypeProvider
{
    public static compiler(p_compiler_type: CompilerType): string
    {
        if (p_compiler_type == CompilerType.GCC)
        {
            return "gcc";
        }
        else if (p_compiler_type == CompilerType.MSVC)
        {
            return "cl /nologo";
        }
    };

    public static math_link(p_compiler_type: CompilerType): string
    {
        if (p_compiler_type == CompilerType.GCC)
        {
            return "-lm";
        }
        return "";
    };

    public static wall(p_compiler_type: CompilerType): string
    {
        if (p_compiler_type == CompilerType.GCC)
        {
            return "-Wall";
        }
        else if (p_compiler_type == CompilerType.MSVC)
        {
            return "";
        }
    };

    public static preprocessor(p_compiler_type: CompilerType, p_preprocessor_name: string, p_preprocessor_value: string): string
    {
        if (p_compiler_type == CompilerType.GCC)
        {
            return `-D ${p_preprocessor_name}=${p_preprocessor_value}`;
        }
        else if (p_compiler_type == CompilerType.MSVC)
        {
            return `/D"${p_preprocessor_name}=${p_preprocessor_value}"`;
        }
    };

    public static debug_symbols(p_compiler_type: CompilerType): string
    {
        if (p_compiler_type == CompilerType.GCC)
        {
            return "-g";
        }
        else if (p_compiler_type == CompilerType.MSVC)
        {
            return "/Zi";
        }
    };

    public static optimization(p_compiler_type: CompilerType, p_build_type: BuildType): string
    {
        let l_optimisation_flags = "";
        switch (p_build_type)
        {
            case BuildType.DEBUG:
                if (p_compiler_type == CompilerType.GCC)
                {
                    l_optimisation_flags += "-O0";
                }
                break;
            case BuildType.RELEASE_DEBUG:
                if (p_compiler_type == CompilerType.GCC)
                {
                    l_optimisation_flags += "-O2";
                }
                else if (p_compiler_type == CompilerType.MSVC)
                {
                    l_optimisation_flags += "/O2";
                }
                break;
            case BuildType.RELEASE:
                if (p_compiler_type == CompilerType.GCC)
                {
                    l_optimisation_flags += "-O2";
                }
                else if (p_compiler_type == CompilerType.MSVC)
                {
                    l_optimisation_flags += "/O2";
                }
                break;
        };
        return l_optimisation_flags;
    };

    public static include_dir(p_compiler_type: CompilerType, p_dir: string): string
    {
        if (p_compiler_type == CompilerType.GCC)
        {
            return `-I${p_dir}`;
        }
        else if (p_compiler_type == CompilerType.MSVC)
        {
            return `/I${p_dir}`;
        }
    };

    public static compile_obj_flag(p_compiler_type: CompilerType): string
    {
        if (p_compiler_type == CompilerType.GCC)
        {
            return `-c`;
        }
        else if (p_compiler_type == CompilerType.MSVC)
        {
            return `/c`;
        }
    };

    public static static_lib_compile_flag(p_compiler_type: CompilerType): string
    {
        if (p_compiler_type == CompilerType.GCC)
        {
            return `ar rcs`;
        }
        else if (p_compiler_type == CompilerType.MSVC)
        {
            return `lib /nologo`;
        }
    };

    public static outfile_gcc(p_file: string): string
    {
        return `-o ${p_file}`;
    };
    public static outfile_msvc(p_file: string): string
    {
        return `/Fo"${p_file}"`;
    };

    public static outfile(p_compile_type: CompilerType, p_file: string): string
    {
        if (p_compile_type == CompilerType.GCC)
        {
            return this.outfile_gcc(p_file);
        }
        else if (p_compile_type == CompilerType.MSVC)
        {
            return this.outfile_msvc(p_file);
        }
    };
};

class PreprocessorConstants2
{
    public DEBUG: string;
    public RELEASE_DEBUG: string;
    public RELEASE: string;
};

class RootPath
{
    public static evaluate(): string
    {
        let l_root_path_dir: string[] = __dirname.split(path.sep);
        return l_root_path_dir.splice(0, l_root_path_dir.length - 1).join(path.sep);
    };
}

const root_path: string = RootPath.evaluate();
const build_directory: string = path.join(root_path, "build");

class Command
{
    public command: string;
    public file: string;

    public constructor(p_command: string, p_file: string)
    {
        this.command = p_command;
        this.file = p_file;
    }

    public execute(): boolean
    {
        let l_begin: number = new Date().getTime();
        let l_options: child_process.ExecSyncOptions = {
            //  stdio: "pipe"
            stdio: "inherit"
        };
        try
        {
            child_process.execSync(this.command, l_options);
        } catch (error)
        {
            console.error(error.stdout.toString());
            return false;
        }

        console.log(`${this.file} :  ${(new Date().getTime() - l_begin) / 1000}s`);
        return true;
    };

    public execute_print_command(): boolean
    {
        console.log(this.command);

        return this.execute();
    };
};


enum ModuleType
{
    INTERFACE = <any>"INTERFACE",
    EXECUTABLE = <any>"EXECUTABLE",
    STATIC_LIBRARY = <any>"STATIC_LIBRARY",
    DYNAMIC_LIBRARY = <any>"DYNAMIC_LIBRARY"
};


class LinkedLib
{
    public name: string;
    public build_type: BuildType;
    public constructor(p_name: string, p_buld_type: BuildType)
    {
        this.name = p_name;
        this.build_type = p_buld_type;
    };
};

class CommandConfiguration
{
    public name: string;
    public module_type: ModuleType;
    public build_type?: BuildType;
    public global_link_flags: string;
    public project_name: string;
    public project_root: string;
    public main_file: string;
    public include_directories: string[];
    public linked_libs: LinkedLib[];

    public build_executable_command(p_compiler_type: CompilerType, p_preprocessor_constants: PreprocessorConstants2): Command
    {
        let l_debug_flags = CompileTypeProvider.wall(p_compiler_type);

        if (this.build_type == BuildType.DEBUG
            || this.build_type == BuildType.RELEASE_DEBUG)
        {
            l_debug_flags += ` ${CompileTypeProvider.debug_symbols(p_compiler_type)}`;
        };

        let l_optimisation_flags = CompileTypeProvider.optimization(p_compiler_type, this.build_type);

        //preprocessors
        let l_preprocessor_values = p_preprocessor_constants[this.build_type];

        let l_include_directories: string = "";
        this.include_directories.forEach(_include_dir => { l_include_directories += ` ${CompileTypeProvider.include_dir(p_compiler_type, _include_dir)}` });

        let l_linked_libs: string = "";
        if (this.linked_libs && this.linked_libs.length > 0)
        {
            this.linked_libs.forEach((_linked_lib: LinkedLib) =>
            {
                if (p_compiler_type == CompilerType.MSVC)
                {
                    l_linked_libs += ` ${path.join(build_directory, _linked_lib.name)}_${_linked_lib.build_type}.lib `;
                }
                else if (p_compiler_type == CompilerType.GCC)
                {
                    l_linked_libs += ` -L${build_directory} -l${_linked_lib.name}_${_linked_lib.build_type} `;
                }

            });
        }

        let l_file_name = `${path.join(build_directory, this.project_name) + '_' + this.build_type + '.exe'}`;

        const c: string = CompileTypeProvider.compiler(p_compiler_type);
        let l_command: string = `${c} ${l_debug_flags} ${l_optimisation_flags} ${l_preprocessor_values} ${this.main_file} ${l_include_directories}`;
        l_command += ` ${CompileTypeProvider.outfile_gcc(l_file_name)} ${l_linked_libs} ${this.global_link_flags} `;
        return new Command(l_command, this.main_file);
    };

    public build_obj_generation_command(p_compiler_type: CompilerType, p_preprocessor_constants: PreprocessorConstants2): Command
    {
        let l_debug_flags = CompileTypeProvider.wall(p_compiler_type);

        let l_library_build_flag = CompileTypeProvider.compile_obj_flag(p_compiler_type);

        if (this.build_type == BuildType.DEBUG
            || this.build_type == BuildType.RELEASE_DEBUG)
        {
            l_debug_flags += ` ${CompileTypeProvider.debug_symbols(p_compiler_type)}`;
        };

        let l_optimisation_flags = CompileTypeProvider.optimization(p_compiler_type, this.build_type);

        //preprocessors
        let l_preprocessor_values = p_preprocessor_constants[this.build_type];

        let l_include_directories: string = "";
        this.include_directories.forEach(_include_dir => { l_include_directories += ` ${CompileTypeProvider.include_dir(p_compiler_type, _include_dir)}` });

        /* 
        let l_linked_libs: string = "";
        if (this.linked_libs && this.linked_libs.length > 0)
        {
            this.linked_libs.forEach((_linked_lib: LinkedLib) =>
            {
                l_linked_libs += ` ${path.join(build_directory, _linked_lib.name)}_${_linked_lib.build_type}.lib `;
            });
        }
        */

        let l_file_name = `${path.join(build_directory, this.project_name) + '_' + this.build_type + '.obj'}`;

        const c: string = CompileTypeProvider.compiler(p_compiler_type);
        let l_command: string = `${c} ${l_library_build_flag} ${l_debug_flags} ${l_optimisation_flags} ${l_preprocessor_values} ${this.main_file} ${l_include_directories}`;
        l_command += ` ${CompileTypeProvider.outfile(p_compiler_type, l_file_name)} ${this.global_link_flags}" `;
        return new Command(l_command, this.main_file);
    };

    public build_lib_generation_command(p_compiler_type: CompilerType, p_preprocessor_constants: PreprocessorConstants2): Command
    {

        let l_obj_file: string = path.join(build_directory, this.project_name) + '_' + this.build_type + '.obj';
        let l_file_name: string = path.join(build_directory, this.project_name) + '_' + this.build_type + '.lib';

        if (p_compiler_type == CompilerType.GCC)
        {
            let l_command: string = `${CompileTypeProvider.static_lib_compile_flag(p_compiler_type)} ${l_file_name} ${l_obj_file}`;
            return new Command(l_command, this.main_file);
        }
        else if (p_compiler_type == CompilerType.MSVC)
        {
            let l_command: string = `${CompileTypeProvider.static_lib_compile_flag(p_compiler_type)} ${l_obj_file}`;
            l_command += ` ${CompileTypeProvider.outfile(p_compiler_type, l_file_name)}" `;
            return new Command(l_command, this.main_file);
        }



    };
};


class EnvironmentEntry
{
    public name: string;
    public value: string;
};

class Environment
{
    public DEBUG: EnvironmentEntry[];
    public RELEASE_DEBUG: EnvironmentEntry[];
    public RELEASE: EnvironmentEntry[];
};


class BuildConfigurationEntry
{
    public base_name: string;
    public module_type: ModuleType;
    public build_type: BuildType;
    public root_folder: string;
    public main_file?: string; //optional for interfaces
    public include_directories: string[];
    public dependencies: string[]; //strings are base_names

    public static build_command_configuration(p_entry: BuildConfigurationEntry, p_compiler_type: CompilerType): CommandConfiguration
    {
        let command_config: CommandConfiguration = new CommandConfiguration();
        command_config.name = p_entry.base_name;
        command_config.module_type = p_entry.module_type;
        command_config.build_type = p_entry.build_type;
        command_config.global_link_flags = CompileTypeProvider.math_link(p_compiler_type); //for math
        command_config.project_name = p_entry.base_name;
        command_config.project_root = path.join(root_path, p_entry.root_folder);
        command_config.main_file = path.join(command_config.project_root, p_entry.main_file);
        command_config.include_directories = p_entry.include_directories.map(_include => path.join(root_path, _include));
        command_config.linked_libs = [];
        return command_config;
    };
};

class BuildConfiguration
{
    public compiler: CompilerType;
    public environment: Environment;
    public modules: BuildConfigurationEntry[];
};



// ################ loading configuration

const build_configuration: BuildConfiguration = JSON.parse(fs.readFileSync(path.join(root_path, "build_config.json")).toString());
const args: string[] = process.argv.slice(2);

// ################ ENVIRONMENT


class PreprocessorBuilder
{
    public static concat_environment_entries(p_compiler_type: CompilerType, p_entries: EnvironmentEntry[]): string
    {
        let l_return: string = "";
        p_entries.forEach((_entry: EnvironmentEntry) =>
        {
            l_return += " ";
            l_return += CompileTypeProvider.preprocessor(p_compiler_type, _entry.name, _entry.value);
            l_return += " ";
        });
        return l_return;
    };

    public static build(p_compiler_type: CompilerType, p_environment: Environment): PreprocessorConstants2
    {
        return {
            DEBUG: this.concat_environment_entries(p_compiler_type, p_environment.DEBUG),
            RELEASE_DEBUG: this.concat_environment_entries(p_compiler_type, p_environment.RELEASE_DEBUG),
            RELEASE: this.concat_environment_entries(p_compiler_type, p_environment.RELEASE)
        };
    };
};

let preprocessor_constants: PreprocessorConstants2 = PreprocessorBuilder.build(build_configuration.compiler, build_configuration.environment);


interface BuildModules
{
    [key: string]: BuildConfigurationEntry;
};

let build_modules: BuildModules = {};

build_configuration.modules.forEach((p_entry: BuildConfigurationEntry) =>
{
    build_modules[p_entry.base_name] = p_entry;
});


let l_all_commands: Command[] = [];

class BuildedModule
{
    public include_dirs: string[];
    public linked_libs: LinkedLib[];
    public main_file?: string;

    public constructor()
    {
        this.include_dirs = [];
        this.linked_libs = [];
    }
};

interface BuildedModules
{
    [p_module: string]: BuildedModule;
};

class RecursiveModuleBuilder
{


    public build_modules: BuildModules;
    public compiler_type: CompilerType;

    public builded_modules: BuildedModules;

    public generated_commands: Command[];

    public dependency_iterator_stack: number[];
    public module_stack: string[];

    public constructor(p_compile_type: CompilerType, p_build_modules: BuildModules)
    {
        this.compiler_type = p_compile_type;
        this.build_modules = p_build_modules;
        this.builded_modules = {};
        this.dependency_iterator_stack = [];
        this.module_stack = [];
        this.generated_commands = [];
    };

    public start(p_module: string): boolean
    {
        this.push_to_stack(p_module, 0);

        return true;
    };

    public step(): boolean
    {
        if (this.dependency_iterator_stack.length == 0)
        {
            return false;
        }

        let l_iterator = this.get_interator_stack();
        let l_module_key = this.get_iterator_module();
        let l_module: BuildConfigurationEntry = this.build_modules[l_module_key];

        if (l_iterator < l_module.dependencies.length)
        {
            // let l_child_module: BuildConfigurationEntry = this.build_modules[l_module.dependencies[l_iterator]];
            this.set_iterator_stack(l_iterator + 1);
            this.push_to_stack(l_module.dependencies[l_iterator], 0);
            return true;
        }
        else
        {

            if (l_module.module_type == ModuleType.EXECUTABLE)
            {
                let l_command_config = BuildConfigurationEntry.build_command_configuration(build_modules[l_module.base_name], build_configuration.compiler);
                for (let i: number = 0; i < l_module.dependencies.length; i++)
                {
                    let l_child_module: BuildConfigurationEntry = this.build_modules[l_module.dependencies[i]];
                    l_command_config.include_directories = l_command_config.include_directories.concat(this.builded_modules[l_child_module.base_name].include_dirs);
                    l_command_config.linked_libs = l_command_config.linked_libs.concat(this.builded_modules[l_child_module.base_name].linked_libs);
                }
                this.generated_commands.push(l_command_config.build_executable_command(this.compiler_type, preprocessor_constants));
            }
            else if (l_module.module_type == ModuleType.STATIC_LIBRARY)
            {
                let l_command_config = BuildConfigurationEntry.build_command_configuration(build_modules[l_module.base_name], build_configuration.compiler);
                for (let i: number = 0; i < l_module.dependencies.length; i++)
                {
                    let l_child_module: BuildConfigurationEntry = this.build_modules[l_module.dependencies[i]];
                    l_command_config.include_directories = l_command_config.include_directories.concat(this.builded_modules[l_child_module.base_name].include_dirs);
                    l_command_config.linked_libs = l_command_config.linked_libs.concat(this.builded_modules[l_child_module.base_name].linked_libs);
                }
                this.generated_commands.push(l_command_config.build_obj_generation_command(this.compiler_type, preprocessor_constants));
                this.generated_commands.push(l_command_config.build_lib_generation_command(this.compiler_type, preprocessor_constants));

                let l_builded_module: BuildedModule = new BuildedModule();
                l_builded_module.include_dirs = l_command_config.include_directories;
                l_builded_module.linked_libs = l_command_config.linked_libs;
                l_builded_module.linked_libs.push(new LinkedLib(l_command_config.name, l_command_config.build_type));
                l_builded_module.main_file = l_command_config.main_file;
                this.builded_modules[l_module_key] = l_builded_module;
            }
            else if (l_module.module_type == ModuleType.INTERFACE)
            {
                let l_builded_module: BuildedModule = new BuildedModule();
                l_builded_module.include_dirs = l_builded_module.include_dirs.concat(l_module.include_directories);
                // l_builded_module.linked_libs = l_builded_module.linked_libs.concat(l_module.linked_libs);
                for (let i: number = 0; i < l_module.dependencies.length; i++)
                {
                    l_builded_module.include_dirs = l_builded_module.include_dirs.concat(this.builded_modules[l_module.dependencies[i]].include_dirs);
                    l_builded_module.linked_libs = l_builded_module.linked_libs.concat(this.builded_modules[l_module.dependencies[i]].linked_libs);
                }
                //                l_builded_module.main_file = l_command_config.main_file;
                this.builded_modules[l_module_key] = l_builded_module;
            }
            this.pop_stack();
            return true;
        }
    };

    public clear_commands()
    {
        this.dependency_iterator_stack = [];
        this.module_stack = [];
        this.generated_commands = [];
    };

    private push_to_stack(p_module: string, p_iterator: number)
    {
        this.module_stack.push(p_module);
        this.dependency_iterator_stack.push(p_iterator);
    };

    private pop_stack()
    {
        this.module_stack.pop();
        this.dependency_iterator_stack.pop();
    };

    private set_iterator_stack(p_iterator: number)
    {
        this.dependency_iterator_stack[this.dependency_iterator_stack.length - 1] = p_iterator;
    };

    private get_interator_stack(): number
    {
        return this.dependency_iterator_stack[this.dependency_iterator_stack.length - 1];
    };

    private get_iterator_module(): string
    {
        return this.module_stack[this.module_stack.length - 1];
    };
};


if (args[0] == "compile_database")
{
    let l_builder = new RecursiveModuleBuilder(build_configuration.compiler, build_modules);
    let l_build_module_keys = Object.keys(build_modules);
    for (let i = 0; i < l_build_module_keys.length; i++)
    {
        let l_module: BuildConfigurationEntry = build_modules[l_build_module_keys[i]];
        if (l_module.module_type == ModuleType.EXECUTABLE || l_module.module_type == ModuleType.STATIC_LIBRARY)
        {
            l_builder.start(l_module.base_name);
            while (l_builder.step()) { }
            l_all_commands = l_all_commands.concat(l_builder.generated_commands);
            l_builder.clear_commands();
        }
    }


    // compile_json_gneeration
    class CommandGenerationEntry
    {
        public directory: string;
        public command: string;
        public file: string;

        public constructor(p_directory: string, p_command: string, p_file: string)
        {
            this.directory = p_directory;
            this.command = p_command;
            this.file = p_file;
        };
    };


    let l_compile_json_generation_file: string = path.join(build_directory, "compile_commands.json");
    let compile_json_generation = JSON.stringify(
        l_all_commands.map(_command => new CommandGenerationEntry(build_directory, _command.command, _command.file))
    );

    fs.writeFileSync(l_compile_json_generation_file, compile_json_generation);

}
else if (args[0] == "all")
{
    let l_builder = new RecursiveModuleBuilder(build_configuration.compiler, build_modules);
    let l_build_module_keys = Object.keys(build_modules);
    for (let i = 0; i < l_build_module_keys.length; i++)
    {
        let l_module: BuildConfigurationEntry = build_modules[l_build_module_keys[i]];
        if (l_module.module_type == ModuleType.EXECUTABLE)
        {
            l_builder.start(l_module.base_name);
            while (l_builder.step()) { }
            l_all_commands = l_all_commands.concat(l_builder.generated_commands);
            l_builder.clear_commands();
        }
    }

    for (let i = 0; i < l_all_commands.length; i++)
    {
        if (!l_all_commands[i].execute())
        {
            break;
        }
    };
}
else
{
    let asked_builded_module: string = args[0];
    let l_builder = new RecursiveModuleBuilder(build_configuration.compiler, build_modules);
    l_builder.start(asked_builded_module);
    while (l_builder.step()) { }
    l_all_commands = l_builder.generated_commands;

    for (let i = 0; i < l_all_commands.length; i++)
    {
        if (!l_all_commands[i].execute())
        {
            break;
        }
    };
}




























/*
class InterfaceLib
{
    public lib_include_dirs: string[];

    public constructor(p_lib_include_dirs: string[])
    {
        this.lib_include_dirs = p_lib_include_dirs;
    }
};

class StaticLib
{
    public lib_commands: Command[];
    public lib: string;
    public lib_include_dirs: string[];

    public constructor(p_lib_commands: Command[], p_lib: string, p_include_dirs: string[])
    {
        this.lib_commands = p_lib_commands;
        this.lib = p_lib;
        this.lib_include_dirs = p_include_dirs;
    }
};



class CommandBuilder
{
    public static build_common2(): InterfaceLib
    {
        return new InterfaceLib(build_modules["Common2"].include_directories);
    };

    public static build_math2(): InterfaceLib
    {
        return new InterfaceLib(build_modules["Math2"].include_directories);
    };

    public static build_Common2Test(p_common2: InterfaceLib): Command
    {
        let l_command_config = BuildConfigurationEntry.build_command_configuration(build_modules["Common2Test"], build_configuration.compiler);
        l_command_config.include_directories = l_command_config.include_directories.concat(p_common2.lib_include_dirs);
        return l_command_config.build_executable_command(build_configuration.compiler, preprocessor_constants);
    };

    public static build_Math2test(p_common2: InterfaceLib, p_math2: InterfaceLib): Command
    {
        let l_command_config = BuildConfigurationEntry.build_command_configuration(build_modules["Math2Test"], build_configuration.compiler);
        l_command_config.include_directories = l_command_config.include_directories.concat(p_common2.lib_include_dirs);
        l_command_config.include_directories = l_command_config.include_directories.concat(p_math2.lib_include_dirs);
        return l_command_config.build_executable_command(build_configuration.compiler, preprocessor_constants);
    };

    public static build_Collision(p_common2: InterfaceLib, p_math2: InterfaceLib): StaticLib
    {
        let l_commands: Command[] = [];
        let l_command_config = BuildConfigurationEntry.build_command_configuration(build_modules["Collision"], build_configuration.compiler);
        l_command_config.include_directories = l_command_config.include_directories.concat(p_common2.lib_include_dirs);
        l_command_config.include_directories = l_command_config.include_directories.concat(p_math2.lib_include_dirs);
        l_command_config.linked_libs.push(l_command_config.name);

        l_commands.push(l_command_config.build_obj_generation_command(build_configuration.compiler, preprocessor_constants));
        l_commands.push(l_command_config.build_lib_generation_command(build_configuration.compiler, preprocessor_constants));
        return new StaticLib(l_commands, l_command_config.name, l_command_config.include_directories);
    };

    public static build_CollisionTest(p_common2: InterfaceLib, p_math2: InterfaceLib, p_collision_link: StaticLib): Command[]
    {
        let l_commands: Command[] = p_collision_link.lib_commands;
        let l_command_config = BuildConfigurationEntry.build_command_configuration(build_modules["CollisionTest"], build_configuration.compiler);
        l_command_config.include_directories = l_command_config.include_directories.concat(p_common2.lib_include_dirs);
        l_command_config.include_directories = l_command_config.include_directories.concat(p_math2.lib_include_dirs);
        l_command_config.include_directories = l_command_config.include_directories.concat(p_collision_link.lib_include_dirs);
        l_command_config.linked_libs.push(p_collision_link.lib);

        return l_commands.concat(l_command_config.build_executable_command(build_configuration.compiler, preprocessor_constants));
    };
};


l_all_commands.push(CommandBuilder.build_Common2Test(CommandBuilder.build_common2()));
l_all_commands.push(CommandBuilder.build_Math2test(CommandBuilder.build_common2(), CommandBuilder.build_math2()));

let l_command_lib: StaticLib = CommandBuilder.build_Collision(CommandBuilder.build_common2(), CommandBuilder.build_math2());

l_all_commands = l_all_commands.concat(
    CommandBuilder.build_CollisionTest(
        CommandBuilder.build_common2(),
        CommandBuilder.build_math2(),
        l_command_lib
    )
);

interface InterfaceLibs
{
    [p_module: string]: InterfaceLib;
};

interface StaticLibs
{
    [p_module: string]: StaticLib;
};

*/