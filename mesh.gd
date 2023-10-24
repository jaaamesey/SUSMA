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

@export var collision_shape: CollisionShape3D
@export var inner_collision_shape: CollisionShape3D
@export var marble_template: RigidBody3D

var voxel_size := 0.1

var brush_size := 0.1
var brush_distance := 0.8
var brush_blend := 0.15

var brush_rotation := Quaternion.IDENTITY
var xr_brush_rotation_offset := Quaternion(Vector3.RIGHT, -PI / 2)

var brush_types := ["sphere", "cube", "angle", "grab"]
var brush_type := "sphere"

var x_symmetry := true

var last_held_brush_pos = null # Vector3 | null
var last_raw_brush_pos := Vector3()
var last_mouse_pos := Vector2()
var last_raw_mouse_pos := Vector2()

var last_mesh_id := -1

var can_fire_collision_pulse := true

var marbles_created := false
var marbles_created_changed := false

func _input(event: InputEvent) -> void:
	if Input.is_action_just_pressed("save_file"):
		save_file_dialog.show()
	if Input.is_action_just_pressed("open_file"):
		open_file_dialog.show()
		
	if Input.is_action_just_pressed("ui_right") or Input.is_action_just_pressed("cycle_brushes"):
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
					Vector3.ZERO,
					OPERATION_TYPE.ADD,
					OPERATION_SHAPE.SPHERE,
					0.075,
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
	xr_right.connect("button_pressed", func(name: String):
		if name == "by_button":
			brush_type = brush_types[(brush_types.find(brush_type) + 1) % brush_types.size()]
		elif name == "ax_button":
			add_marble(last_raw_brush_pos)
		elif name == "primary_click":
			x_symmetry = !x_symmetry
	)


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
	elif xr_left.get_input("by_button"):
		var rotation_vec := xr_right.get_vector2("primary") * 1.5 * delta
		xr_brush_rotation_offset = xr_brush_rotation_offset * Quaternion(Vector3.UP, rotation_vec.x) * Quaternion(Vector3.RIGHT, rotation_vec.y)
	else:
		brush_distance += brush_distance_spd * xr_right.get_vector2("primary").y
		
	if Input.is_action_pressed("rotate_brush"):
		Input.mouse_mode = Input.MOUSE_MODE_CONFINED_HIDDEN
		var mouse_dir := 0.01 * (last_raw_mouse_pos - get_viewport().get_mouse_position())
		brush_rotation = brush_rotation * Quaternion(Vector3.UP, mouse_dir.x) * Quaternion(Vector3.RIGHT, mouse_dir.y) 
		mouse_pos = last_mouse_pos
	elif Input.is_action_just_released("rotate_brush"):
		get_viewport().warp_mouse(last_mouse_pos)
		mouse_pos = last_mouse_pos
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	
	var brush_pos := camera.project_position(mouse_pos, -brush_distance + camera.position.z)

	if get_viewport().use_xr:
		brush_pos = xr_right.global_position - brush_distance * xr_right.get_global_transform().basis.z
		brush_rotation = xr_brush_rotation_offset * Quaternion.from_euler(xr_right.global_rotation).inverse()
	if brush_type == "grab":
		brush_rotation = Quaternion.IDENTITY

		
	crosshair_node.global_position = brush_pos
	world_crosshair_node.global_position = brush_pos
	cursor_node.global_position = brush_pos
	cursor_node.global_rotation = brush_rotation.inverse().get_euler()
	cursor_node.scale = brush_size * Vector3.ONE
	for cursor in cursor_node.get_children():
		cursor.visible = cursor.name == "KEEP" or cursor.name == brush_type
	cursor_depth_indicator_node.global_position = brush_pos
	cursor_depth_indicator_node.global_rotation = camera.global_rotation

	var is_add_held := Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT) or xr_right.get_float("trigger") > 0.5
	var is_subtract_held := Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT) or xr_right.get_float("grip") > 0.5
	
	var op_type := OPERATION_TYPE.ADD if is_add_held else OPERATION_TYPE.SUBTRACT
	var shape: OPERATION_SHAPE
	match brush_type:
		"sphere":
			shape = OPERATION_SHAPE.SPHERE
		"cube":
			shape = OPERATION_SHAPE.CUBE
		"angle":
			shape = OPERATION_SHAPE.SOLID_ANGLE
		"grab":
			op_type = OPERATION_TYPE.DRAG
			shape = OPERATION_SHAPE.SPHERE
	
	if get_viewport().use_xr:
		var haptic_intensity := 0.2
		var sd := get_grid_val(brush_pos)

		if sd < 0.01 and sd < 0:
			if can_fire_collision_pulse:
				xr_right.trigger_haptic_pulse("haptic", 0.1, haptic_intensity, 0.1, 0)
				can_fire_collision_pulse = false
		else:
			if can_fire_collision_pulse == false:
				xr_right.trigger_haptic_pulse("haptic", 0.1, haptic_intensity, 0.05, 0)
				can_fire_collision_pulse = true
	
	if Input.is_action_just_pressed("add_marble"):
		add_marble(brush_pos)
		
	if !open_file_dialog.visible and is_add_held != is_subtract_held and !Input.is_action_pressed("rotate"):
		var direction := 0.001 * (last_raw_brush_pos - brush_pos) / brush_size

		if last_held_brush_pos == null or brush_pos.distance_squared_to(last_held_brush_pos) > (0.0001 * camera.position.z):
			if get_viewport().use_xr:
				var haptic_intensity := 0.1
				xr_right.trigger_haptic_pulse("haptic", 0.1, haptic_intensity, delta, 0)
			push_operation(
				brush_pos, 
				brush_rotation,
				Vector3(1, 1, 1),
				direction,
				op_type,
				shape,
				brush_size,
				brush_blend * brush_size,
			)
			if x_symmetry:
				var euler_rotation := -brush_rotation.get_euler()
				euler_rotation.y *= -1
				push_operation(
					Vector3(-brush_pos.x, brush_pos.y, brush_pos.z), 
					Quaternion(brush_rotation.x, -brush_rotation.y, -brush_rotation.z, brush_rotation.w),
					Vector3.ONE if brush_type == "grab" else Vector3(-1, 1, 1),
					Vector3(-direction.x, direction.y, direction.z),
					op_type, 
					shape,
					brush_size,
					brush_blend * brush_size,
				)
			last_held_brush_pos = brush_pos
	else:
		last_held_brush_pos = null
		
	last_raw_brush_pos = brush_pos
	last_mouse_pos = mouse_pos
	last_raw_mouse_pos = get_viewport().get_mouse_position()
		
	regen_mesh(voxel_size)
	
	var mesh_id := mesh.get_rid().get_id()

	if marbles_created and (mesh_id != last_mesh_id or marbles_created_changed):
		collision_shape.shape = mesh.create_trimesh_shape()
		if marbles_created:
			var inner := mesh.create_outline(-0.01)
			if inner:
				inner_collision_shape.shape = inner.create_trimesh_shape()

	last_mesh_id = mesh_id
	marbles_created_changed = false
	
	brush_size_label_node.text = str(brush_size).pad_decimals(2)
	z_label_node.text = str(brush_distance).pad_decimals(2)
	
	xr_sculpt_info_node.text = ""
	sculpt_info_label_node.text = ""
	
	var sculpt_info = xr_sculpt_info_node if get_viewport().use_xr else sculpt_info_label_node
	sculpt_info.text += "FPS: %s\n" % Engine.get_frames_per_second()
	sculpt_info.text += "Voxels per unit: %3.1f\n" % (1.0 / voxel_size)
	sculpt_info.text += "Vertices: %s\n" % mesh.get_faces().size() if mesh != null else 0
	sculpt_info.text += "Brush type: %s\n" % brush_type
	sculpt_info.text += "Brush size: %3.3f\n" % brush_size
	sculpt_info.text += "Brush distance: %3.3f\n" % brush_distance
	sculpt_info.text += "Brush blend factor: %3.3f\n" % brush_blend
	sculpt_info.text += "Brush symmetry (X): %s\n" % x_symmetry
	sculpt_info.text = sculpt_info.text.to_upper()

func add_marble(pos: Vector3):
	var new_marble: RigidBody3D = marble_template.duplicate()
	new_marble.global_position = pos
	new_marble.freeze = false
	new_marble.sleeping = false
	marble_template.get_parent().add_child(new_marble)
	if !marbles_created:
		marbles_created = true
		marbles_created_changed = true

