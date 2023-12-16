extends Control

var ver_major = 0
var ver_minor = 0

var isHiDPI = DisplayServer.screen_get_dpi() >= 240

# gdre_set_key
var gdre_set_key = preload("res://gdre_set_key.tscn")
var gdre_recover = preload("res://gdre_recover.tscn")
var RECOVERY_DIALOG: Control = null

func test_text_to_bin(txt_to_bin: String, output_dir: String):
	var importer:ImportExporter = ImportExporter.new()
	var dst_file = txt_to_bin.get_file().replace(".tscn", ".scn").replace(".tres", ".res")
	importer.convert_res_txt_2_bin(output_dir, txt_to_bin, dst_file)
	importer.convert_res_bin_2_txt(output_dir, output_dir.path_join(dst_file), dst_file.replace(".scn", ".tscn").replace(".res", ".tres"))

func popup_error_box(message: String, title: String, parent_window: Window) -> AcceptDialog:
	var dialog = AcceptDialog.new()
	dialog.set_text(message)
	dialog.set_title(title)
	get_tree().get_root().add_child(dialog)	
	dialog.connect("confirmed", parent_window.show)
	dialog.connect("canceled", parent_window.show)
	dialog.popup_centered()
	return dialog

const ERR_SKIP = 45

func _on_recovery_done():
	if RECOVERY_DIALOG:
		RECOVERY_DIALOG.hide_win()
		get_tree().get_root().remove_child(RECOVERY_DIALOG)
		RECOVERY_DIALOG = null
	else:
		print("Recovery dialog not instantiated!!!")

func _on_recover_project_file_selected(path):
	# open the recover dialog
	var recover_dialog
	assert(gdre_recover.can_instantiate())
	RECOVERY_DIALOG = gdre_recover.instantiate()
	RECOVERY_DIALOG.set_root_window(get_window())
	#recover_dialog.visible = false
	var root_child = get_tree().get_root().get_children()[0]
	var before_children = get_tree().get_root().get_children()
	get_tree().get_root().add_child(RECOVERY_DIALOG)
	get_tree().get_root().move_child(RECOVERY_DIALOG, root_child.get_index() -1)
	var after_children = get_tree().get_root().get_children()
	RECOVERY_DIALOG.add_pack(path)
	RECOVERY_DIALOG.connect("recovery_done", self._on_recovery_done)
	RECOVERY_DIALOG.show_win()
	
	print("HALDO!!!!")
	print("HALDO2!!!!")
	
	

func _on_REToolsMenu_item_selected(index):
	match index:
		0:
			# Recover Project...
			# pop open a file dialog
			var dialog = FileDialog.new()
			if isHiDPI:
				dialog.size *= 4.0
				dialog.min_size = dialog.size

			dialog.set_access(FileDialog.ACCESS_FILESYSTEM)
			dialog.file_mode = FileDialog.FILE_MODE_OPEN_FILE
			dialog.filters = ["*.exe,*.bin,*.32,*.64,*.x86_64,*.x86,*.arm64,*.universal;Self contained executable files", "*.pck;PCK files", "*.apk;APK files", "*;All files"]
			# TODO: remove this
			dialog.current_dir = "/Users/nikita/Workspace/godot-test-bins"
			if (dialog.current_dir.is_empty()):
				dialog.current_dir = GDRESettings.get_exec_dir()
			dialog.connect("file_selected", self._on_recover_project_file_selected)
			get_tree().get_root().add_child(dialog)
			if isHiDPI:
				dialog.get_viewport().content_scale_factor = 2.0
				
			dialog.popup_centered()

			
		1:  # set key
			# Open the set key dialog
			$SetEncryptionKeyWindow.popup_centered()
		2:  # about
			$AboutPopup.popup_centered()
		3:  # Report a bug
			OS.shell_open("https://github.com/bruvzg/gdsdecomp/issues/new?assignees=&labels=bug&template=bug_report.yml&sys_info=" + GDRESettings.get_sys_info_string())
		4:  # Quit
			get_tree().quit()
	
