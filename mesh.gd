extends GDExample

@export var crosshair_node: Node3D
@export var cursor_node: Node3D

var voxel_size := 0.1

var brush_size := 0.1
var brush_distance := 0.8
var brush_blend := 0.1


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
	regen_mesh(voxel_size)


func _process(delta: float) -> void:
	var mouse_pos := get_viewport().get_mouse_position()
	var camera := get_viewport().get_camera_3d()


	if Input.is_action_pressed("brush_distance_increase"):
		brush_distance += 0.4 * delta
	if Input.is_action_pressed("brush_distance_decrease"):
		brush_distance -= 0.4 * delta

	if Input.is_action_pressed("brush_blend_increase"):
		brush_blend += 0.02 * delta
	if Input.is_action_pressed("brush_blend_decrease"):
		brush_blend -= 0.02 * delta
	brush_blend = clamp(brush_blend, 0, 2)
		
	if Input.is_action_pressed("brush_size_increase"):
		brush_size += 0.1 * delta
	if Input.is_action_pressed("brush_size_decrease"):
		brush_size -= 0.1 * delta
	brush_size = clamp(brush_size, 0.01, 100)
		
	var brush_pos := camera.project_position(mouse_pos, -brush_distance + camera.position.z)
	crosshair_node.global_position = brush_pos
	cursor_node.global_position = brush_pos
	cursor_node.scale = brush_size * Vector3.ONE

	
	if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT) != Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT):
		if last_held_brush_pos == null or brush_pos.distance_squared_to(last_held_brush_pos) > (0.0001 * camera.position.z):
			push_operation(
				brush_pos, 
				OPERATION_TYPE.ADD if Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT) else OPERATION_TYPE.SUBTRACT, 
				brush_size,
				brush_blend * brush_size,
			)
			last_held_brush_pos = brush_pos
	else:
		last_held_brush_pos = null
		
	regen_mesh(voxel_size)
