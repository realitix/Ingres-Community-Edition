/*
** Copyright (c) 2011 Actian Corporation. All rights reserved
**
** Description:
**	Execution wrapper that invokes the Java Virtual Machine
**	with the given .jar file name and optionally displays
**	a splash screen given the (in-memory) image data.
**
** History:
**    31-Aug-2011 (whiro01)
**	Initial version with complete code for Windows (some
**	TODO items for other platforms that will be corrected
**	as necessary).  Still have some registry issues and
**	support for JREs (different paths than JDK).
**    02-Sep-2011 (whiro01)
**	Work with finding the JVM better in JRE-only environments
**	and if JAVA_HOME is not set in the environment (on Windows);
**	add checksum parameter to Launcher "main".
**    03-Sep-2011 (whiro01)
**	Also check "jre\bin\server" directory for JVM library.
**    06-Sep-2011 (whiro01)
**	Get this to compile on OSX; create Java VM on second thread;
**	allow any platform to create second thread if necessary.
**    09-Sep-2011 (whiro01)
**	Port the threading stuff to Windows.
**    24-Sep-2011 (whiro01)
**	Finish coding Linux variations; change 'WINDOWS' to 'NT_GENERIC'.
**    25-Sep-2011 (whiro01)
**	A couple more WINDOWS -> NT_GENERIC changes and MACOSX -> OSX.
**    28-Oct-2011 (whiro01)
**	As a temporary workaround, put "-Xint" argument to JVM to fix
**	crash on Linux in KeyPairGenerator with compiled native code.
**    30-Oct-2011 (whiro01)
**	Changed 32-bit path on Linux; don't try to recurse directory
**	tree if parent directory can't be found (segmentation fault).
*/
#include <stdlib.h>
#include <stdio.h>
#include <jni.h>
#include <string>
#include "execute.h"
#include "util.h"

using namespace std;

#if defined(NT_GENERIC) || defined(WIN32)
	#include <process.h>
	typedef HANDLE pthread_t;
	typedef void * pthread_attr_t;
	#define pthread_attr_init(x)
	#define pthread_attr_setdetachstate(x, y)
	#define pthread_attr_destroy(x)
	#define APIFUNC	__stdcall
	#define USE_SECOND_THREAD	1
	#define LIB_PATH	"bin"
	#define	JVM_LIB		"jvm.dll"
	#define JNI_CREATEJAVAVM	"JNI_CreateJavaVM"
	#define SPLASH_LIB	"splashscreen.dll"
#elif defined(LINUX) || defined(UNIX)
	#include <pthread.h>
	#include <sys/types.h>
	#include <dirent.h>
	#define APIFUNC
	#define NATIVE_PTHREADS
	#define USE_SECOND_THREAD	1
    #ifdef __x86_64__
	#define LIB_PATH	"lib/amd64"
    #else
	#define LIB_PATH	"lib/i386"
    #endif
	#define	JVM_LIB		"libjvm.so"
	#define JNI_CREATEJAVAVM	"JNI_CreateJavaVM"
	#define SPLASH_LIB	"libsplashscreen.so"
#elif defined(DARWIN) || defined(OSX)
	#include <pthread.h>
	#define APIFUNC
	#define NATIVE_PTHREADS
	#define USE_SECOND_THREAD	1
	#define LIB_PATH	"Libraries"
	#if __x86_64__
		#define	JVM_LIB	"libserver.dylib"
	#else
		#define JVM_LIB	"libclient.dylib"
	#endif
	#define JNI_CREATEJAVAVM	"JNI_CreateJavaVM_Impl"
	#define SPLASH_LIB	"libsplashscreen.jnilib"
#else
	#error	Need definitions of JVM_LIB and SPLASH_LIB for this platform.
#endif

