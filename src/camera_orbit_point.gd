extends Node3D

@onready var x_rotator: Node3D = $XRotator
@onready var camera: Camera3D = $XRotator/Camera3D
@onready var xr_camera: Camera3D = $XRotator/Camera3D/XROrigin3D/XRCamera3D
@onready var xr_left: XRController3D = $XRotator/Camera3D/XROrigin3D/LeftController
@onready var xr_right: XRController3D = $XRotator/Camera3D/XROrigin3D/RightController

@onready var xr_interface := XRServer.find_interface("OpenXR")

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	if !xr_interface.is_initialized():
		xr_interface = null
	if xr_interface:
		print("XR interface initialized")
		get_viewport().use_xr = true

var _has_mouse_warped := false
func _input(event: InputEvent) -> void:
	if Input.is_action_just_pressed("toggle_vr"):
		get_viewport().use_xr = !get_viewport().use_xr
		
	var zoom_spd := 1.1
	if Input.is_action_just_pressed("zoom_in"):
		camera.position.z /= zoom_spd
	if Input.is_action_just_pressed("zoom_out"):
		camera.position.z *= zoom_spd
	camera.position.z = clamp(camera.position.z, 0.02, 10)
		
	var rotate_spd := 0.005
	if Input.is_action_pressed("rotate"):
		if event is InputEventMouseMotion:
			var warped_pos := (Vector2(
				fposmod(event.position.x, get_viewport().size.x),
				fposmod(event.position.y, get_viewport().size.y)
			))
			
			if warped_pos.distance_squared_to(event.position) > 0.01:
				Input.warp_mouse(warped_pos)
				_has_mouse_warped = true
			elif _has_mouse_warped:
				_has_mouse_warped = false
			else:
				var mouse_vel: Vector2 = event.relative
				x_rotator.rotate_x(-mouse_vel.y * rotate_spd)
				rotate_y(-mouse_vel.x * rotate_spd)

				
# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	if Input.is_action_pressed("rotate"):
		var spd := 2 * delta
		camera.position.x += (Input.get_action_strength("move_right") - Input.get_action_strength("move_left")) * spd
		camera.position.y += (Input.get_action_strength("move_up") - Input.get_action_strength("move_down")) * spd
	
	if xr_interface:
		var rotate_spd := 1 * delta
		var ls_vec = xr_left.get_input("primary")
		if ls_vec == null: ls_vec = Vector2()
		var move_spd = 5 * delta
		if xr_left.get_input("trigger") and xr_left.get_input("trigger") > 0.5:
			camera.position.x += ls_vec.x * move_spd
			camera.position.z += -ls_vec.y * move_spd
		elif xr_left.get_input("grip") and xr_left.get_input("grip") > 0.5:
			camera.position.x += ls_vec.x * move_spd
			camera.position.y += ls_vec.y * move_spd
		else:	
			x_rotator.rotate_x(-ls_vec.y * rotate_spd)
			rotate_y(-ls_vec.x * rotate_spd)
