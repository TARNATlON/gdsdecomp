/**
 * MIT License
 *
 * Copyright (c) 2019 Pranav, 2023 Nikita Lita
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "glob.h"
#include "core/io/dir_access.h"
#include "core/templates/hash_map.h"
#include "gdre_settings.h"
#include "modules/regex/regex.h"
#include <functional>
#include <regex>

namespace {

// SPECIAL_CHARS
// closing ')', '}' and ']'
// '-' (a range in character set)
// '&', '~', (extended character set operations)
// '#' (comment) and WHITESPACE (ignored) in verbose mode
static String special_characters = "()[]{}?*+-|^$\\.&~# \t\n\r\v\f";

HashMap<char32_t, String> _init_map() {
	HashMap<char32_t, String> map;
	for (int i = 0; i < special_characters.length(); i++) {
		auto sc = special_characters[i];
		map.insert(
				static_cast<char32_t>(sc), String{ "\\" } + sc);
	}
	return map;
}

static HashMap<char32_t, String> special_characters_map = _init_map();

String translate(const String &pattern) {
	std::size_t i = 0, n = pattern.length();
	String result_string;

	while (i < n) {
		auto c = pattern[i];
		i += 1;
		if (c == '*') {
			result_string += ".*";
		} else if (c == '?') {
			result_string += ".";
		} else if (c == '[') {
			auto j = i;
			if (j < n && pattern[j] == '!') {
				j += 1;
			}
			if (j < n && pattern[j] == ']') {
				j += 1;
			}
			while (j < n && pattern[j] != ']') {
				j += 1;
			}
			if (j >= n) {
				result_string += "\\[";
			} else {
				auto stuff = pattern.substr(i, j);
				if (stuff.find("--") == String::npos) {
					stuff.replace(String{ "\\" }, String{ R"(\\)" });
				} else {
					Vector<String> chunks;
					std::size_t k = 0;
					if (pattern[i] == '!') {
						k = i + 2;
					} else {
						k = i + 1;
					}

					while (true) {
						size_t off = k;
						k = pattern.substr(off, j).find("-");
						if (k == -1) {
							break;
						}
						k += off;
						chunks.push_back(pattern.substr(i, k));
						i = k + 1;
						k = k + 3;
					}

					chunks.push_back(pattern.substr(i, j));
					// Escape backslashes and hyphens for set difference (--).
					// Hyphens that create ranges shouldn't be escaped.
					bool first = true;
					for (auto &s : chunks) {
						s.replace(String{ "\\" }, String{ R"(\\)" });
						s.replace(String{ "-" }, String{ R"(\-)" });
						if (first) {
							stuff += s;
							first = false;
						} else {
							stuff += "-" + s;
						}
					}
				}

				// Escape set operations (&&, ~~ and ||).
				String result;
				Ref<RegEx> escapere = RegEx::create_from_string(R"([&~|])");
				escapere->sub(stuff, R"(\\\1)", true);
				stuff = result;
				i = j + 1;
				if (stuff[0] == '!') {
					stuff = "^" + stuff.substr(1);
				} else if (stuff[0] == '^' || stuff[0] == '[') {
					stuff = "\\\\" + stuff;
				}
				result_string = result_string + "[" + stuff + "]";
			}
		} else {
			if (special_characters.find_char(c) != String::npos) {
				result_string += special_characters_map[static_cast<char32_t>(c)];
			} else {
				result_string += c;
			}
		}
	}
	return String{ "((" } + result_string + String{ R"()|[\r\n])$)" };
}

Vector<String> filter(const Vector<String> &names,
		const String &pattern) {
	// std::cout << "Pattern: " << pattern << "\n";
	Vector<String> result;
	for (auto &name : names) {
		// std::cout << "Checking for " << name.string() << "\n";
		if (Glob::fnmatch(name, pattern)) {
			result.push_back(name);
		}
	}
	return result;
}

bool has_magic(const String &pathname) {
	static const auto magic_check = std::regex("([*?[])");
	return std::regex_search(pathname.utf8().get_data(), magic_check);
}

bool is_hidden(const String &pathname) {
	return pathname[0] == '.';
}

bool is_recursive(const String &pattern) {
	return pattern == "**";
}

Vector<String> iter_directory(const String &dir, bool dironly, bool include_hidden) {
	Error err;
	Ref<DirAccess> da = DirAccess::open(dir, &err);
	ERR_FAIL_COND_V_MSG(da.is_null(), Vector<String>(), "Failed to open directory " + dir);
	da->set_include_hidden(include_hidden);
	Vector<String> ret = da->get_directories();
	if (!dironly) {
		ret.append_array(da->get_files());
	}
	if (dir.is_absolute_path()) {
		for (int i = 0; i < ret.size(); i++) {
			ret.ptrw()[i] = dir.path_join(ret[i]);
		}
	}
	return ret;
}

// Recursively yields relative pathnames inside a literal directory.
Vector<String> rlistdir(const String &dirname, bool dironly, bool include_hidden) {
	Vector<String> result;

	auto names = iter_directory(dirname, dironly, include_hidden);
	for (auto &x : names) {
		result.push_back(x);
		for (auto &y : rlistdir(x, dironly, include_hidden)) {
			if (!dirname.is_absolute_path()) {
				y = x.path_join(y);
			}
			result.push_back(y);
		}
	}
	return result;
}

// This helper function recursively yields relative pathnames inside a literal
// directory.
Vector<String> glob2(const String &dirname, [[maybe_unused]] const String &pattern,
		bool dironly, bool include_hidden) {
	// std::cout << "In glob2\n";
	Vector<String> result;
	//assert(is_recursive(pattern));
	for (auto &dir : rlistdir(dirname, dironly, include_hidden)) {
		result.push_back(dir);
	}
	return result;
}

// These 2 helper functions non-recursively glob inside a literal directory.
// They return a list of basenames.  _glob1 accepts a pattern while _glob0
// takes a literal basename (so it only has to check for its existence).

Vector<String> glob1(const String &dirname, const String &pattern,
		bool dironly, bool include_hidden) {
	// std::cout << "In glob1\n";
	auto names = iter_directory(dirname, dironly, include_hidden);
	Vector<String> filtered_names;
	for (auto &n : names) {
		if (!is_hidden(n)) {
			filtered_names.push_back(n.get_file());
		}
	}
	return filter(filtered_names, pattern);
}

Vector<String> glob0(const String &dirname, const String &basename,
		bool dironly, bool include_hidden) {
	// std::cout << "In glob0\n";
	Vector<String> result;
	if (basename.is_empty()) {
		// 'q*x/' should match only directories.
		auto da = DirAccess::open(dirname);
		if (da.is_valid()) {
			da->set_include_hidden(include_hidden);
			if (da->current_is_dir()) {
				result = { basename };
			}
		}
	} else {
		if (DirAccess::exists(dirname.path_join(basename))) {
			result = { basename };
		}
	}
	return result;
}

Vector<String> _glob(const String &inpath, bool recursive = false,
		bool dironly = false, bool include_hidden = false) {
	Vector<String> result;

	const auto pathname = inpath;
	auto path = pathname;

	// if (pathname[0] == '~') {
	// 	// expand tilde
	// 	path = expand_tilde(path);
	// }

	auto dirname = pathname.get_base_dir();
	const auto basename = pathname.get_file();

	if (!has_magic(pathname)) {
		//assert(!dironly);
		if (!basename.is_empty()) {
			if (DirAccess::exists(path)) {
				result.push_back(path);
			}
		} else {
			// Patterns ending with a slash should match only directories
			if (DirAccess::open(dirname)->current_is_dir()) {
				result.push_back(path);
			}
		}
		return result;
	}

	if (dirname.is_empty()) {
		if (recursive && is_recursive(basename)) {
			return glob2(dirname, basename, dironly, include_hidden);
		} else {
			return glob1(dirname, basename, dironly, include_hidden);
		}
	}

	Vector<String> dirs;
	if (dirname != pathname && has_magic(dirname)) {
		dirs = _glob(dirname, recursive, true, include_hidden);
	} else {
		dirs = { dirname };
	}

	std::function<Vector<String>(const String &, const String &, bool, bool)>
			glob_in_dir;
	if (has_magic(basename)) {
		if (recursive && is_recursive(basename)) {
			glob_in_dir = glob2;
		} else {
			glob_in_dir = glob1;
		}
	} else {
		glob_in_dir = glob0;
	}

	for (auto &d : dirs) {
		for (auto &name : glob_in_dir(d, basename, dironly, include_hidden)) {
			String subresult = name;
			if (name.get_base_dir().is_empty()) {
				subresult = d.path_join(name);
			}
			result.push_back(subresult);
		}
	}

	return result;
}

} //namespace

bool Glob::fnmatch(const String &name, const String &pattern) {
	return RegEx::create_from_string(translate(pattern))->search(name).is_valid();
}

Vector<String> Glob::glob(const String &pathname, bool hidden) {
	return _glob(pathname, false, hidden);
}

Vector<String> Glob::rglob(const String &pathname, bool hidden) {
	return _glob(pathname, true, hidden);
}

Vector<String> Glob::glob_list(const Vector<String> &pathnames, bool hidden) {
	Vector<String> result;
	for (auto &pathname : pathnames) {
		for (auto &match : _glob(pathname, false, hidden)) {
			result.push_back(std::move(match));
		}
	}
	return result;
}

Vector<String> Glob::rglob_list(const Vector<String> &pathnames, bool hidden) {
	Vector<String> result;
	for (auto &pathname : pathnames) {
		for (auto &match : _glob(pathname, true, hidden)) {
			result.push_back(std::move(match));
		}
	}
	return result;
}

void Glob::_bind_methods() {
	ClassDB::bind_static_method(get_class_static(), "glob", &Glob::glob, DEFVAL(false));
	ClassDB::bind_static_method(get_class_static(), "rglob", &Glob::rglob, DEFVAL(false));
	ClassDB::bind_static_method(get_class_static(), "glob_list", &Glob::glob_list, DEFVAL(false));
	ClassDB::bind_static_method(get_class_static(), "rglob_list", &Glob::rglob_list, DEFVAL(false));
	ClassDB::bind_static_method(get_class_static(), "fnmatch", &Glob::fnmatch);
}
