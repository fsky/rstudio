/*
 * Process.hpp
 *
 * Copyright (C) 2009-11 by RStudio, Inc.
 *
 * This program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */


#ifndef CORE_SYSTEM_PROCESS_HPP
#define CORE_SYSTEM_PROCESS_HPP

#include <vector>

#include <boost/optional.hpp>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <core/system/Types.hpp>
#include <core/FilePath.hpp>

namespace core {

class Error;

namespace system {

////////////////////////////////////////////////////////////////////////////////
//
// Run child process synchronously
//
//

// Struct for speicfying pseudoterminal options
struct Pseudoterminal
{
   Pseudoterminal(int cols, int rows)
      : cols(cols), rows(rows)
   {
   }

   int cols;
   int rows;
};

// Struct for specifying process options
struct ProcessOptions
{
   ProcessOptions()
#ifdef _WIN32
      : terminateChildren(false),
        detachProcess(false),
        redirectStdErrToStdOut(false)
#else
      : terminateChildren(false),
        detachSession(false),
        redirectStdErrToStdOut(false)
#endif
   {
   }

   // environment variables to set for the child process
   // if you want to simply merge in some additional environment
   // variables you can use the helper functions in Environment.hpp
   // to derive the desired environment
   boost::optional<Options> environment;

   // terminate should also terminate all children owned by the process
   // NOTE: currently only supported on posix -- in the posix case this
   // results in a call to ::setpgid(0,0) to create a new process group
   // and the specification of -pid to kill so as to kill the child and
   // all of its subprocesses
   // NOTE: to support the same behavior on Win32 we'll need to use
   // CreateJobObject/CREATE_BREAKAWAY_FROM_JOB to get the same effect
   bool terminateChildren;

#ifndef _WIN32
   // Calls ::setsid after fork for POSIX (no effect on Windows)
   bool detachSession;

   // attach the child process to pseudoterminal pipes
   boost::optional<Pseudoterminal> pseudoterminal;
#endif

#ifdef _WIN32
   // Creates the process with DETACHED_PROCESS on Win32 (no effect on POSIX)
   bool detachProcess;

   // If true, uses ConsoleIO.exe to capture low-level console input and output
   // (that cannot be accessed by redirecting stdin/stdout). This is not
   // recommended unless absolutely necessary as it introduces a lot of
   // complexity.
   //
   // If true, detachProcess and redirectStdErrToStdOut are ignored.
   bool lowLevelConsoleIO;
#endif

   bool redirectStdErrToStdOut;

   // function to run within the child process immediately after the fork
   // NOTE: only supported on posix as there is no fork on Win32
   boost::function<void()> onAfterFork;

   core::FilePath workingDir;
};

// Struct for returning output and exit status from a process
struct ProcessResult
{
   ProcessResult() : exitStatus(-1) {}

   // Standard output from process
   std::string stdOut;

   // Standard error from process
   std::string stdErr;

   // Process exit status. Potential values:
   //   0   - successful execution
   //   1   - application defined failure code (1, 2, 3, etc.)
   //  15   - process killed by terminate()
   //  -1   - unable to determine exit status
   int exitStatus;
};


// Run a program synchronously. Note that if executable is not an absolute
// path then runProgram will duplicate the actions of the shell in searching
// for an executable to run. Some platform specific notes:
//
//  - Posix: The executable path is not executed by /bin/sh, rather it is
//    executed directly by ::execvp. This means that shell metacharacters
//    (e.g. stream redirection, piping, etc.) are not supported in the
//    command string.
//
//  - Win32: The search for the executable path includes auto-appending .exe
//    and .cmd (in that order) for the path search and invoking cmd.exe if
//    the target is a batch (.cmd) file.
//
Error runProgram(const std::string& executable,
                 const std::vector<std::string>& args,
                 const std::string& input,
                 const ProcessOptions& options,
                 ProcessResult* pResult);

// Run a command synchronously. The command will be passed to and executed
// by a command shell (/bin/sh on posix, cmd.exe on windows).
//
Error runCommand(const std::string& command,
                 const ProcessOptions& options,
                 ProcessResult* pResult);

Error runCommand(const std::string& command,
                 const std::string& input,
                 const ProcessOptions& options,
                 ProcessResult* pResult);


////////////////////////////////////////////////////////////////////////////////
//
// ProcessSupervisor -- run child processes asynchronously
//
// Any number of processes can be run by calling runProgram or runCommand and
// their results will be delivered using the provided callbacks. Note that
// the poll() method must be called periodically (e.g. during standard event
// pumping / idle time) in  order to check for output & status of children.
//
// If you want to pair a call to runProgam or runCommand with an object which
// will live for the lifetime of the child process you should create a
// shared_ptr to that object and then bind the applicable members to the
// callback function(s) -- the bind will keep the shared_ptr alive (see the
// implementation of the single-callback version of runProgram for an example)
//
//

// Operations that can be performed from within ProcessCallbacks
class ProcessOperations
{
public:
   virtual ~ProcessOperations() {}

