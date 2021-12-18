#include <core/io/file_access.h>
#include <core/io/resource.h>
#include <core/io/dir_access.h>
#include <core/io/file_access.h>
#include "external/toojpeg/toojpeg.h"
#include <modules/webp/image_loader_webp.cpp>
namespace gdreutil{
static Vector<String> get_recursive_dir_list(const String dir, const Vector<String> &wildcards = Vector<String>(), const bool absolute = true, const String rel = "", const bool &res = false) {
    Vector<String> ret;
    Error err;
    DirAccess *da = DirAccess::open(dir.plus_file(rel), &err);
    ERR_FAIL_COND_V_MSG(!da, ret, "Failed to open directory " + dir);

    if (!da) {
        return ret;
    }
    String base = absolute ? dir : "";
    da->list_dir_begin();
    String f = da->get_next();
    while (!f.is_empty()) {
        if (f == "." || f == "..") {
            f = da->get_next();
            continue;
        } else if (da->current_is_dir()) {
            ret.append_array(get_recursive_dir_list(dir, wildcards, absolute, rel.plus_file(f), res));
        } else {
            if (wildcards.size() > 0) {
                for (int i = 0; i < wildcards.size(); i++) {
                    if (f.get_file().match(wildcards[i])) {
                        ret.append((res ? "res://" : "") + base.plus_file(rel).plus_file(f));
                        break;
                    }
                }
            } else {
                ret.append((res ? "res://" : "") + base.plus_file(rel).plus_file(f));
            }
        }
        f = da->get_next();
    }
    da->list_dir_end();
    memdelete(da);
    return ret;
}
FileAccess *_____tmp_file;

Error save_image_as_webp(const String &p_path, const Ref<Image> &p_img, bool lossy = false) {
    Ref<Image> source_image = p_img->duplicate();
    Vector<uint8_t> buffer;
    if (lossy) {
        buffer = _webp_lossy_pack(source_image, 1);
    } else {
        buffer = _webp_lossless_pack(source_image);
    }
    Error err;
    FileAccess * file = FileAccess::open(p_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V_MSG(err, err, vformat("Can't save WEBP at path: '%s'.", p_path));
    file->store_buffer(buffer.ptr(), buffer.size());
    if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
		memdelete(file);
		return ERR_CANT_CREATE;
	}
	file->close();
	memdelete(file);
	return OK;
}

Error save_image_as_jpeg(const String &p_path, const Ref<Image> &p_img) {
	Vector<uint8_t> buffer;
	Ref<Image> source_image = p_img->duplicate();

	if (source_image->is_compressed()) {
		source_image->decompress();
	}

	ERR_FAIL_COND_V(source_image->is_compressed(), FAILED);

	int width = source_image->get_width();
	int height = source_image->get_height();
    bool isRGB;
    if (source_image->detect_alpha()) {
        WARN_PRINT("Alpha channel detected, will not be saved to jpeg...");
    }
	switch (source_image->get_format()) {
		case Image::FORMAT_L8:
            isRGB = false;
            break;
		case Image::FORMAT_LA8:
            isRGB = false;
            source_image->convert(Image::FORMAT_L8);
            break;
		case Image::FORMAT_RGB8:
            isRGB = true;
            break;
		case Image::FORMAT_RGBA8:
		default:
            source_image->convert(Image::FORMAT_RGB8);
            isRGB = true;
			break;
	}
    
	const Vector<uint8_t> image_data = source_image->get_data();
	const uint8_t *reader = image_data.ptr();
	// we may be passed a buffer with existing content we're expected to append to
    Error err;
    _____tmp_file = FileAccess::open(p_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V_MSG(err, err, vformat("Can't save JPEG at path: '%s'.", p_path));

	int success = 0;
	{ // scope writer lifetime
        bool success = TooJpeg::writeJpeg([](unsigned char oneByte){
            _____tmp_file->store_8(oneByte);
        }, image_data.ptr(), width, height, isRGB, 100, false);
		ERR_FAIL_COND_V_MSG(success, ERR_BUG, "Failed to convert image to JPEG");
	}

	if (_____tmp_file->get_error() != OK && _____tmp_file->get_error() != ERR_FILE_EOF) {
		memdelete(_____tmp_file);
		return ERR_CANT_CREATE;
	}

	_____tmp_file->close();
	memdelete(_____tmp_file);

	return OK;
}


}