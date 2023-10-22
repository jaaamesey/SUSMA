extends GDExample

@export var crosshair_node: Node3D
@export var world_crosshair_node: Node3D
@export var cursor_node: Node3D
@export var cursor_depth_indicator_node: Node3D
@export var brush_size_label_node: Label3D
@export var z_label_node: Label3D
@export var sculpt_info_label_node: RichTextLabel
@export var xr_sculpt_info_node: Label3D

@export var xr_camera: XRCamera3D
@export var xr_left: XRController3D
@export var xr_right: XRController3D

@export var open_file_dialog: FileDialog
@export var save_file_dialog: FileDialog

var voxel_size := 0.1

var brush_size := 0.1
var brush_distance := 0.8
var brush_blend := 0.15

var brush_types := ["sphere", "cube", "grab"]
var brush_type := "sphere"

var x_symmetry := true


var last_held_brush_pos = null # Vector3 | null


func _input(event: InputEvent) -> void:
	if Input.is_action_just_pressed("save_file"):
		save_file_dialog.show()
	if Input.is_action_just_pressed("open_file"):
		open_file_dialog.show()
		
	if Input.is_action_just_pressed("ui_right"):
		brush_type = brush_types[(brush_types.find(brush_type) + 1) % brush_types.size()]
	if Input.is_action_just_pressed("ui_left"):
		brush_type = brush_types[(brush_types.find(brush_type) - 1) % brush_types.size()]
			
	if Input.is_action_just_pressed("ui_up"):
		voxel_size -= 0.01 if voxel_size - 0.01 > 0 else 0.001
	elif Input.is_action_just_pressed("ui_down"):
		voxel_size += 0.01
	else:
		return

	var start_time := Time.get_ticks_msec()
	regen_mesh(voxel_size)
	print("Meshify time (ms): ", Time.get_ticks_msec() - start_time)
	print("Voxel size: ", voxel_size)
	print("Verts: ", mesh.get_faces().size() if mesh != null else 0)


func _ready():
	regen_mesh(voxel_size)
	open_file_dialog.connect("file_selected", 
		func(path: String): 
			var gltf_doc := GLTFDocument.new()
			var gltf_state := GLTFState.new()
			gltf_doc.append_from_file(path, gltf_state)
			var verts := gltf_state.meshes[0].mesh.get_surface_arrays(Mesh.ARRAY_VERTEX)
			var tris := gltf_state.meshes[0].mesh.get_surface_arrays(Mesh.ARRAY_INDEX)

			for i in range(0, verts[0].size()):
				push_operation(
					verts[0][i],
					Quaternion.IDENTITY,
					Vector3.ONE,
					OPERATION_TYPE.ADD,
					OPERATION_SHAPE.SPHERE,
					0.1,
					0.1,
				)
	)
	save_file_dialog.connect("file_selected", 
		func(path: String):
			var gltf_doc := GLTFDocument.new()
			var gltf_state := GLTFState.new()
			gltf_doc.append_from_scene(self, gltf_state)
			gltf_doc.write_to_filesystem(gltf_state, path)
	)
	open_file_dialog.show()

