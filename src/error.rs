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

//! Error types for libpathrs.

// NOTE: This module is mostly a workaround until several issues have been
//       resolved:
//
//  * https://github.com/shepmaster/snafu/issues/188.
//  * `std::error::Backtrace` is stabilised.
//  * `std::error::Error::chain` is stabilised.
//  * I figure out a nice way to implement GlobalBacktrace...

pub use crate::syscalls::{Error as SyscallError, FrozenFd};

use std::error::Error as StdError;
use std::io::Error as IOError;
use std::sync::atomic::{AtomicBool, Ordering};

use snafu::{GenerateBacktrace, ResultExt};

/// A wrapper around [`backtrace::Backtrace`].
///
/// The primary reason for this is that it allows for custom configuration of
/// whether backtraces are generated by libpathrs. You may configure this by
/// modifying [`BACKTRACES_ENABLED`].
///
/// # Stability
/// Note that this interface will drastically change once
/// `std::error::Backtrace` is stabilised.
///
/// [`backtrace::Backtrace`]: https://docs.rs/backtrace/*/backtrace/struct.Backtrace.html
/// [`BACKTRACES_ENABLED`]: static.BACKTRACES_ENABLED.html
// NOTE: Once std's Backtrace is finalised this will need to be changed.
#[derive(Debug)]
pub struct Backtrace(pub Option<backtrace::Backtrace>);

/// Controls whether backtraces will be generated during error handling within
/// libpathrs.
///
/// By default, backtraces are disabled for release builds and enabled otherwise.
// TODO: This should probably be a getter+setter setup but I couldn't figure out
//       nice names for the getter and setter.
pub static BACKTRACES_ENABLED: AtomicBool = AtomicBool::new(cfg!(debug_assertions));

impl GenerateBacktrace for Backtrace {
    fn generate() -> Self {
        Backtrace(match BACKTRACES_ENABLED.load(Ordering::SeqCst) {
            true => Some(backtrace::Backtrace::new()),
            false => None,
        })
    }

    fn as_backtrace(&self) -> Option<&snafu::Backtrace> {
        self.0.as_ref()
    }
}

/// The primary error type returned by libpathrs.
///
/// All public interfaces of libpathrs will return this error in `Result`s. In
/// order to enable or disable backtrace-generation for libpathrs `Error`s,
/// modify [`BACKTRACES_ENABLED`].
///
/// # Caveats
/// Until [`Error::chain`] is stabilised, it will be necessary for callers
/// to manually implement their own version of this feature.
///
/// [`BACKTRACES_ENABLED`]: static.BACKTRACES_ENABLED.html
/// [`Error::chain`]: https://doc.rust-lang.org/nightly/std/error/trait.Error.html#method.chain
#[derive(Snafu, Debug)]
#[snafu(visibility = "pub(crate)")]
pub enum Error {
    /// The requested feature is not yet implemented.
    #[snafu(display("feature '{}' not implemented", feature))]
    NotImplemented {
        /// Feature which is not implemented.
        feature: String,
        /// Backtrace captured at time of error.
        backtrace: Backtrace,
    },

    /// The requested feature is not supported by this kernel.
    #[snafu(display("feature '{}' not supported on this kernel", feature))]
    NotSupported {
        /// Feature which is not supported.
        feature: String,
        /// Backtrace captured at time of error.
        backtrace: Backtrace,
    },

    /// One of the provided arguments in invalid.
    #[snafu(display("invalid {} argument: {}", name, description))]
    InvalidArgument {
        /// Name of the invalid argument.
        name: String,
        /// Description of what makes the argument invalid.
        description: String,
        /// Backtrace captured at time of error.
        backtrace: Backtrace,
    },

    /// libpathrs has detected some form of safety requirement violation.
    /// This might be an attempted breakout by an attacker or even a bug
    /// internal to libpathrs.
    #[snafu(display("violation of safety requirement: {}", description))]
    SafetyViolation {
        /// Description of safety requirement which was violated.
        description: String,
        /// Backtrace captured at time of error.
        backtrace: Backtrace,
    },

