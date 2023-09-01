extends GDExample

@export var crosshair_node: Node3D

var voxel_size := 0.1

var brush_distance := 0.8
var brush_blend := 0.13

var last_held_brush_pos = null # Vector3 | null


func _input(event: InputEvent) -> void:
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
	set_brush_pos(Vector3(0, 0, 999999999999))
	regen_mesh(voxel_size)


func _process(delta: float) -> void:
	var mouse_pos := get_viewport().get_mouse_position()
	var camera := get_viewport().get_camera_3d()


	if Input.is_action_pressed("brush_distance_increase"):
		brush_distance += 0.02 * delta
	if Input.is_action_pressed("brush_distance_decrease"):
		brush_distance -= 0.02 * delta

	if Input.is_action_pressed("brush_blend_increase"):
		brush_blend += 0.02 * delta
	if Input.is_action_pressed("brush_blend_decrease"):
		brush_blend -= 0.02 * delta
		
	var brush_pos := camera.project_position(mouse_pos, -brush_distance + camera.position.z)
	crosshair_node.global_position = brush_pos

	set_brush_blend(brush_blend)

	
	if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):
		if last_held_brush_pos == null or brush_pos.distance_squared_to(last_held_brush_pos) > (0.001 * camera.position.z):
			set_brush_pos(brush_pos)
			last_held_brush_pos = brush_pos
	else:
		last_held_brush_pos = null
	regen_mesh(voxel_size)
