/*************************************************************************/
/*  godotver.h                                                           */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef GODOTVER_H
#define GODOTVER_H

#include "core/object/ref_counted.h"
#include "core/string/ustring.h"

#include "modules/modules_enabled.gen.h"

#ifdef MODULE_REGEX_ENABLED
#include "modules/regex/regex.h"
#endif
// <sys/sysmacros.h> is included somewhere, which defines major(dev) to gnu_dev_major(dev)
#if defined(major)
#undef major
#endif
#if defined(minor)
#undef minor
#endif

class SemVer : public RefCounted {
	GDCLASS(SemVer, RefCounted);

private:
#ifdef MODULE_REGEX_ENABLED
	static RegEx *regex;
#endif

protected:
	int major = 0;
	int minor = 0;
	int patch = 0;
	String prerelease;
	String build_metadata;
	bool valid = false;
	bool is_strict = true;

	enum _TYPE {
		STRICT,
		WINDOWS,
		GODOT
	};
	virtual int cmp(const Ref<SemVer> &p_b) const;
	virtual String as_text() const;
	virtual _TYPE get_type() const { return STRICT; }
	static bool parse_digit_only_field(const String &p_field, uint64_t &r_result);
	static void _bind_methods();

public:
	bool operator==(const Ref<SemVer> &b) const {
		return cmp(b) == 0;
	}

	bool operator!=(const Ref<SemVer> &b) const {
		return cmp(b) != 0;
	}

	bool operator<(const Ref<SemVer> &b) const {
		return cmp(b) < 0;
	}

	bool operator<=(const Ref<SemVer> &b) const {
		return cmp(b) <= 0;
	}

	bool operator>(const Ref<SemVer> &b) const {
		return cmp(b) > 0;
	}

	bool operator>=(const Ref<SemVer> &b) const {
		return cmp(b) >= 0;
	}

	bool eq(const Ref<SemVer> &b) const {
		return cmp(b) == 0;
	}

	bool neq(const Ref<SemVer> &b) const {
		return cmp(b) != 0;
	}

	bool lt(const Ref<SemVer> &b) const {
		return cmp(b) < 0;
	}

	bool lte(const Ref<SemVer> &b) const {
		return cmp(b) <= 0;
	}

	bool gt(const Ref<SemVer> &b) const {
		return cmp(b) > 0;
	}

	bool gte(const Ref<SemVer> &b) const {
		return cmp(b) >= 0;
	}

	operator String() const { return as_text(); };
	String to_string() const;

	bool is_prerelease() { return prerelease != ""; }

	int get_major() const { return major; }
	int get_minor() const { return minor; }
	int get_patch() const { return patch; }
	String get_prerelease() const { return prerelease; }
	String get_build_metadata() const { return build_metadata; }

	void set_major(int p_par) { major = p_par; }
	void set_minor(int p_par) { minor = p_par; }
	void set_patch(int p_par) { patch = p_par; }
	void set_prerelease(String p_par) { prerelease = p_par; }
	void set_build_metadata(String p_par) { build_metadata = p_par; }

	bool valid() const { return valid; }

	static bool parse_valid(const String &p_ver_text, Ref<SemVer> &r_semver);
	static Ref<SemVer> parse(const String &p_ver_text);
	static Ref<SemVer> create(int p_major, int p_minor, int p_patch,
			const String &p_prerelease = "", const String &p_build_metadata = "");
	SemVer() {}
	SemVer(int p_major, int p_minor, int p_patch,
			const String &p_prerelease = "", const String &p_build_metadata = "") :
			major(p_major),
			minor(p_minor),
			patch(p_patch),
			prerelease(p_prerelease),
			build_metadata(p_build_metadata) {
		valid = true;
	}
};

class GodotVer : public SemVer {
	GDCLASS(GodotVer, SemVer);

#ifdef MODULE_REGEX_ENABLED
	static RegEx *non_strict_regex;
#endif
protected:
	virtual int cmp(const Ref<SemVer> &p_b) const override;
	virtual String as_text() const override;
	static void _bind_methods();

public:
	bool is_not_custom_build();
	static bool parse_valid(const String &p_ver_text, Ref<GodotVer> &r_semver, bool p_strict = true);
	static Ref<GodotVer> parse(const String &p_ver_text, bool p_strict = true);
	static Ref<GodotVer> create(int p_major, int p_minor, int p_patch,
			const String &p_prerelease = "", const String &p_build_metadata = "");
	GodotVer() {
		is_strict = false;
	}
	GodotVer(int p_major, int p_minor, int p_patch,
			const String &p_prerelease = "", const String &p_build_metadata = "<unknown_build>") :
			SemVer(p_major, p_minor, p_patch, p_prerelease, p_build_metadata) {
		is_strict = false;
	}
};

#endif // GODOTVER_H
