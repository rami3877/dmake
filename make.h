/*
 ===============================================================================
            Builder API Native Interface Documentation & Specifications
 ===============================================================================
 
  This library implements a pure C build-automation engine. It handles compilation, 
  linking, and recursive dependency resolving using an optimized incremental build 
  approach (tracking modification timestamps).

  Global API Functions:
    setGlobalCompiler(const i8*)  Set default compiler (default: gcc)
    setBuildDir(const i8*)        Set output directory (default: build)
    setGlobalFlags(const i8*)     Set default compilation flags (e.g. -Wall -O2 );
    setGlobalIncludes(const i8*)  Set default include directories (space-separated)
    setGlobalLibRpaths(const i8*) Set default library paths for runtime rpath linking
    objectAdd(BObject*)           Process, compile, and link a target object

  Environment Variables Configuration:
    BUILDER_EXTRA_FLAGS      Append extra compilation flags via shell.
    BUILDER_EXTRA_INCLUDES   Append extra include paths (-I) via shell.
    BUILDER_EXTRA_LIBRSPATH  Append library paths (-L) and active runpaths (-Wl,-rpath).

*/
#ifndef BUILDER_H
#define BUILDER_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <glob.h>

typedef unsigned long long u64;
typedef char i8;

#define null NULL

typedef enum {
    B_EXEC,
    B_STATIC,
    B_SHARED
} BObjectType;

typedef struct BObject {
    const i8 *name;
    BObjectType type;
    const i8 *compiler;
    const i8 **flags;
    const i8 **includes;
    const i8 **files;
    const i8 **librs;
    struct BObject **object;
} BObject;



/**
 * @brief Configures the global default compiler fallback.
 * @param compiler The name or path of the compiler executable (e.g., "gcc", "clang").
 * @note This compiler is automatically used whenever a specific target object 
 *       (BObject) leaves its custom `.compiler` property unassigned.
 */
void setGlobalCompiler(const i8 *compiler);

/**
 * @brief Sets the global output build directory.
 * @param dir The relative or absolute directory path (e.g., "build", "bin/").
 * @note The routine guarantees a trailing '/' safely handles string concatenation. 
 *       All intermediary `.o` object files, artifacts, and final binaries are stored here.
 */
void setBuildDir(const i8 *dir);

/**
 * @brief Sets global compiler flags applicable to all compile commands.
 * @param flags A space-separated list of arguments (e.g., "-Wall -Wextra -O3").
 * @note These flags are injected into every target's translation unit assembly stage.
 */
void setGlobalFlags(const i8 *flags);

/**
 * @brief Sets global include search directory paths.
 * @param includes A space-separated list of paths (e.g., "include src/headers").
 * @note The engine automatically converts every entry into proper "-I path" arguments.
 */
void setGlobalIncludes(const i8 *includes);

/**
 * @brief Configures global external library paths and runtime runpaths (Rpath).
 * @param paths A space-separated list of search directories.
 * @note The function maps entries to "-L" link-time options, alongside structural 
 *       "-Wl,-rpath" arguments to ensure dynamic shared libraries (.so) resolve correctly at runtime.
 */
void setGlobalLibRpaths(const i8 *paths);

/**
 * @brief Handles automatic script self-rebuilding, internal context evaluation, and CLI subcommands.
 * @param argc Pass-through token count from main block.
 * @param argv Pass-through argument array from main block.
 * @param source_file The active build script path (always feed macro context __FILE__).
 * 
 * @details This wrapper serves three central orchestration behaviors:
 *  1. Self-Rebuilding: Compares script source code modification times against its runtime binary.
 *     If source code changes, it recompiles and instantly dry-reloads via (execv).
 *  2. Help CLI Command (-h / --help): Interrupts flow to echo manual usage, syntax structures, and an integration model.
 *  3. Clean CLI Command (clean): Executes an automated file wipe of target directories and purges the binary utility.
 * 
 * @note The CLI help menu explicitly documents the integrated Shell environment variables below.
 */
void check_and_rebuild_self(int argc, char **argv, const i8 *source_file);

