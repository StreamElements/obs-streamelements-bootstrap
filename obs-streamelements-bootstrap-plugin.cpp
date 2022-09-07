#include "obs-streamelements-bootstrap-plugin.hpp"

#include "obs-streamelements-bootstrap-config.h"

#include <windows.h>
#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <thread>
#include <mutex>
#include <fstream>
#include <regex>
#include <ctime>

#ifdef APPLE
#include <sys/types.h>
#include <unistd.h>
#endif

static HINSTANCE g_hinstDLL = NULL;
static HMODULE g_loadedDLL = NULL;
static std::string g_locale = "en-US";

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpvReserved )  // reserved
{
	g_hinstDLL = hinstDLL;

	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

/* Defines common functions (required) */
OBS_DECLARE_MODULE()

/* Implements common ini-based locale (optional) */
// OBS_MODULE_USE_DEFAULT_LOCALE("obs-streamelements", "en-US")

///
// Replace substring
//
// @param input		input text
// @param what		text to replace
// @param with		text to replace with
// @return resulting string
//
static std::string replace_substr(std::string input, const char *what,
				   const char *with)
{
	std::string result = input;

	size_t what_len = strlen(what);

	// While we can find @what, replace it with @with
	for (size_t index = result.find(what, 0); index != std::string::npos;
	     index = result.find(what, 0)) {
		result = result.replace(index, what_len, with);
	}

	return result;
}

void remove_ext(std::wstring& str) {
	size_t index = str.find_last_of(L'.');

	str[index] = 0;
}

typedef void (*obs_module_set_pointer_func_t)(obs_module_t *);
typedef bool (*obs_module_load_func_t)(void);
typedef void (*obs_module_post_load_func_t)(void);
typedef void (*obs_module_unload_func_t)(void);
typedef void (*obs_module_set_locale_func_t)(const char*);
typedef void (*obs_module_free_locale_func_t)(void);
typedef int64_t (*streamelements_get_version_number_func_t)(void);

extern "C" bool obs_module_load(void)
{
	std::wstring path(32768, L'\0');

	DWORD actual_path_len = GetModuleFileNameW(g_hinstDLL, &(path[0]),
						    (DWORD)path.capacity());

	if (actual_path_len == 0) {
		blog(LOG_ERROR,
		     "obs-streamelements-bootstrap: GetModuleFileNameW call failed");

		return false;
	} else {
		path.resize((size_t)actual_path_len);
	}

	remove_ext(path);

	std::wstring path_qt5 = path.c_str();
	path_qt5.append(
		L".qt5.dymod."); // traing point prevents LoadLibraryW from adding ".DLL" to the file name

	std::wstring path_qt6 = path.c_str();
	path_qt6.append(
		L".qt6.dymod."); // traing point prevents LoadLibraryW from adding ".DLL" to the file name

	if (obs_get_version() < MAKE_SEMANTIC_VERSION(28, 0, 0)) {
		// Only for OBS major version below 28, try loading Qt5-purposed plugin flavor.
		// OBS 28+ comes with Qt6, so if OBS major version is at least 28, we can safely assume that Qt6 is required.
		// This check is necessary to prevent Qt5 DLLs from accidentally loading from other programs' paths, and breaking OBS.

		blog(LOG_INFO,
		     "obs-streamelements-bootstrap: trying to load Qt5 flavor of a plugin for OBS version %s",
		     obs_get_version_string());

		g_loadedDLL = LoadLibraryW(path_qt5.c_str());
	} else {
		blog(LOG_INFO,
		     "obs-streamelements-bootstrap: skipping trying to load Qt5 flavor of a plugin for OBS version %s",
		     obs_get_version_string());
	}

	if (!g_loadedDLL) {
		g_loadedDLL = LoadLibraryW(path_qt6.c_str());
	}

	if (g_loadedDLL == NULL) {
		blog(LOG_ERROR,
		     "obs-streamelements-bootstrap: failed loading module '%S' error %lu", path.c_str(), GetLastError());

		return false;
	}

	blog(LOG_INFO,
	     "obs-streamelements-bootstrap: loaded module: %S", path.c_str());

	obs_module_set_pointer_func_t obs_module_set_pointer_func =
		(obs_module_set_pointer_func_t)GetProcAddress(
			g_loadedDLL, "obs_module_set_pointer");

	if (obs_module_set_pointer_func) {
		obs_module_set_pointer_func(obs_module_pointer);
	}

	obs_module_set_locale_func_t obs_module_set_locale_func =
		(obs_module_set_locale_func_t)GetProcAddress(
			g_loadedDLL, "obs_module_set_locale");

	if (obs_module_set_locale_func) {
		obs_module_set_locale_func(g_locale.c_str());
	}

	obs_module_load_func_t obs_module_load_func =
		(obs_module_load_func_t)GetProcAddress(
			g_loadedDLL, "obs_module_load");

	bool result = false;
	if (obs_module_load_func) {
		result = obs_module_load_func();
	}

	blog(LOG_INFO, "obs-streamelements-bootstrap: initialized: %S",
	     path.c_str());

	return true;
}

extern "C" void obs_module_post_load(void)
{
	if (g_loadedDLL) {
		obs_module_post_load_func_t func =
			(obs_module_post_load_func_t)GetProcAddress(g_loadedDLL, "obs_module_post_load");

		if (func) {
			(func)();
		}
	}
}

extern "C" void obs_module_unload(void)
{
	if (g_loadedDLL) {
		obs_module_unload_func_t func =
			(obs_module_unload_func_t)GetProcAddress(
				g_loadedDLL, "obs_module_unload");

		if (func) {
			func();
		}

		// FreeLibrary(g_loadedDLL);

		// g_loadedDLL = NULL;
	}
}

extern "C" void obs_module_set_locale(const char *locale)
{
	g_locale = locale;

	if (g_loadedDLL) {
		obs_module_set_locale_func_t func =
			(obs_module_set_locale_func_t)GetProcAddress(
				g_loadedDLL, "obs_module_set_locale");

		if (func) {
			(func)(g_locale.c_str());
		}
	}
}

extern "C" void obs_module_free_locale(void)
{
	if (g_loadedDLL) {
		obs_module_free_locale_func_t func =
			(obs_module_free_locale_func_t)GetProcAddress(
				g_loadedDLL, "obs_module_free_locale");

		if (func) {
			(func)();
		}
	}
}

MODULE_EXPORT int64_t streamelements_get_version_number(void)
{
	if (g_loadedDLL) {
		streamelements_get_version_number_func_t func =
			(streamelements_get_version_number_func_t)GetProcAddress(
				g_loadedDLL, "streamelements_get_version_number");

		if (func) {
			return func();
		}
	}

	return (int64_t)0L;
}