typedef jint (JNICALL *CreateJavaVMFunc)(JavaVM **pvm, void **penv, void *args);
typedef void (JNICALL *SplashInitFunc)();
typedef void (JNICALL *SplashCloseFunc)();
typedef void (JNICALL *SplashLoadMemoryFunc)(void *pdata, int size);
typedef void (JNICALL *SplashLoadFileFunc)(const char *filename);
typedef void (JNICALL *SplashSetFileJarNameFunc)(const char* filename, const char* jarname);

static int execute_in_second_thread = USE_SECOND_THREAD;

#if defined(NT_GENERIC) || defined(WIN32)
#define RTLD_LAZY 0
static char *dlerror_value = NULL;

static char *
get_win_error(void)
{
	DWORD error_code = GetLastError();
	LPSTR buffer;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error_code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&buffer,
		0, NULL);
	return buffer;
}

static void *
dlopen(const char *name, int flags)
{
	UNREFERENCED(flags);
	void *result = LoadLibrary(name);
	dlerror_value = get_win_error();
	return result;
}

static void
dlclose(void *handle)
{
	if (handle != NULL) {
	    FreeLibrary((HMODULE)handle);
	}
}

static char *
dlerror(void)
{
	/* We need to reset the error */
	char *result = dlerror_value;
	dlerror_value = NULL;
	return result;
}

static void *
dlsym(void *handle, const char *name)
{
	void *result = (void *)GetProcAddress((HMODULE)handle, name);
	dlerror_value = result ? NULL : (char *)"function not found";
	return result;
}

static void
setenv(const char *key, const char *value, bool overwrite)
{
	UNREFERENCED(overwrite);
	char *buf = (char *)malloc(strlen(key) + strlen(value) + 2);
	sprintf(buf, "%s=%s", key, value);
	putenv(buf);
}
#endif


static char *
format_option(const char *format, const char *value)
{
	char *buf = (char *)malloc(strlen(format) + strlen(value) + 10);
	sprintf(buf, format, value);
	return buf;
}


static void
show_splash(string& jre_path, const unsigned char *splash_image, unsigned int splash_size)
{
	/* If there is a splash screen specified, create a temp file
	** with the contents of the (currently in-memory) image and
	** notify the splashscreen library to display it.
	*/
	if (splash_image != NULL && splash_size != 0) {
	    string splash_path = jre_path + FILE_SEPARATOR + LIB_PATH + FILE_SEPARATOR + SPLASH_LIB;
	    if (!file_exists(splash_path.c_str()))
		return;
	    void *splashmodule = dlopen(splash_path.c_str(), RTLD_LAZY);
	    if (splashmodule == NULL)
		return;
	    SplashInitFunc SplashInit = (SplashInitFunc)dlsym(splashmodule, "SplashInit");
	    SplashInit();
	    SplashLoadMemoryFunc SplashLoadMemory = (SplashLoadMemoryFunc)dlsym(splashmodule, "SplashLoadMemory");
	    SplashLoadMemory((void *)splash_image, splash_size);
	    SplashSetFileJarNameFunc SplashSetFileJarName = (SplashSetFileJarNameFunc)dlsym(splashmodule, "SplashSetFileJarName");
	    SplashSetFileJarName(NULL, NULL);
	}
}