   // Write (synchronously) to standard input
   virtual Error writeToStdin(const std::string& input, bool eof) = 0;

   // Operations which apply to Pseudoterminals (only available
   // if ProcessOptions::pseudoterminal is specified)
   virtual Error ptySetSize(int cols, int rows) = 0;
   virtual Error ptyInterrupt() = 0;

   // Terminate the process (SIGTERM)
   virtual Error terminate() = 0;
};

// Callbacks for reporting various states and streaming output (note that
// all callbacks are optional)
struct ProcessCallbacks
{
   // Called after the process begins running (note: is called during
   // the first call to poll and therefore after runAsync returns). Can
   // be used for writing initial standard input to the child
   boost::function<void(ProcessOperations&)> onStarted;

   // Called periodically (at whatever interval poll is called) during the
   // lifetime of the child process (will not be called until after the
   // first call to onStarted). If it returns false then the child process
   // is terminated.
   boost::function<bool(ProcessOperations&)> onContinue;

   // Streaming callback for standard output
   boost::function<void(ProcessOperations&, const std::string&)> onStdout;

   // Streaming callback for standard error
   boost::function<void(ProcessOperations&, const std::string&)> onStderr;

   boost::function<void(ProcessOperations&, const std::vector<char>&)>
                                                  onConsoleOutputSnapshot;

   // Called if an IO error occurs while reading from standard streams. The
   // default behavior if no callback is specified is to log and then terminate
   // the child (which will result in onExit being called w/ exitStatus == 15)
   boost::function<void(ProcessOperations&,const Error&)> onError;

   // Called after the process has exited. Passes exitStatus (see ProcessResult
   // comment above for potential values)
   boost::function<void(int)> onExit;
};


// Process supervisor
class ProcessSupervisor : boost::noncopyable
{
public:
   ProcessSupervisor();
   virtual ~ProcessSupervisor();

   // Run a child asynchronously, invoking callbacks as the process starts,
   // produces output, and exits. Output callbacks are streamed/interleaved,
   // but note that output is collected at a polling interval so it is
   // possible that e.g. two writes to standard output which had an
   // intervening write to standard input might still be concatenated. See
   // comment on runProgram above for the semantics of the "executable"
   // argument.
   Error runProgram(const std::string& executable,
                    const std::vector<std::string>& args,
                    const ProcessOptions& options,
                    const ProcessCallbacks& callbacks);

   // Run a command asynchronously (same as above but uses a command shell
   // rather than running the executable directly)
   Error runCommand(const std::string& command,
                    const ProcessOptions& options,
                    const ProcessCallbacks& callbacks);

   // Run a child asynchronously, invoking the completed callback when the
   // process exits. Note that if input is provided then then the standard
   // input stream is closed (so EOF is sent) after the input is written.
   // Note also that the standard error handler (log and terminate) is also
   // used. If you want more customized behavior then you can use the more
   // granular runProgram call above. See comment on runProgram above for the
   // semantics of the "command" argument.
   Error runProgram(
            const std::string& executable,
            const std::vector<std::string>& args,
            const std::string& input,
            const ProcessOptions& options,
            const boost::function<void(const ProcessResult&)>& onCompleted);

   // Run a command asynchronously (same as above but uses a command shell
   // rather than running the executable directly)
   Error runCommand(
            const std::string& command,
            const ProcessOptions& options,
            const boost::function<void(const ProcessResult&)>& onCompleted);

   Error runCommand(
            const std::string& command,
            const std::string& input,
            const ProcessOptions& options,
            const boost::function<void(const ProcessResult&)>& onCompleted);


   // Check whether any children are currently active
   bool hasRunningChildren();

   // Poll for child (output and exit) events. returns true if there
   // are still children being supervised after the poll
   bool poll();

   // Terminate all running children
   void terminateAll();

   // Wait for all children to exit. Returns false if the operaiton timed out
   bool wait(
      const boost::posix_time::time_duration& pollingInterval =
         boost::posix_time::milliseconds(100),
      const boost::posix_time::time_duration& maxWait =
         boost::posix_time::time_duration(boost::posix_time::not_a_date_time));

private:
   struct Impl;
   boost::scoped_ptr<Impl> pImpl_;
};

} // namespace system
} // namespace core

#endif // CORE_SYSTEM_PROCESS_HPP