func _on_re_editor_standalone_write_log_message(message):
	$log_window.text += message
	$log_window.scroll_to_line($log_window.get_line_count() - 1)

func _on_setenc_key_ok_pressed():
	# get the current text in the line edit
	var keytextbox = $SetEncryptionKeyWindow/VBoxContainer/KeyText
	var key:String = keytextbox.text
	if key.length() == 0:
		GDRESettings.reset_encryption_key()
	# set the key
	else:
		var err:int = GDRESettings.set_encryption_key_string(key)
		if (err != OK):
			keytextbox.text = ""
			# pop up an accept dialog
			popup_error_box("Invalid key!\nKey must be a hex string with 64 characters", "Error", $SetEncryptionKeyWindow)
			return
	# close the window
	$SetEncryptionKeyWindow.hide()
	

	
func _on_setenc_key_cancel_pressed():
	$SetEncryptionKeyWindow.hide()


func _on_version_lbl_pressed():
	OS.shell_open("https://github.com/bruvzg/gdsdecomp")

func _ready():
	var popup_menu_gdremenu:PopupMenu = $MenuContainer/REToolsMenu.get_popup()
	popup_menu_gdremenu.connect("id_pressed", self._on_REToolsMenu_item_selected)

	$version_lbl.text = GDRESettings.get_gdre_version()
	# If CLI arguments were passed in, just quit
	# check if the current screen is hidpi
	if isHiDPI:
		# set the content scaling factor to 2x
		var viewport = get_viewport()
		get_viewport().content_scale_factor = 2.0
		# get the menu container
		#for menuitem: MenuButton in $MenuContainer.get_children():
			## get the items in the menu
			#var popup : PopupMenu = menuitem.get_popup()
			#popup.content_scale_factor = 2.0
			#popup.size = popup.size * 4.0
	if handle_cli():
		get_tree().quit()
	
func get_arg_value(arg):
	var split_args = arg.split("=")
	if split_args.size() < 2:
		print("Error: args have to be in the format of --key=value (with equals sign)")
		get_tree().quit()
	return split_args[1]

func normalize_path(path: String):
	return path.replace("\\","/")
# func print_import_info_from_pak(pak_file: String):
# 	var pckdump = PckDumper.new()
# 	pckdump.load_pck(pak_file)
# 	var importer:ImportExporter = ImportExporter.new()
# 	importer.load_import_files()
# 	var arr = importer.get_import_files()
# 	print("size is " + str(arr.size()))
# 	for ifo in arr:
# 		var s:String = ifo.get_source_file() + " is "
# 		if ifo.get_import_loss_type() == 0:
# 			print(s + "lossless")
# 		elif ifo.get_import_loss_type() == -1:
# 			print(s + "unknown")
# 		else:
# 			print(s + "lossy")
# 		print((ifo as ImportInfo).to_string())
# 	pckdump.clear_data()
# 	importer.reset()
	
# func print_import_info(output_dir: String):
# 	var importer:ImportExporter = ImportExporter.new()
# 	importer.load_import_files()
# 	var arr = importer.get_import_files()
# 	print("size is " + str(arr.size()))
# 	for ifo in arr:
# 		var s:String = ifo.get_source_file() + " is "
# 		if ifo.get_import_loss_type() == 0:
# 			print(s + "lossless")
# 		elif ifo.get_import_loss_type() == -1:
# 			print(s + "unknown")
# 		else:
# 			print(s + "lossy")
# 		print((ifo as ImportInfo).to_string())
# 	importer.reset()

func test_decomp(fname):
	var decomp = GDScriptDecomp_ed80f45.new()
	var f = fname
	if f.get_extension() == "gdc":
		print("decompiling " + f)
		#
		#if decomp.decompile_byte_code(output_dir.path_join(f)) != OK: 
		if decomp.decompile_byte_code(f) != OK: 
			print("error decompiling " + f)
		else:
			var text = decomp.get_script_text()
			var gdfile:FileAccess = FileAccess.open(f.replace(".gdc",".gd"), FileAccess.WRITE)
			if gdfile == null:
				gdfile.store_string(text)
				gdfile.close()
				#da.remove(f)
				print("successfully decompiled " + f)
			else:
				print("error failed to save "+ f)

