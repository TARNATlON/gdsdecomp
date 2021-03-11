/*************************************************************************/
/*  bytecode_base.cpp                                                    */
/*************************************************************************/

#include "bytecode_base.h"

#include "core/io/file_access_encrypted.h"
#include "core/os/file_access.h"

void GDScriptDecomp::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("decompile_byte_code", "path"), &GDScriptDecomp::decompile_byte_code);
	ObjectTypeDB::bind_method(_MD("decompile_byte_code_encrypted", "path", "key"), &GDScriptDecomp::decompile_byte_code_encrypted);

	ObjectTypeDB::bind_method(_MD("get_script_text"), &GDScriptDecomp::get_script_text);
	ObjectTypeDB::bind_method(_MD("get_error_message"), &GDScriptDecomp::get_error_message);
}

void GDScriptDecomp::_ensure_space(String &p_code) {

	if (!p_code.ends_with(" ")) {
		p_code += String(" ");
	}
}

Error GDScriptDecomp::decompile_byte_code_encrypted(const String &p_path, Vector<uint8_t> p_key) {

	Vector<uint8_t> bytecode;

	FileAccess *fa = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V(!fa, ERR_FILE_CANT_OPEN);

	FileAccessEncrypted *fae = memnew(FileAccessEncrypted);
	ERR_FAIL_COND_V(!fae, ERR_FILE_CORRUPT);

	Error err = fae->open_and_parse(fa, p_key, FileAccessEncrypted::MODE_READ);

	if (err) {
		fa->close();
		memdelete(fa);
		memdelete(fae);

		ERR_FAIL_COND_V(err, ERR_FILE_CORRUPT);
	}

	bytecode.resize(fae->get_len());
	fae->get_buffer(bytecode.ptr(), bytecode.size());
	fae->close();
	memdelete(fae);

	error_message = RTR("No error");

	return decompile_buffer(bytecode);
}

Error GDScriptDecomp::decompile_byte_code(const String &p_path) {

	Vector<uint8_t> bytecode;

	bytecode = FileAccess::get_file_as_array(p_path);

	error_message = RTR("No error");

	return decompile_buffer(bytecode);
}

String GDScriptDecomp::get_script_text() {

	return script_text;
}

String GDScriptDecomp::get_error_message() {

	return error_message;
}

String GDScriptDecomp::get_constant_string(Vector<Variant> &constants, uint32_t constId) {
	String constString = constants[constId].get_construct_string();
	if (constants[constId].get_type() == Variant::Type::STRING) {
		constString = constString.replace("\n", "\\n");
	}
	return constString;
}