/* TODO: different platforms have different locations.
** TODO: JDK has different location than JRE
** TODO: use registry on Windows to find JVM if JAVA_HOME not set:
**   HKEY_LOCAL_MACHINE/Software/JavaSoft/Java Runtime Environment
**   CurrentVersion key has "1.7" e.g.
**   Java7FamilyVersion would have "1.7.0" (or like 1.6.0_24)
**   then "1.7" or "1.7.0" has these keys:
**     JavaHome has same as what JAVA_HOME should be
**       (in other words, ...\jre and jvm.dll is in JavaHome\bin\client)
**     RuntimeLib has full path of "jvm.dll"
*/
bool
find_jre_path(string& path)
{
	bool found = false;
	string java_home;
	const char *env_java_home = getenv("JAVA_HOME");
	if (env_java_home == NULL || *env_java_home == '\0') {
#if defined(NT_GENERIC) || defined(WIN32)
	    // Try some things
	    while (!found) {
		const char *program_files = getenv("ProgramFiles");
		if (program_files == NULL || *program_files == '\0') {
		    // TODO: look in the registry
		    break;
		}
		else {
		    string progfilesjava = string(program_files) + FILE_SEPARATOR + "Java";
		    if (dir_exists(progfilesjava.c_str())) {
			// Look backward to find the newest version
			for (int i = 9; i > 5; i--) {
			    char jrename[10];
			    sprintf(jrename, "jre%d", i);
			    string jre_dir = progfilesjava + FILE_SEPARATOR + jrename;
			    if (dir_exists(jre_dir.c_str())) {
				found = true;
				java_home = jre_dir;
				break;
			    }
			    // TODO: look for JDK directories also
			    // but these names look like:
			    // jdk1.6.0_16
			    // so we need a directory search
			}
		    }
		    else {
			// TODO: look in the registry
			break;
		    }
		}
	    }
#elif defined(DARWIN) || defined(OSX)
	    while (!found) {
		const char *framework = "/System/Library/Frameworks/JavaVM.framework";
		if (dir_exists(framework)) {
		    java_home = framework;
		    found = true;
		}
		else {
		    // TODO: ??? I don't know where else to try looking
		    break;
		}
	    }
#elif defined(LINUX)
#else
	#error Need code to find Java Home on this platform!
#endif
	}
	else {
	    java_home = string(env_java_home);
	    found = true;
	}
	if (found) {
	    // java_home could be pointing to a plain JRE or to a JDK
	    // Try the JDK form first
	    path = java_home + FILE_SEPARATOR + "jre";
	    if (!dir_exists(path.c_str())) {
		// Probably a plain JRE
		path = java_home;
	    }
	}
	return found;
}


bool
check_jvm_lib_path(string& jre_path, const char *dir, const char *jvm_lib_name, string& jvm_lib_path) {
	jvm_lib_path = jre_path + FILE_SEPARATOR + LIB_PATH + FILE_SEPARATOR + dir +
		FILE_SEPARATOR + jvm_lib_name;
	return file_exists(jvm_lib_path.c_str());
}

bool
find_jvm_lib(string& jre_path, const char *jvm_lib_name, string& jvm_lib_path) {
#if defined(NT_GENERIC) || defined(WIN32) || defined(LINUX)
	if (check_jvm_lib_path(jre_path, "client", jvm_lib_name, jvm_lib_path))
	    return true;
	if (check_jvm_lib_path(jre_path, "server", jvm_lib_name, jvm_lib_path))
	    return true;
#if defined(LINUX)
	// Check other known locations
	if (check_jvm_lib_path(jre_path, "default", jvm_lib_name, jvm_lib_path))
	    return true;
	string rootpath = jre_path + FILE_SEPARATOR + LIB_PATH;
	DIR *dirp = opendir(rootpath.c_str());
	if (dirp != NULL) {
	    struct dirent *pentry;
	    while ((pentry = readdir(dirp)) != NULL) {
		if (dir_exists(pentry->d_name)) {
		    if (check_jvm_lib_path(jre_path, pentry->d_name, jvm_lib_name, jvm_lib_path)) {
			closedir(dirp);
			return true;
		    }
		}
	    }
	    closedir(dirp);
	}
#endif
#elif defined(DARWIN) || defined(OSX)
	jvm_lib_path = jre_path + FILE_SEPARATOR + LIB_PATH
		+ FILE_SEPARATOR + jvm_lib_name;
	return (file_exists(jvm_lib_path.c_str()));
#else
	#error Must implement 'find_jvm_lib' on this platform!
#endif
	return false;
}

typedef struct _threadargs {
	const char *jar_file;
	const char *checksum;
	const unsigned char *splash_image;
	unsigned int splash_size;
	int argc;
	char** argv;
} threadargs;

