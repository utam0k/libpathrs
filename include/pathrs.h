/*
 * libpathrs: safe path resolution on Linux
 * Copyright (C) 2019 Aleksa Sarai <cyphar@cyphar.com>
 * Copyright (C) 2019 SUSE LLC
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef __CBINDGEN_ALIGNED
#undef __CBINDGEN_ALIGNED
#endif
#define __CBINDGEN_ALIGNED(n) __attribute__((aligned(n)))


#ifndef LIBPATHRS_H
#define LIBPATHRS_H

/*
 * WARNING: This file was auto-generated by rust-cbindgen. Don't modify it.
 *          Instead, re-generate it with:
 *            % cbindgen -c cbindgen.toml -o include/pathrs.h
 */


#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

/**
 * The type of object being passed to "object agnostic" libpathrs functions.
 */
typedef enum {
    __PATHRS_INVALID_TYPE = 0,
    /**
     * NULL.
     */
    PATHRS_NONE = 57343,
    /**
     * `pathrs_error_t`
     */
    PATHRS_ERROR = 57344,
    /**
     * `pathrs_root_t`
     */
    PATHRS_ROOT = 57345,
    /**
     * `pathrs_handle_t`
     */
    PATHRS_HANDLE = 57346,
} pathrs_type_t;

/**
 * The backend used for path resolution within a `pathrs_root_t` to get a
 * `pathrs_handle_t`. Can be used with `pathrs_configure()` to change the
 * resolver for a `pathrs_root_t`.
 */
enum pathrs_resolver_t {
    __PATHRS_INVALID_RESOLVER = 0,
    /**
     * Use the native openat2(2) backend (requires kernel support).
     */
    PATHRS_KERNEL_RESOLVER = 61440,
    /**
     * Use the userspace "emulated" backend.
     */
    PATHRS_EMULATED_RESOLVER = 61441,
};
typedef uint16_t pathrs_resolver_t;

/**
 * This is only exported to work around a Rust compiler restriction. Consider
 * it an implementation detail and don't make use of it.
 */
typedef struct __pathrs_handle_t __pathrs_handle_t;

/**
 * This is only exported to work around a Rust compiler restriction. Consider
 * it an implementation detail and don't make use of it.
 */
typedef struct __pathrs_root_t __pathrs_root_t;

/**
 * Represents a single entry in a Rust backtrace in C. This structure is
 * owned by the relevant `pathrs_error_t`.
 */
typedef struct __CBINDGEN_ALIGNED(8) {
    /**
     * Instruction pointer at time of backtrace.
     */
    const void *ip;
    /**
     * Address of the enclosing symbol at time of backtrace.
     */
    const void *symbol_address;
    /**
     * Symbol name for @symbol_address (or NULL if none could be resolved).
     */
    const char *symbol_name;
    /**
     * Filename in which the symbol is defined (or NULL if none could be
     * resolved -- usually due to lack of debugging symbols).
     */
    const char *symbol_file;
    /**
     * Line within @symbol_file on which the symbol is defined (will only make
     * sense if @symbol_file is non-NULL).
     */
    uint32_t symbol_lineno;
} __pathrs_backtrace_entry_t;

/**
 * Represents a Rust Vec<T> in an FFI-safe way. It is absolutely critical that
 * the FFI user does not modify *any* of these fields.
 */
typedef struct __CBINDGEN_ALIGNED(8) {
    /**
     * Pointer to the head of the vector.
     */
    const __pathrs_backtrace_entry_t *head;
    /**
     * Number of elements in the vector (must not be modified).
     */
    uintptr_t length;
    /**
     * Capacity of the vector (must not be modified).
     */
    uintptr_t __capacity;
} __pathrs_backtrace_t;

/**
 * This is only exported to work around a Rust compiler restriction. Consider
 * it an implementation detail and don't make use of it.
 */
typedef __pathrs_backtrace_t pathrs_backtrace_t;