    /// The requested libpathrs operation resulted in an [`IOError`]. This
    /// should be contrasted with [`RawOsError`] -- which indicates an error
    /// triggered by one of libpathrs's syscall wrappers.
    ///
    /// [`IOError`]: https://doc.rust-lang.org/std/io/struct.Error.html
    /// [`RawOsError`]: enum.Error.html#variant.RawOsError
    // TODO: Remove the OsError and RawOsError distinction.
    #[snafu(display("{} failed", operation))]
    OsError {
        /// Operation which was being attempted.
        operation: String,
        /// Underlying error.
        source: IOError,
        /// Backtrace captured at time of error.
        backtrace: Backtrace,
    },

    /// The requested libpathrs operation directly resulted in an operating
    /// system error. This should be contrasted with [`InternalOsError`]
    /// (which is an error triggered internally by libpathrs while servicing
    /// the user request).
    ///
    /// The requested libpathrs operation resulted in a [`SyscallError`] by
    /// one of libpathrs's syscall wrappers. This should be contrasted with
    /// [`OsError`] -- which indicates an error triggered by a Rust stdlib
    /// function.
    ///
    /// [`IOError`]: https://doc.rust-lang.org/std/io/struct.Error.html
    /// [`OsError`]: enum.Error.html#variant.OsError
    // TODO: Remove the OsError and RawOsError distinction.
    #[snafu(display("{} failed", operation))]
    RawOsError {
        /// Operation which was being attempted.
        operation: String,
        /// Underlying syscall wrapper error.
        #[snafu(backtrace)]
        source: SyscallError,
    },

    /// Wrapped represents an Error which has some simple string-wrapping
    /// information. This is used to allow for some additional context to be
    /// added at call-sites.
    // XXX: Arguably this is super ugly and we should have a separate
    //      context selector for each callsite but that's just ridiculous.
    #[snafu(display("{}", context))]
    Wrapped {
        /// Additional context information about the contained error.
        context: String,
        /// Underlying wrapped error.
        #[snafu(backtrace)]
        #[snafu(source(from(Error, Box::new)))]
        source: Box<Error>,
    },
}

// Private trait necessary to work around the "orphan trait" restriction.
pub(crate) trait ErrorExt {
    /// Wrap a `Result<..., Error>` with an additional context string.
    fn wrap<S: Into<String>>(self, context: S) -> Self;
}

impl<T> ErrorExt for Result<T, Error> {
    fn wrap<S: Into<String>>(self, context: S) -> Self {
        self.context(Wrapped {
            context: context.into(),
        })
    }
}

/// A backport of the nightly-only [`Chain`]. This method
/// will be removed as soon as that is stabilised.
///
/// [`Chain`]: https://doc.rust-lang.org/nightly/std/error/struct.Chain.html
// XXX: https://github.com/rust-lang/rust/issues/58520
pub(crate) struct Chain<'a> {
    current: Option<&'a (dyn StdError + 'static)>,
}

impl<'a> Iterator for Chain<'a> {
    type Item = &'a (dyn StdError + 'static);

    fn next(&mut self) -> Option<Self::Item> {
        let current = self.current;
        self.current = self.current.and_then(StdError::source);
        current
    }
}

impl Error {
    /// A backport of the nightly-only [`Error::chain`]. This method
    /// will be removed as soon as that is stabilised.
    ///
    /// [`Error::chain`]: https://doc.rust-lang.org/nightly/std/error/trait.Error.html#method.chain
    // XXX: https://github.com/rust-lang/rust/issues/58520
    pub(crate) fn iter_chain_hotfix(&self) -> Chain {
        Chain {
            current: Some(self),
        }
    }

    /// Shorthand for `self.iter_chain_hotfix().last()`.
    pub(crate) fn root_cause(&self) -> &(dyn StdError + 'static) {
        self.iter_chain_hotfix()
            .last()
            .expect("Error::iter_chain_hotfix() should have at least one result")
    }
}