/**
 * @brief Core Build Engine - Evaluates, transforms, and links build target structures.
 * @param object Pointer to the targeted object configuration (BObject).
 * 
 * @details Engine Features & Runtime Environment Variable Injection:
 *  - Incremental Compilation: Checks source vs object timestamps to skip up-to-date translation units.
 *  - Wildcard Expansion: Wireframe structures process Unix glob filters (e.g., "*.c") seamlessly.
 *  - Recursive Dependency Resolution: Walks down target `.object` trees to build dependencies prior to root assembly.
 * 
 *  - Shell Environment Integration:
 *    The engine reads the operating system environment blocks at runtime via `getenv()`. 
 *    These parameters override or extend the hardcoded configuration without requiring script recompilation:
 *    1. `BUILDER_EXTRA_FLAGS`: Injects dynamic compilation/optimization arguments directly into the translation stages.
 *    2. `BUILDER_EXTRA_INCLUDES`: Dynamically appends extra lookup directory headers via mapped "-I" parameters.
 *    3. `BUILDER_EXTRA_LIBRSPATH`: Resolves linker search entries via "-L" and generates embedded dynamic runpaths via "-Wl,-rpath".
 */
void objectAdd(BObject *object);

#endif // BUILDER_H

#ifdef BUILDER_IMPLEMENTATION

// Global Configuration States
static const i8 *global_compiler = "gcc";
static const i8 *global_build_dir = "build/";
static i8 *global_api_flags = null;
static i8 *global_api_includes = null;
static i8 *global_api_libpaths = null;

void setGlobalCompiler(const i8 *compiler) { 
    global_compiler = compiler; 
}

void setBuildDir(const i8 *dir) { 
    if (!dir) return;
    u64 len = strlen(dir);
    if (dir[len - 1] != '/') {
        asprintf((i8**)&global_build_dir, "%s/", dir);
    } else {
        global_build_dir = dir;
    }
}

void setGlobalFlags(const i8 *flags) { 
    if (flags) global_api_flags = strdup(flags); 
}

void setGlobalIncludes(const i8 *includes) { 
    if (includes) global_api_includes = strdup(includes); 
}

void setGlobalLibRpaths(const i8 *paths) { 
    if (paths) global_api_libpaths = strdup(paths); 
}

static i8** split_space(const i8 *str, u64 *count) {
    *count = 0;
    if (!str || strlen(str) == 0) return null;
    
    i8 *copy = strdup(str);
    i8 *token = strtok(copy, " ");
    u64 cap = 4;
    i8 **res = malloc(cap * sizeof(i8*));
    
    while (token) {
        if (*count >= cap) { 
            cap *= 2; 
            res = realloc(res, cap * sizeof(i8*)); 
        }
        res[(*count)++] = strdup(token);
        token = strtok(null, " ");
    }
    free(copy);
    return res;
}

static int fileIsModif(const i8 *src, const i8 *obj) {
    struct stat s_stat, o_stat;
    if (stat(src, &s_stat) != 0) return 1;
    if (stat(obj, &o_stat) != 0) return 1;
    return s_stat.st_mtime > o_stat.st_mtime;
}

static int run_command(i8 **argv) {
    u64 size = 0;
    while (argv[size]) size++;
    
    u64 total_len = 0;
    for (u64 i = 0; i < size; i++) {
        total_len += strlen(argv[i]) + 1;
    }
    
    i8 *cmd = malloc(total_len + 1); 
    cmd[0] = '\0';
    
    for (u64 i = 0; i < size; i++) { 
        strcat(cmd, argv[i]); 
        strcat(cmd, " "); 
    }
    
    printf("[CMD] %s\n", cmd);
    int res = system(cmd);
    free(cmd);
    return res;
}