func export_imports(output_dir:String, files: PackedStringArray):
	var importer:ImportExporter = ImportExporter.new()
	importer.export_imports(output_dir, files)
	importer.reset()
				
	
func dump_files(output_dir:String, files: PackedStringArray, ignore_checksum_errors: bool = false) -> int:
	var err:int = OK;
	var pckdump = PckDumper.new()
	if err == OK:
		err = pckdump.check_md5_all_files()
		if err != OK:
			if (err != ERR_SKIP and not ignore_checksum_errors):
				print("MD5 checksum failed, not proceeding...")
				return err
			elif (ignore_checksum_errors):
				print("MD5 checksum failed, but --ignore_checksum_errors specified, proceeding anyway...")
		err = pckdump.pck_dump_to_dir(output_dir, files)
		if err != OK:
			print("error dumping to dir")
			return err
	else:
		print("ERROR: failed to load exe")
	return err;

func print_usage():
	print("Godot Reverse Engineering Tools")
	print("")
	print("Without any CLI options, the tool will start in GUI mode")
	print("\nGeneral options:")
	print("  -h, --help: Display this help message")
	print("\nFull Project Recovery options:")
	print("Usage: GDRE_Tools.exe --headless --recover=<PCK_OR_EXE_OR_EXTRACTED_ASSETS_DIR> [options]")
	print("")
	print("--recover=<GAME_PCK/EXE/APK_OR_EXTRACTED_ASSETS_DIR>\t\tThe PCK, APK, EXE, or extracted project directory to perform full project recovery on")
	print("--extract=<GAME_PCK/EXE/APK_OR_EXTRACTED_ASSETS_DIR>\t\tThe PCK, APK, or EXE to extract")

	print("\nOptions:\n")
	print("--key=<KEY>\t\tThe Key to use if project is encrypted (hex string)")
	print("--output-dir=<DIR>\t\tOutput directory, defaults to <NAME_extracted>, or the project directory if one of specified")
	print("--ignore-checksum-errors\t\tIgnore MD5 checksum errors when extracting/recovering")
	print("--translation-only\t\tOnly extract translation files")

# TODO: remove this hack
var translation_only = false

func copy_dir(src:String, dst:String) -> int:
	var da:DirAccess = DirAccess.open(src)
	if !da.dir_exists(src):
		print("Error: " + src + " does not appear to be a directory")
		return ERR_FILE_NOT_FOUND
	da.make_dir_recursive(dst)
	da.copy_dir(src, dst)
	return OK

func get_cli_abs_path(path:String) -> String:
	if path.is_absolute_path():
		return path
	var exec_path = GDRESettings.get_exec_dir()
	var abs_path = exec_path.path_join(path)
	return abs_path

