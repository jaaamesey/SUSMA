extends GDExample

var voxel_size := 0.1

var brush_size := 0.9
var brush_blend := 0.5

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

	if Input.is_action_just_pressed("brush_size_decrease"):
		brush_size += 0.01
	if Input.is_action_just_pressed("brush_size_increase"):
		brush_size -= 0.01
		
	if Input.is_action_just_pressed("brush_blend_increase"):
		brush_blend += 0.01
	if Input.is_action_just_pressed("brush_blend_decrease"):
		brush_blend -= 0.01
	
	if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):
		set_brush_pos(camera.project_position(mouse_pos, -brush_size + camera.position.z))
		set_brush_blend(brush_blend)
	regen_mesh(voxel_size)