void check_and_rebuild_self(int argc, char **argv, const i8 *source_file) {
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        printf("Usage: %s [COMMAND]\n\n", argv[0]);
        printf("Commands:\n");
        printf("  (No args)       Build the targets defined in the project.\n");
        printf("  cleanall        Remove the build directory and the builder executable.\n");
        printf("  clean           Remove the build directory                            \n");
        printf("  -h, --help      Show this help menu.\n\n");
        
        printf("Global API Functions:\n");
        printf("  setGlobalCompiler(const i8*)  Set default compiler (default: \"gcc\")\n");
        printf("  setBuildDir(const i8*)        Set output directory (default: \"build/\")\n");
        printf("  setGlobalFlags(const i8*)     Set default compilation flags (e.g. \"-Wall -O2\")\n");
        printf("  setGlobalIncludes(const i8*)  Set default include directories (space-separated)\n");
        printf("  setGlobalLibRpaths(const i8*) Set default library paths for runtime rpath linking\n");
        printf("  objectadd(BObject*)           Process, compile, and link a target object\n\n");

        printf("Environment Variables Configuration:\n");
        printf("  BUILDER_EXTRA_FLAGS      Append extra compilation flags via shell.\n");
        printf("  BUILDER_EXTRA_INCLUDES   Append extra include paths (-I) via shell.\n");
        printf("  BUILDER_EXTRA_LIBRSPATH  Append library paths (-L) and active runpaths (-Wl,-rpath).\n\n");

        printf("Complete Integration Example (e.g., make.c):\n");
        printf("===============================================================================\n");
        printf("  #define BUILDER_IMPLEMENTATION\n");
        printf("  #include \"builder.h\"\n\n");
        printf("  int main(int argc, char **argv) {\n");
        printf("      check_and_rebuild_self(argc, argv, __FILE__);\n\n");
        printf("      setGlobalCompiler(\"clang\");\n");
        printf("      setGlobalFlags(\"-Wall -Wextra -O3\");\n");
        printf("      setBuildDir(\"output/\");\n\n");
        printf("      const i8 *math_src[] = { \"src/math/*.c\", null };\n");
        printf("      BObject my_lib = {\n");
        printf("          .name = \"mymath\",\n");
        printf("          .type = B_STATIC,\n");
        printf("          .files = math_src\n");
        printf("      };\n\n");
        printf("      const i8 *main_src[] = { \"src/main.c\", null };\n");
        printf("      const i8 *inc_paths[] = { \"include\", null };\n");
        printf("      BObject *deps[] = { &my_lib, null };\n");
        printf("      BObject app = {\n");
        printf("          .name = \"my_program\",\n");
        printf("          .type = B_EXEC,\n");
        printf("          .files = main_src,\n");
        printf("          .includes = inc_paths,\n");
        printf("          .object = deps\n");
        printf("      };\n\n");
        printf("      objectAdd(&app);\n");
        printf("      return 0;\n");
        printf("  }\n");
        printf("===============================================================================\n");
        exit(0);
    }

    if (argc > 1 && strcmp(argv[1], "cleanall") == 0) {
        printf("[Clean] Cleaning build directory: %s\n", global_build_dir);
        i8 *cmd_dir = null;
        asprintf(&cmd_dir, "rm -rf %s", global_build_dir);
        system(cmd_dir);
        free(cmd_dir);

        printf("[Clean] Removing self executable: %s\n", argv[0]);
        if (unlink(argv[0]) != 0) {
            i8 *cmd_self = null;
            asprintf(&cmd_self, "rm -f %s", argv[0]);
            system(cmd_self);
            free(cmd_self);
        }
        printf("[Clean] Operation complete.\n");
        exit(0); 
    }

    if (argc > 1 && strcmp(argv[1], "clean") == 0) {
        printf("[Clean] Cleaning build directory: %s\n", global_build_dir);
        i8 *cmd_dir = null;
        asprintf(&cmd_dir, "rm -rf %s", global_build_dir);
        system(cmd_dir);
        free(cmd_dir);

        printf("[Clean] Operation complete.\n");
        exit(0); 
    }

    struct stat src_stat, bin_stat;
    if (stat(source_file, &src_stat) == 0 && stat(argv[0], &bin_stat) == 0) {
        if (src_stat.st_mtime > bin_stat.st_mtime) {
            printf("[Self-Rebuild] Code changed. Recompiling %s...\n", source_file);
            i8 *cmd = null; 
            asprintf(&cmd, "%s %s -o %s", global_compiler, source_file, argv[0]);
            
            if (system(cmd) == 0) {
                free(cmd);
                printf("[Self-Rebuild] Success! Reloading builder...\n");
                execv(argv[0], argv);
                perror("execv failed");
                exit(1);
            }
            free(cmd);
        }
    }
}