func recovery(  input_file:String,
				output_dir:String,
				enc_key:String,
				extract_only: bool,
				ignore_checksum_errors: bool = false):
	var da:DirAccess
	var is_dir:bool = false
	var err: int = OK
	input_file = get_cli_abs_path(input_file)
	if output_dir == "":
		output_dir = input_file.get_basename()
		if output_dir.get_extension():
			output_dir += "_recovery"
	else:
		output_dir = get_cli_abs_path(output_dir)

	da = DirAccess.open(input_file.get_base_dir())

	# check if da works
	if da == null:
		print("Error: failed to locate parent dir for " + input_file)
		return
	#directory
	if da.dir_exists(input_file):
		if !da.dir_exists(input_file.path_join(".import")):
			print("Error: " + input_file + " does not appear to be a project directory")
			return
		else:
			is_dir = true
	#PCK/APK
	elif not da.file_exists(input_file):
		print("Error: failed to locate " + input_file)
		return

	GDRESettings.open_log_file(output_dir)
	if (enc_key != ""):
		err = GDRESettings.set_encryption_key_string(enc_key)
		if (err != OK):
			print("Error: failed to set key!")
			return
	
	err = GDRESettings.load_pack(input_file)
	if (err != OK):
		print("Error: failed to open " + input_file)
		return

	print("Successfully loaded PCK!") 
	ver_major = GDRESettings.get_ver_major()
	ver_minor = GDRESettings.get_ver_minor()
	var version:String = GDRESettings.get_version_string()
	print("Version: " + version)
	var files: PackedStringArray = []
	if (translation_only):
		files = GDRESettings.get_file_list()
		var new_files:PackedStringArray = []
		# remove all the non ".translation" files
		for i in range(files.size()-1, -1, -1):
			if (files[i].get_extension() == "translation"):
				new_files.append(files[i])
		files = new_files
		print("Translation only mode, only extracting translation files")

	if output_dir != input_file and not is_dir: 
		if (da.file_exists(output_dir)):
			print("Error: output dir appears to be a file, not extracting...")
			return
	if is_dir:
		if extract_only:
			print("Why did you open a folder to extract it??? What's wrong with you?!!?")
			return
		if copy_dir(input_file, output_dir) != OK:
			print("Error: failed to copy " + input_file + " to " + output_dir)
			return
	else:
		err = dump_files(output_dir, files, ignore_checksum_errors)
		if (err != OK):
			print("Error: failed to extract PAK file, not exporting assets")
			return
	if (extract_only):
		return
	export_imports(output_dir, files)

func print_version():
	print("Godot RE Tools " + GDRESettings.get_gdre_version())

func close_log():
	var path = GDRESettings.get_log_file_path()
	if path == "":
		return
	GDRESettings.close_log_file()
	print("Log file written to: " + path)
	print("Please include this file when reporting issues!")

func handle_cli() -> bool:
	var args = OS.get_cmdline_args()
	var input_extract_file:String = ""
	var input_file:String = ""
	var output_dir: String = ""
	var enc_key: String = ""
	var txt_to_bin: String = ""
	var ignore_md5: bool = false
	if (args.size() == 0 or (args.size() == 1 and args[0] == "res://gdre_main.tscn")):
		return false
	for i in range(args.size()):
		var arg:String = args[i]
		if arg == "--help":
			print_version()
			print_usage()
			get_tree().quit()
		if arg.begins_with("--version"):
			print_version()
			get_tree().quit()
		if arg.begins_with("--extract"):
			input_extract_file = normalize_path(get_arg_value(arg))
		if arg.begins_with("--recover"):
			input_file = normalize_path(get_arg_value(arg))
		if arg.begins_with("--txt-to-bin"):
			txt_to_bin = normalize_path(get_arg_value(arg))	
		elif arg.begins_with("--output-dir"):
			output_dir = normalize_path(get_arg_value(arg))
		elif arg.begins_with("--key"):
			enc_key = get_arg_value(arg)
		elif arg.begins_with("--ignore-checksum-errors"):
			ignore_md5 = true
		elif arg.begins_with("--translation-only"):
			translation_only = true

	if input_file != "":
		recovery(input_file, output_dir, enc_key, false, ignore_md5)
		GDRESettings.unload_pack()
		close_log()
		get_tree().quit()
	elif input_extract_file != "":
		recovery(input_extract_file, output_dir, enc_key, true, ignore_md5)
		GDRESettings.unload_pack()
		close_log()
		get_tree().quit()
	elif txt_to_bin != "":
		txt_to_bin = get_cli_abs_path(txt_to_bin)
		output_dir = get_cli_abs_path(output_dir)
		test_text_to_bin(txt_to_bin, output_dir)
		get_tree().quit()
	else:
		print("ERROR: invalid option")

		print_usage()
		get_tree().quit()
	return true