/**
 * Attempts to represent a Rust Error type in C. This structure must be freed
 * using `pathrs_free(PATHRS_ERROR)`.
 */
typedef struct __CBINDGEN_ALIGNED(8) {
    /**
     * Raw errno(3) value of the underlying error (or 0 if the source of the
     * error was not due to a syscall error).
     */
    uint64_t saved_errno;
    /**
     * Textual description of the error.
     */
    const char *description;
    /**
     * Backtrace captured at the error site (or NULL if backtraces have been
     * disabled at libpathrs build-time or through an environment variable).
     */
    pathrs_backtrace_t *backtrace;
} pathrs_error_t;

/**
 * A handle to a path within a given Root. This handle references an
 * already-resolved path which can be used for only one purpose -- to "re-open"
 * the handle and get an actual fs::File which can be used for ordinary
 * operations.
 *
 * It is critical for the safety of users of this library that *at no point* do
 * you use interfaces like libc::openat directly on file descriptors you get
 * from using this library (or extract the RawFd from a fs::File). You must
 * always use operations through a Root.
 */
typedef __pathrs_handle_t pathrs_handle_t;

/**
 * A handle to the root of a directory tree to resolve within. The only purpose
 * of this "root handle" is to get Handles to inodes within the directory tree.
 *
 * At the time of writing, it is considered a *VERY BAD IDEA* to open a Root
 * inside a possibly-attacker-controlled directory tree. While we do have
 * protections that should defend against it (for both drivers), it's far more
 * dangerous than just opening a directory tree which is not inside a
 * potentially-untrusted directory.
 */
typedef __pathrs_root_t pathrs_root_t;

/**
 * Global configuration for pathrs, for use with
 *    `pathrs_configure(PATHRS_NONE, NULL)`
 */
typedef struct __CBINDGEN_ALIGNED(8) {
    /**
     * Sets whether backtraces will be generated for errors. This is a global
     * setting, and defaults to **disabled** for release builds of libpathrs
     * (but is **enabled** for debug builds).
     */
    bool error_backtraces;
    /**
     * Extra padding fields -- must be set to zero.
     */
    uint8_t __padding[7];
} pathrs_config_global_t;

/**
 * Configuration for a specific `pathrs_root_t`, for use with
 *    `pathrs_configure(PATHRS_ROOT, <root>)`
 */
typedef struct __CBINDGEN_ALIGNED(8) {
    /**
     * Resolver used for all resolution under this `pathrs_root_t`.
     */
    pathrs_resolver_t resolver;
    /**
     * Extra padding fields -- must be set to zero.
     */
    uint16_t __padding[3];
} pathrs_config_root_t;

/**
 * Configure pathrs and its objects and fetch the current configuration.
 *
 * Given a (ptr_type, ptr) combination the provided @new_ptr configuration will
 * be applied, while the previous configuration will be stored in @old_ptr.
 *
 * If @new_ptr is NULL the active configuration will be unchanged (but @old_ptr
 * will be filled with the active configuration). Similarly, if @old_ptr is
 * NULL the active configuration will be changed but the old configuration will
 * not be stored anywhere. If both are NULL, the operation is a no-op.
 *
 * Only certain objects can be configured with pathrs_configure():
 *
 *   * PATHRS_NONE (@ptr == NULL), with pathrs_config_global_t.
 *   * PATHRS_ROOT, with pathrs_config_root_t.
 *
 * The caller *must* set @cfg_size to the sizeof the configuration type being
 * passed. This is used for backwards and forward compatibility (similar to the
 * openat2(2) and similar syscalls).
 *
 * For all other types, a pathrs_error_t will be returned (and as usual, it is
 * up to the caller to pathrs_free it).
 */
pathrs_error_t *pathrs_configure(pathrs_type_t ptr_type,
                                 void *ptr,
                                 void *old_cfg_ptr,
                                 const void *new_cfg_ptr,
                                 uintptr_t cfg_size);