void* APIFUNC
execute_java_in_second_thread(void *arg)
{
	JNIEnv *env;
	JavaVM *jvm;
	jint res;
	jclass cls;
	jmethodID mainMid;
	jstring jstr;
	jstring jchecksum;
	jclass stringClass;
	jobjectArray args;
	int i;
	int ret = 0;
	JavaVMInitArgs vm_args;
	JavaVMOption options[20];
	char newpath[4096];
	void *jvmModule;
	CreateJavaVMFunc CreateJavaVM = NULL;
	string jre_path;
	string jvm_lib_path;

	// Unpack the arguments from the passed-in structure
	threadargs* t_args = (threadargs*)arg;
	const char *jar_file = t_args->jar_file;
	const char *checksum = t_args->checksum;
	const unsigned char *splash_image = t_args->splash_image;
	unsigned int splash_size = t_args->splash_size;
	int argc = t_args->argc;
	char **argv = t_args->argv;

	// Look for an appropriate Java Runtime Environment on this machine
	if (!find_jre_path(jre_path)) {
	    fprintf(stderr, "Could not find an installed version of Java!\n");
	    return (void*)(1);
	}
	/* Make path to appropriate JRE locations */
	if (!find_jvm_lib(jre_path, JVM_LIB, jvm_lib_path)) {
	    fprintf(stderr, "Cannot find %s!\n", JVM_LIB);
	    return (void*)(1);
	}

	const char *env_path = getenv(PATH_VAR);
	if (env_path == NULL)
	    sprintf(newpath, "%s%c%s",
		    jre_path.c_str(), FILE_SEPARATOR,
		    LIB_PATH
		    );
	else
	    sprintf(newpath, "%s%c%s%c%s",
		    env_path, PATH_SEPARATOR,
		    jre_path.c_str(), FILE_SEPARATOR,
		    LIB_PATH
		    );
	setenv(PATH_VAR, newpath, true);

	jvmModule  = dlopen(jvm_lib_path.c_str(), RTLD_LAZY);
	if (jvmModule == NULL) {
	    fprintf(stderr, "Cannot load \"%s\": %s\n", jvm_lib_path.c_str(), dlerror());
	    return (void*)(1);
	}
	CreateJavaVM = (CreateJavaVMFunc)dlsym(jvmModule, JNI_CREATEJAVAVM);
	if (CreateJavaVM == NULL) {
	    fprintf(stderr, "Cannot find \"%s\" function: %s\n", JNI_CREATEJAVAVM, dlerror());
	    return (void*)(1);
	}
	if (!file_exists(jar_file)) {
	    fprintf(stderr, "Cannot find \"%s\" file!\n", jar_file);
	    return (void*)(1);
	}

	/* TODO: parse Java options and add to vm_args */
	// TODO: look through arguments to see if second thread use is specified
	int nOptions = 0;
	char *classPath = getenv("CLASSPATH");
	if (classPath != NULL && *classPath != '\0')
	{
	    options[nOptions].optionString = format_option("-Djava.class.path=%s", classPath);
	    options[nOptions++].extraInfo = NULL;
	}
#if defined(LINUX)
	options[nOptions].optionString = "-Xint";
	options[nOptions++].extraInfo = NULL;
#endif
	options[nOptions].optionString = format_option("-Djava.class.path=%s", jar_file);
	options[nOptions++].extraInfo = NULL;
	options[nOptions].optionString = format_option("-Dsun.java.command=%s", jar_file);
	options[nOptions++].extraInfo = NULL;
	vm_args.version = JNI_VERSION_1_6;
	vm_args.options = options;
	vm_args.nOptions = nOptions;
	vm_args.ignoreUnrecognized = JNI_TRUE;

	/* Create the Java VM */
	res = CreateJavaVM(&jvm, (void**)&env, &vm_args);
	if (res < 0) {
	    fprintf(stderr, "Can't create Java VM\n");
	    return (void*)(1);
	}

	/* Show splash screen (if defined). */
	show_splash(jre_path, splash_image, splash_size);

	/* Get the launcher class and run the only public method. */
	cls = env->FindClass("jarfile/Launcher");
	if (cls == NULL) {
	    ret = 1;
	    goto destroy;
	}

	/* Find the Launcher "main" method. */
	mainMid = env->GetStaticMethodID(cls, "main",
		"(Ljava/lang/String;[Ljava/lang/String;)V");
	if (mainMid == NULL) {
	    ret = 1;
	    goto destroy;
	}

	/* Create Java strings from the argument list (convert to UTF-8 first) */
	stringClass = env->FindClass("java/lang/String");
	args = env->NewObjectArray(argc - 1, stringClass, NULL);
	if (args == NULL) {
	    ret = 1;
	    goto destroy;
	}
	for (i = 1; i < argc; i++) {
	    size_t len = mbs_to_utf8(NULL, argv[i], 0);
	    char *buf = (char *)malloc(len + 1);
	    if (buf == NULL) {
		ret = 1;
		goto destroy;
	    }
	    mbs_to_utf8(buf, argv[i], len + 1);
	    jstr = env->NewStringUTF(buf);
	    free(buf);
	    if (jstr == NULL) {
		ret = 1;
		goto destroy;
	    }
	    env->SetObjectArrayElement(args, i - 1, jstr);
	}
	jchecksum = env->NewStringUTF(checksum);

	/* Call the main method, which will run the whole application. */
	env->CallStaticVoidMethod(cls, mainMid, jchecksum, args);

destroy:
	if (env->ExceptionOccurred()) {
	    env->ExceptionDescribe();
	    ret = 1;
	}
	jvm->DestroyJavaVM();
	if (jvmModule != NULL) {
	    dlclose(jvmModule);
	    jvmModule = NULL;
	}
	return (void*)(ret);
}