static void collect_objects(BObject *obj, const i8 ***all_objects, u64 *total_count) {
    if (!obj || !obj->object) return;
    u64 idx = 0;
    while (obj->object[idx]) {
        BObject *dep = obj->object[idx];
        objectAdd(dep);
        
        i8 *dep_target = null;
        if (dep->type == B_STATIC) {
            asprintf(&dep_target, "%slib%s.a", global_build_dir, dep->name);
        } else if (dep->type == B_SHARED) {
            asprintf(&dep_target, "%slib%s.so", global_build_dir, dep->name);
        } else {
            asprintf(&dep_target, "%s%s.elf", global_build_dir, dep->name);
        }

        *all_objects = realloc(*all_objects, (*total_count + 1) * sizeof(i8*));
        (*all_objects)[*total_count] = dep_target; 
        (*total_count)++;
        idx++;
    }
}

void objectAdd(BObject *object) {
    if (!object) return;
    
    i8 *mkdir_base = null; 
    asprintf(&mkdir_base, "mkdir -p %s", global_build_dir);
    system(mkdir_base); 
    free(mkdir_base);

    const i8 *active_compiler = object->compiler ? object->compiler : global_compiler;
    i8 *dirbuild = null; 
    asprintf(&dirbuild, "%s%s", global_build_dir, object->name);
    
    u64 fileCount = 0;
    i8 **backup_files = null;
    u64 f_idx = 0;
    while (object->files && object->files[f_idx]) {
        const i8 *f_pattern = object->files[f_idx];
        if (strchr(f_pattern, '*') || strchr(f_pattern, '?')) {
            glob_t g_res;
            if (glob(f_pattern, 0, null, &g_res) == 0) {
                for (size_t g = 0; g < g_res.gl_pathc; g++) {
                    struct stat s;
                    if (stat(g_res.gl_pathv[g], &s) == 0 && S_ISREG(s.st_mode)) {
                        backup_files = realloc(backup_files, (fileCount + 1) * sizeof(i8*));
                        backup_files[fileCount++] = strdup(g_res.gl_pathv[g]);
                    }
                }
                globfree(&g_res);
            }
        } else {
            backup_files = realloc(backup_files, (fileCount + 1) * sizeof(i8*));
            backup_files[fileCount++] = strdup(f_pattern);
        }
        f_idx++;
    }

    u64 shellFlagCount = 0, shellIncludeCount = 0, shellLibPathCount = 0;
    i8 **shell_flags    = split_space(getenv("BUILDER_EXTRA_FLAGS"), &shellFlagCount);
    i8 **shell_includes = split_space(getenv("BUILDER_EXTRA_INCLUDES"), &shellIncludeCount);
    i8 **shell_libpaths = split_space(getenv("BUILDER_EXTRA_LIBRSPATH"), &shellLibPathCount);

    u64 apiFlagCount = 0, apiIncludeCount = 0, apiLibPathCount = 0;
    i8 **api_flags = split_space(global_api_flags, &apiFlagCount);
    i8 **api_includes = split_space(global_api_includes, &apiIncludeCount);
    i8 **api_libpaths = split_space(global_api_libpaths, &apiLibPathCount);

    u64 totalExtraFlags = shellFlagCount + apiFlagCount;
    const i8 **merged_flags = totalExtraFlags ? malloc(totalExtraFlags * sizeof(i8*)) : null;
    u64 m_f = 0;
    for (u64 j = 0; j < apiFlagCount; j++)   merged_flags[m_f++] = api_flags[j];
    for (u64 j = 0; j < shellFlagCount; j++) merged_flags[m_f++] = shell_flags[j];

    u64 totalExtraIncludes = shellIncludeCount + apiIncludeCount;
    const i8 **merged_includes = totalExtraIncludes ? malloc(totalExtraIncludes * sizeof(i8*)) : null;
    u64 m_i = 0;
    for (u64 j = 0; j < apiIncludeCount; j++)   merged_includes[m_i++] = api_includes[j];
    for (u64 j = 0; j < shellIncludeCount; j++) merged_includes[m_i++] = shell_includes[j];

    u64 totalExtraLibPaths = shellLibPathCount + apiLibPathCount;
    const i8 **merged_libpaths = totalExtraLibPaths ? malloc(totalExtraLibPaths * sizeof(i8*)) : null;
    u64 m_l = 0;
    for (u64 j = 0; j < apiLibPathCount; j++)   merged_libpaths[m_l++] = api_libpaths[j];
    for (u64 j = 0; j < shellLibPathCount; j++) merged_libpaths[m_l++] = shell_libpaths[j];

    u64 flagCount = 0, includeCount = 0, libCount = 0;
    while (object->flags && object->flags[flagCount]) flagCount++;
    while (object->includes && object->includes[includeCount]) includeCount++;
    while (object->librs && object->librs[libCount]) libCount++;

    int needBuild = 0;
    const i8 **all_objects = malloc(fileCount * sizeof(i8*));
    u64 total_objects_count = 0;

    for (u64 i = 0; i < fileCount; i++) {
        const i8 *src_file = backup_files[i];
        i8 *out_obj = null; 
        asprintf(&out_obj, "%s/%s.o", dirbuild, src_file);
        
        i8 *slash = strrchr(out_obj, '/');
        if (slash) {
            *slash = '\0';
            i8 *mkdir_cmd = null; 
            asprintf(&mkdir_cmd, "mkdir -p %s", out_obj);
            system(mkdir_cmd); 
            free(mkdir_cmd);
            *slash = '/';
        }

        if (fileIsModif(src_file, out_obj)) {
            needBuild = 1; 
            u64 extra_flags = (object->type == B_SHARED) ? 1 : 0;
            u64 cmd_alloc_size = 1 + flagCount + totalExtraFlags + extra_flags + ((includeCount + totalExtraIncludes) * 2) + 4 + 1;
            const i8 **compiler_cmd = malloc(cmd_alloc_size * sizeof(i8*));
            
            u64 c_idx = 0; 
            compiler_cmd[c_idx++] = active_compiler; 
            if (object->type == B_SHARED) compiler_cmd[c_idx++] = "-fPIC";
            
            if (object->flags) { 
                for (u64 j = 0; j < flagCount; j++) compiler_cmd[c_idx++] = object->flags[j]; 
            }
            for (u64 j = 0; j < totalExtraFlags; j++) compiler_cmd[c_idx++] = merged_flags[j];
            
            if (object->includes) {
                for (u64 j = 0; j < includeCount; j++) {
                    compiler_cmd[c_idx++] = "-I"; 
                    compiler_cmd[c_idx++] = object->includes[j];
                }
            }
            for (u64 j = 0; j < totalExtraIncludes; j++) {
                compiler_cmd[c_idx++] = "-I"; 
                compiler_cmd[c_idx++] = merged_includes[j];
            }
            
            compiler_cmd[c_idx++] = "-c"; 
            compiler_cmd[c_idx++] = src_file;
            compiler_cmd[c_idx++] = "-o"; 
            compiler_cmd[c_idx++] = out_obj;
            compiler_cmd[c_idx] = null; 

#ifdef BUILDER_PR
            printf("[%s] Compiling: %s -> %s\n", active_compiler, src_file, out_obj);
#endif
            if (run_command((i8 **)compiler_cmd)) {
                needBuild = 2; 
                free(compiler_cmd); 
                free(out_obj); 
                break;
            }
            free(compiler_cmd);
        }
        all_objects[total_objects_count++] = out_obj; 
    }

    collect_objects(object, &all_objects, &total_objects_count);

    i8 *check_target = null;
    if (object->type == B_STATIC) {
        asprintf(&check_target, "%slib%s.a", global_build_dir, object->name);
    } else if (object->type == B_SHARED) {
        asprintf(&check_target, "%slib%s.so", global_build_dir, object->name);
    } else {
        asprintf(&check_target, "%s%s.elf", global_build_dir, object->name);
    }
    
    if (access(check_target, F_OK) != 0) needBuild = 1;
    free(check_target);

    if (needBuild == 1 && total_objects_count > 0) {
        i8 *out_target = null; 
        const i8 **link_cmd = null;

        i8 **processed_libs = malloc((libCount + 1) * sizeof(i8*));
        for (u64 j = 0; j < libCount; j++) {
            const i8 *lib_item = object->librs[j];
            if (lib_item[0] != '-' && !strstr(lib_item, ".a") && !strstr(lib_item, ".so") && !strstr(lib_item, "/")) {
                asprintf(&processed_libs[j], "-l%s", lib_item);
            } else {
                processed_libs[j] = strdup(lib_item);
            }
        }
        processed_libs[libCount] = null;

        if (object->type == B_STATIC) {
            asprintf(&out_target, "%slib%s.a", global_build_dir, object->name);
            u64 cmd_size = 3 + total_objects_count + 1;
            link_cmd = malloc(cmd_size * sizeof(i8*)); 
            u64 l_idx = 0;
            
            link_cmd[l_idx++] = "ar"; 
            link_cmd[l_idx++] = "rcs"; 
            link_cmd[l_idx++] = out_target;
            for (u64 i = 0; i < total_objects_count; i++) link_cmd[l_idx++] = all_objects[i];
            link_cmd[l_idx] = null;

#ifdef BUILDER_PR
            printf("[ar] Creating Static Library: %s\n", out_target);
#endif
        } 
        else {
            if (object->type == B_SHARED) {
                asprintf(&out_target, "%slib%s.so", global_build_dir, object->name);
            } else {
                asprintf(&out_target, "%s%s.elf", global_build_dir, object->name);
            }

            u64 extra_link = (object->type == B_SHARED) ? 1 : 0;
            u64 cmd_size = 1 + extra_link + flagCount + totalExtraFlags + total_objects_count + (totalExtraLibPaths * 4) + libCount + 3 + 1;
            link_cmd = malloc(cmd_size * sizeof(i8*)); 
            u64 l_idx = 0;
            
            link_cmd[l_idx++] = active_compiler;
            if (object->type == B_SHARED) link_cmd[l_idx++] = "-shared";
            
            if (object->flags) { 
                for (u64 j = 0; j < flagCount; j++) link_cmd[l_idx++] = object->flags[j]; 
            }
            for (u64 j = 0; j < totalExtraFlags; j++) link_cmd[l_idx++] = merged_flags[j];
            for (u64 i = 0; i < total_objects_count; i++) link_cmd[l_idx++] = all_objects[i];
            
            for (u64 j = 0; j < totalExtraLibPaths; j++) {
                link_cmd[l_idx++] = "-L"; 
                link_cmd[l_idx++] = merged_libpaths[j];
                link_cmd[l_idx++] = "-Wl,-rpath";
                link_cmd[l_idx++] = merged_libpaths[j];
            }
            for (u64 j = 0; j < libCount; j++) link_cmd[l_idx++] = processed_libs[j];
            
            link_cmd[l_idx++] = "-o"; 
            link_cmd[l_idx++] = out_target;
            link_cmd[l_idx] = null;

#ifdef BUILDER_PR
            printf("[Linking Target]: %s\n",  out_target);
#endif
        }

        if (!run_command((i8 **)link_cmd)) {
            printf("[SUCCESS] Target %s built successfully!\n", object->name);
        }
        
        for (u64 j = 0; j < libCount; j++) free(processed_libs[j]);
        free(processed_libs); 
        free(link_cmd); 
        free(out_target);
    } 
    else if (needBuild == 0) {
        printf("[ Target %s is up to date ]\n", object->name);
    }

    if (merged_flags)    free(merged_flags); 
    if (merged_includes) free(merged_includes); 
    if (merged_libpaths) free(merged_libpaths); 

    if (shell_flags) { 
        for (u64 j = 0; j < shellFlagCount; j++) free(shell_flags[j]); 
        free(shell_flags); 
    }
    if (shell_includes) { 
        for (u64 j = 0; j < shellIncludeCount; j++) free(shell_includes[j]); 
        free(shell_includes); 
    }
    if (shell_libpaths) { 
        for (u64 j = 0; j < shellLibPathCount; j++) free(shell_libpaths[j]); 
        free(shell_libpaths); 
    }

    if (api_flags) { 
        for (u64 j = 0; j < apiFlagCount; j++) free(api_flags[j]); 
        free(api_flags); 
    }
    if (api_includes) { 
        for (u64 j = 0; j < apiIncludeCount; j++) free(api_includes[j]); 
        free(api_includes); 
    }
    if (api_libpaths) { 
        for (u64 j = 0; j < apiLibPathCount; j++) free(api_libpaths[j]); 
        free(api_libpaths); 
    }

    for (u64 i = 0; i < total_objects_count; i++) {
        if (all_objects[i] && strstr(all_objects[i], dirbuild)) {
            free((void*)all_objects[i]);
        }
    }
    free(all_objects);

    for (u64 i = 0; i < fileCount; i++) free(backup_files[i]);
    if (backup_files) free(backup_files);
    free(dirbuild);
}

#endif // BUILDER_IMPLEMENTATION