func _process(delta: float) -> void:
	var mouse_pos := get_viewport().get_mouse_position()
	var camera := xr_camera if get_viewport().use_xr else get_viewport().get_camera_3d()

	var brush_distance_spd := 2.0 * delta
	if Input.is_action_pressed("slow_brush_distance"):
		brush_distance_spd *= 0.25
	
	if !Input.is_action_pressed("rotate"):
		if Input.is_action_pressed("brush_distance_increase"):
			brush_distance += brush_distance_spd
		if Input.is_action_pressed("brush_distance_decrease"):
			brush_distance -= brush_distance_spd
		if Input.is_action_just_pressed("center_z"):
			brush_distance = 0
		
		if Input.is_action_pressed("brush_size_increase"):
			brush_size += 0.3 * delta
		if Input.is_action_pressed("brush_size_decrease"):
			brush_size -= 0.3 * delta
		brush_size = clamp(brush_size, 0.01, 100)

		if Input.is_action_pressed("brush_blend_increase"):
			brush_blend += 0.1 * delta
		if Input.is_action_pressed("brush_blend_decrease"):
			brush_blend -= 0.1 * delta
		brush_blend = clamp(brush_blend, 0, 2)
		
		if Input.is_action_just_pressed("toggle_symmetry"):
			x_symmetry = !x_symmetry
	
	if xr_left.get_input("ax_button"):
		brush_size += 0.3 * delta * xr_right.get_vector2("primary").y
	else:
		brush_distance += brush_distance_spd * xr_right.get_vector2("primary").y
		
	var brush_pos := camera.project_position(mouse_pos, -brush_distance + camera.position.z)
	var brush_rotation := Quaternion.from_euler(Vector3(20, 10, 0))
	if get_viewport().use_xr:
		brush_pos = xr_right.global_position - brush_distance * xr_right.get_global_transform().basis.z
	crosshair_node.global_position = brush_pos
	world_crosshair_node.global_position = brush_pos
	cursor_node.global_position = brush_pos
	cursor_node.global_rotation = brush_rotation.inverse().get_euler()
	cursor_node.scale = brush_size * Vector3.ONE
	for cursor in cursor_node.get_children():
		cursor.visible = cursor.name == brush_type
	cursor_depth_indicator_node.global_position = brush_pos
	cursor_depth_indicator_node.global_rotation = camera.global_rotation

	var is_add_held := Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT) or xr_right.get_float("trigger") > 0.5
	var is_subtract_held := Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT) or xr_right.get_float("grip") > 0.5

	var shape: OPERATION_SHAPE
	match brush_type:
		"sphere":
			shape = OPERATION_SHAPE.SPHERE
		"cube":
			shape = OPERATION_SHAPE.CUBE
	

	if !open_file_dialog.visible and is_add_held != is_subtract_held and !Input.is_action_pressed("rotate"):
		if last_held_brush_pos == null or brush_pos.distance_squared_to(last_held_brush_pos) > (0.0001 * camera.position.z):
			push_operation(
				brush_pos, 
				brush_rotation,
				Vector3.ONE,
				OPERATION_TYPE.ADD if is_add_held else OPERATION_TYPE.SUBTRACT, 
				shape,
				brush_size,
				brush_blend * brush_size,
			)
			if x_symmetry:
				push_operation(
					Vector3(-brush_pos.x, brush_pos.y, brush_pos.z), 
					brush_rotation,
					Vector3(-1, 1, 1),
					OPERATION_TYPE.ADD if is_add_held else OPERATION_TYPE.SUBTRACT, 
					shape,
					brush_size,
					brush_blend * brush_size,
				)
			last_held_brush_pos = brush_pos
	else:
		last_held_brush_pos = null
		
	regen_mesh(voxel_size)
	
	brush_size_label_node.text = str(brush_size).pad_decimals(2)
	z_label_node.text = str(brush_distance).pad_decimals(2)
	
	xr_sculpt_info_node.text = ""
	sculpt_info_label_node.text = ""
	
	var sculpt_info = xr_sculpt_info_node if get_viewport().use_xr else sculpt_info_label_node
	sculpt_info.text += "FPS: %s\n" % Engine.get_frames_per_second()
	sculpt_info.text += "Voxel size: %s\n" % voxel_size
	sculpt_info.text += "Vertices: %s\n" % mesh.get_faces().size() if mesh != null else 0
	sculpt_info.text += "Brush type: %s\n" % brush_type
	sculpt_info.text += "Brush size: %3.3f\n" % brush_size
	sculpt_info.text += "Brush distance: %3.3f\n" % brush_distance
	sculpt_info.text += "Brush blend factor: %3.3f\n" % brush_blend
	sculpt_info.text += "Brush symmetry (X): %s\n" % x_symmetry
	sculpt_info.text = sculpt_info.text.to_upper()