#ifndef NATIVE_PTHREADS
#if defined(NT_GENERIC) || defined(WIN32)
static int
pthread_create(pthread_t* ptid, pthread_attr_t* pattr, void* (APIFUNC *start_address)(void *), void* arglist)
{
	UNREFERENCED(pattr);
	// TODO: stack size to be specified on command line
	uintptr_t tid = _beginthreadex(NULL, 0, reinterpret_cast<unsigned (APIFUNC *)(void *)>(start_address), arglist, 0, NULL);
	if (tid == -1L)
	    return errno;
	*ptid = (pthread_t)tid;
	return 0;
}

static int
pthread_join(pthread_t tid, void **ret)
{
	DWORD retv;
	WaitForSingleObject(tid, INFINITE);
	GetExitCodeThread(tid, &retv);
	*ret = *((void**)&retv);
	return 0;
}
#endif
#endif

int
execute_java(
	const char *jar_file,
	const char *checksum,
	const unsigned char *splash_image,
	unsigned int splash_size,
	int argc,
	char** argv)
{
	void *ret;
	// Pack the arguments into the "threadargs" structure
	threadargs t_args;
	t_args.jar_file = jar_file;
	t_args.checksum = checksum;
	t_args.splash_image = splash_image;
	t_args.splash_size = splash_size;
	t_args.argc = argc;
	t_args.argv = argv;

	if (execute_in_second_thread) {
	    pthread_t tid;
	    pthread_attr_t attr;
	    pthread_attr_init(&attr);
	    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	    int rc = pthread_create(&tid, &attr, execute_java_in_second_thread, &t_args);
	    if (rc != 0) {
		fprintf(stderr, "Error %d creating second thread!\n", rc);
		return (1);
	    }
	    pthread_attr_destroy(&attr);
	    pthread_join(tid, &ret);
	}
	else {
	    ret = execute_java_in_second_thread(&t_args);
	}
	return *((int*)&ret);
}