pathrs_handle_t *pathrs_creat(pathrs_root_t *root,
                              const char *path,
                              unsigned int mode);

/**
 * Retrieve the error stored by a pathrs object.
 *
 * Whenever an error occurs during an operation on a pathrs object, the object
 * will store the error for retrieval with pathrs_error(). Note that performing
 * any subsequent operations will clear the stored error -- so the error must
 * immediately be fetched by the caller.
 *
 * If there is no error associated with the object, NULL is returned (thus you
 * can safely check for whether an error occurred with pathrs_error).
 *
 * It is critical that the correct pathrs_type_t is provided for the given
 * pointer (otherwise memory corruption will almost certainly occur).
 */
pathrs_error_t *pathrs_error(pathrs_type_t ptr_type, void *ptr);

/**
 * Free a libpathrs object.
 *
 * It is critical that the correct pathrs_type_t is provided for the given
 * pointer (otherwise memory corruption will almost certainly occur).
 */
void pathrs_free(pathrs_type_t ptr_type, void *ptr);

int pathrs_hardlink(pathrs_root_t *root, const char *path, const char *target);

int pathrs_mkdir(pathrs_root_t *root, const char *path, unsigned int mode);

int pathrs_mknod(pathrs_root_t *root,
                 const char *path,
                 unsigned int mode,
                 dev_t dev);

/**
 * Open a root handle.
 *
 * The default resolver is automatically chosen based on the running kernel.
 * You can switch the resolver used with pathrs_configure() -- though this
 * is not strictly recommended unless you have a good reason to do it.
 *
 * The provided path must be an existing directory. If using the emulated
 * driver, it also must be the fully-expanded path to a real directory (with no
 * symlink components) because the given path is used to double-check that the
 * open operation was not affected by an attacker.
 *
 * NOTE: Unlike other libpathrs methods, pathrs_open will *always* return a
 *       pathrs_root_t (but in the case of an error, the returned root handle
 *       will be a "dummy" which is just used to store the error encountered
 *       during setup). Errors during pathrs_open() can only be detected by
 *       immediately calling pathrs_error() with the returned root handle --
 *       and as with valid root handles, the caller must free it with
 *       pathrs_free().
 *
 *       This unfortunate API wart is necessary because there is no obvious
 *       place to store a libpathrs error when first creating an root handle
 *       (other than using thread-local storage but that opens several other
 *       cans of worms). This approach was chosen because in principle users
 *       could call pathrs_error() after every libpathrs API call.
 */
pathrs_root_t *pathrs_open(const char *path);

/**
 * Within the given root's tree, perform the rename (with all symlinks being
 * scoped to the root). The flags argument is identical to the renameat2(2)
 * flags that are supported on the system.
 */
int pathrs_rename(pathrs_root_t *root,
                  const char *src,
                  const char *dst,
                  int flags);

/**
 * "Upgrade" the handle to a usable fd, suitable for reading and writing. This
 * does not consume the original handle (allowing for it to be used many
 * times).
 *
 * It should be noted that the use of O_CREAT *is not* supported (and will
 * result in an error). Handles only refer to *existing* files. Instead you
 * need to use creat().
 *
 * In addition, O_NOCTTY is automatically set when opening the path. If you
 * want to use the path as a controlling terminal, you will have to do
 * ioctl(fd, TIOCSCTTY, 0) yourself.
 */
int pathrs_reopen(pathrs_handle_t *handle, int flags);

/**
 * Within the given root's tree, resolve the given path (with all symlinks
 * being scoped to the root) and return a handle to that path. The path *must
 * already exist*, otherwise an error will occur.
 */
pathrs_handle_t *pathrs_resolve(pathrs_root_t *root, const char *path);

int pathrs_symlink(pathrs_root_t *root, const char *path, const char *target);

#endif /* LIBPATHRS_H */

#ifdef __CBINDGEN_ALIGNED
#undef __CBINDGEN_ALIGNED
#endif

