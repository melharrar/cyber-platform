space-pcd
=========

# Latest Notes 

## Config Files
pcd.X.X.X/.config contains the default configuration
config_Q6 and config_x86 are auto-generated, and should not be modified

## Building

You must provide the full path to the MBCC cross-compiler for unknown reasons (it is already on the path, but can't be found during compilation)
It seems the CONFIG_PCD_CROSS_COMPILER_PREFIX flag in .config is in fact the default build environment, not an additional compiler. TODO, it would be nice to selectively build Q6 or x86 for testing

PCD - Process Control Daemon application and library

---

# Old Notes

attempt : 
    installed qconf :
        sudo apt-get install gridengine-client          WRONG

    installed bison
        sudo apt-get install bison


    installed flex
        sudo apt-get install flex

make defconfig                          OK
make pcd                                FAIL

    add
    #include <sys/resource.h>  
    to process.c and it works now. 

make pcd                                OK

=== now for microblaze ===
edit .config file : 
CONFIG_PCD_CROSS_COMPILER_PREFIX="microblazeel-xilinx-linux-gnu-"
CONFIG_PCD_PLATFORM_OTHER=y

that's it

---

PCD APPLICATION

The Process Control Daemon is a system level process manager, meant mostly 
for RT embedded systems running Linux. The PCD is used for booting the system
in a synchronized manner, control and monitor processes and recover from 
crashes. 

The PCD application is designed to run on an embedded target platform.
It was developed and tested on an ARM11 platform. However, the code is 
portable and was compiled and tested on in x86 and MIPS environments as well.

The PCD can be activated in full mode, or in crash-daemon only mode.
In full mode, the PCD must be given with a startup script which contains all 
the required rules (See the documentation, examples and the design document for
more details). In crash-daemon only mode, the PCD is idle and waiting for an
incoming crash notification from any process in order to display the crash
dump log. Please note that all processes which require the PCD crash-daemon
facility must link with the PCD library and register to the PCD exception
handlers.

Configuration:
The PCD project supports Kconfig configuration, similarly to the Linux Kernel.
The supported configuration commands are:
- make defconfig - Configure default PCD settings.
- make menuconfig - Configure the PCD settings in a textual menu.
- make xconfig - Configure the PCD settings in a graphical menu (requires qt3-devel library).
- make oldconfig - Configure the PCD using existing settings in .config file.

Don't forget to:
- Select the correct platform (or other if it is not supported).
- Setup your cross compiler.
- Setup your target installation directory.

Make sure that you have write permission to the destination directories!

If the process fails during configuration time and it's not because of write permission,
please make sure that you have bison and flex packages installed. These are required 
for the Kconfig engine.

For ubuntu users, use these commands to get these packages:
$ sudo apt-get install bison
$ sudo apt-get install flex

Compilation:
Run "make all" or "make pcd" to compile everything.

Installation:
Run "make install" to install the executables and libraries in the target filesystem,
according to the configuration. The binaries are also installed to bin/host and bin/target
directories for your reference.

Command line parameters for the PCD (on the target):

-f FILE, --file=FILE    : Specify PCD rules file.
-p, --print             : Print parsed configuration.
-v, --verbose           : Enable Verbose display (recommended).
-t tick, --timer-tick=t : Setup timer ticks in ms (default 200ms).
-e FILE, --errlog=FILE  : Specify error log file (in nvram)
-d, --debug             : Debug mode
-c, --crashd            : Enable crash-daemon only mode (no rules file)
-V, --version           : Display PCD version
-h, --help              : Display usage screen

The PCD application needs to run as soon as the system is up. Usually, you
should start it from init.d/rcS file, in the background.

PCD scripts:

The following contains the syntax of a rule in the script. Each rule tells
the PCD which process to start, and when. It also specifies what are the
conditions to start it, what are the conditions for successfull run, and
what to do in case of an error.

# Include a rule file (optional)
```
INCLUDE = filename.pcd

################### Start of a rule block #############################

# Index of the rule
RULE = GROUPNAME_RULENAME

# Condition to start rule, existence of one of the following
# 
# NONE          - No start condition, application is spawn immediately
# FILE filename     - The existence of a file
# RULE_COMPLETED id     - Rule id completed successfully
# NETDEVICE netdev  - The existence of a networking device
# IPC_OWNER owner   - The existence of an IPC destination point
# ENV_VAR name, value   - Value of a variable
#
START_COND = { NONE; FILE filename; PNAME pname; RULE_COMPLETED id; NETDEVICE netdev; 
               IPC_OWNER owner, STATUS script, status }

# Command with parameters, NONE for sync point
COMMAND = cmd parameters...

# Scheduling (priority) of the process
SCHED = { NICE value; FIFO value }

# Daemon flag - Process must not end
DAEMON = { YES, NO }

# Condition to end rule and move to next rule, wait for one of the following:
# 
# NONE          - No monitor on the result, just spawn application and continue.
# FILE filename     - The existence of a file
# EXIT status       - The application exited with status. Other statuses are considered failure
# NETDEVICE netdev  - The existence of a networking device
# IPC_OWNER owner   - The existence of an IPC destination point
# PROCESS_READY     - The process sent a READY event though PCD API.
# WAIT msecs        - Delay, ignore END_COND_TIMEOUT
#
END_COND = { NONE; FILE filename; EXIT status; NETDEVICE netdev; IPC_OWNER owner; 
             WAIT msecs; PROCESS_READY }

# Timeout for end condition. Fail if timeout expires. -1 if not relevant.
END_COND_TIMEOUT = msecs

# Action upon failure, do one of the following actions upon failure
# NONE - Do not take any action
# REBOOT - Reboot the system
# RESTART - Restart the rule
# EXEC_RULE id - Execute a rule
# 
FAILURE_ACTION = { NONE, REBOOT, RESTART, EXEC_RULE id }

# Rule is Active or not (To be activated later by PCD API)
ACTIVE = { YES, NO }

# <Optional> Start the process either with a direct UID number or the login name (convered to UID in runtime)
USER = { uid; username }

################### End of a rule block #############################
```

Checkout the script examples available in the documentation area.

---

PCD LIBRARY

The PCD library provides means to communicate with the PCD application,
as well as some other services, such as signal handlers (for catching 
crashes), singleton check for daemons, etc. See pcdapi header file and
documentation for more details.

Other applications can link with this library and request the PCD to start
or stop other applications, according to the logic of the target.

To use the PCD library, you need to include the pcdapi.h file in the pcd/include
directory. This directory contains all required header files. This can be done
by adding the folloing to your CFLAGS: 
CFLAGS+=-I<PCD_HEADERS_DIR>

Furthermore, you need to link your application with libpcd and libipc. This can
be done by adding the following to your LDFLAGS:
LDFLAGS+=-L<PCD_LIBS_INSTALLATION_DIR> -lpcd -lipc


---

PCD PARSER APPLICATION

The PCD parser is designed to run on the Host machine and not on the target.
The PCD parser provides the following services:
- A means to "compile" a PCD script on the host. Check and report syntax errors.
- Generate a header file, so all applications could include it. The header file
  contains all the component rule definitions which are required when calling
  the pcd API.
- Generate a dependency graph. The output of this function is a textual file in
  dot syntax, which can be displayed graphically. The graph displays the dependency
  graph of the system and the flow of the startup. It can help designers to
  design, understand and fix the way and order the system starts.
  The script is converted to graphical layout using the Graphviz tool 
  (Available for Windows/Linux): http://graphviz.org/Download.php

Command line parameters (on the host):

-f FILE, --file=FILE            Specify PCD rules file.
-g FILE, --graph=FILE           Generate a graph file.
-d [0|1|2], --display=[0|1|2]   Items to display in graph file 
                                   (Active|All|Inactive).
-o FILE, --output=FILE          Generate an output header file with rules    
                                                                  definitions.
-b DIR, --base-dir=DIR          Specify base directory on the host.
-v, --verbose                   Print parsed configuration.
-h, --help                      Print this message and exit. 

---

DOCUMENTATION

Documentation is available in the PCD's homepage in the Real-Time Embedded 
website: http://www.rt-embedded.com/pcd

The design document is available in the PCD project homepage at SourceForge:
http://sourceforge.net/projects/pcd

---

SUPPORT

Support is available in the support forum of the PCD project homepage at 
SourceForge:
http://sourceforge.net/projects/pcd

---

AUTHOR

This library and application was originally designed and written by Hai Shalom.
The code was written for Texas Instruments and exported to open source. Now it
is managed and maintained by Hai Shalom as part of the PCD Project group.


If you have any questions, comments or remarks, or want to join the PCD Project
group, please contact me:
hai@rt-embedded.com


The PCD Homepage:
http://www.rt-embedded.com/pcd/
http://sourceforge.net/projects/pcd

